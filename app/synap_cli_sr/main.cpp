/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2021 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
///
/// Synap command line application for Super Resolution.
///

#include "synap/arg_parser.hpp"
#include "synap/buffer.hpp"
#include "synap/buffer_cache.hpp"
#include "synap/file_utils.hpp"
#include "synap/image_utils.hpp"
#include "synap/input_data.hpp"
#include "synap/network.hpp"
#include "synap/timer.hpp"
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

using namespace std;
using namespace synaptics::synap;


// Minimal simulation of an AMP BD.
// This class allows to simulate the use-case where memory for inference input data is not
// allocated by a Buffer but has been allocated externally.
class BdDummy {
    size_t _size;
    Allocator* _alloc;
    Allocator::Memory _mem;

public:
    BdDummy(size_t size, Allocator* a) : _size(size), _alloc{a}, _mem{_alloc->alloc(_size)} {}
    ~BdDummy() { _alloc->dealloc(_mem); }
    uint32_t mem_id() const { return _mem.mem_id; }
    void* data() const { return _mem.address; }
    size_t size() const { return _size; }
    bool assign(const void* d, size_t sz)
    {
        return _size == sz && memcpy(_mem.address, d, sz);
    }

    bool assign(const void* d, size_t y_size, size_t uv_offset)
    {
        size_t data_size = y_size;
        size_t mem_offset = uv_offset;
        cout << "data_size " << data_size << " mem_offset " << mem_offset << endl;

        memcpy(_mem.address, d, data_size);
        memcpy(static_cast<char*>(_mem.address) + mem_offset, static_cast<const char*>(d) + data_size, data_size / 2);
        cout << "memcpy done" << endl;
        return true;
    }
};

static void get_frame_info(Network *net, bool input, size_t *frame_mem_size, size_t *y_size, size_t *uv_offset)
{
    auto y_idx = net->inputs[0].size() > net->inputs[1].size() ? 0 : 1;

    Dimensions dim;
    if (input) {
        cout << "input ";
        dim = net->inputs[y_idx].dimensions();

    } else {
        cout << "output ";
        dim = net->outputs[y_idx].dimensions();
    }

    // simulate VxG wxh aligment logic
    // MPEG2/H264/VP8 request width and height align to 16
    // H265/VP9 w and h align to 64
    // if w and h both align to 64, uv offset will be 4096 page size alignment
    const size_t align_num = 64;
    auto aligned_h = Allocator::align(dim.h, align_num);
    auto aligned_w = Allocator::align(dim.w, align_num);
    *y_size = dim.w * dim.h;
    *uv_offset = aligned_h * aligned_w;
    *frame_mem_size = aligned_h * aligned_w * 3 / 2;
    cout << "wxh=" << dim.w << "x" << dim.h << " align to wxh=" << aligned_w << "x" << aligned_h
         << " memory size " << *frame_mem_size << endl;
}

int main(int argc, char** argv)
{
    ArgParser args(argc, argv, "Super-res upscaling on Y-binary images", "[options] y-bin-file(s)");
    string model = args.get("-m", "<file> Model file (.synap or legacy .nb)");
    string nb = args.get("--nb", "<file> Deprecated, same as -m");
    string meta = args.get("--meta", "<file> json meta file for legacy .nb models");
    args.check_help("--help", "Show help");
    validate_model_arg(model, nb, meta);

    Network net;
    if (!net.load_model(model, meta)) {
        cerr << "Failed to load network" << endl;
        return 1;
    }

    size_t in_mem_size, in_y_size, in_uv_offset;
    size_t out_mem_size, out_y_size, out_uv_offset;

    get_frame_info(&net, true, &in_mem_size, &in_y_size, &in_uv_offset);
    get_frame_info(&net, false, &out_mem_size, &out_y_size, &out_uv_offset);

    // Simulate BDs containing data
    auto allocator = std_allocator();
#ifdef ENABLE_OSAL_ALLOCATOR
    allocator = osal_allocator();
#endif
    BdDummy input_bd(in_mem_size, allocator);
    BdDummy output_bd(out_mem_size, allocator);

    // Use a buffer cache.
    // Even if not actually needed here we simulate the actual use where memory has
    // already been allocated inside BDs.
    BufferCache<BdDummy*> buffers{};

    string input_filename;
    while ((input_filename = args.get()) != "") {
        cout << " " << endl;
        cout << "Input image: " << input_filename << endl;
        InputData image(input_filename);
        if (image.empty()) {
            cout << "Error, unable to read data from file: " << input_filename << endl;
            return 1;
        }

        if (!input_bd.assign(image.data(), in_y_size, in_uv_offset)) {
            cerr << "Invalid input image size." << endl;
            return 1;
        }

        cout << "input mem_id " << input_bd.mem_id() << " y_size " << in_y_size << " uv offset " << in_uv_offset << endl;
        cout << "output mem_id " << output_bd.mem_id() << " y_size " << out_y_size << " uv offset " << out_uv_offset << endl;

        Timer tmr;
        Buffer* input_y = buffers.get(&input_bd, input_bd.mem_id(), in_y_size, 0);
        Buffer* input_uv = buffers.get(&input_bd, input_bd.mem_id(), in_y_size / 2, in_uv_offset);

        Buffer* output_y = buffers.get(&output_bd, output_bd.mem_id(), out_y_size, 0);
        Buffer* output_uv = buffers.get(&output_bd, output_bd.mem_id(), out_y_size / 2, out_uv_offset);

        auto y_idx = net.inputs[0].size() > net.inputs[1].size() ? 0 : 1;
        auto uv_idx = 1 - y_idx;

        if (!net.inputs[y_idx].set_buffer(input_y)) {
            cerr << "Failed to set input y"<< endl;
            return 1;
        }
        if (!net.inputs[uv_idx].set_buffer(input_uv)) {
            cerr << "Failed to set input uv"<< endl;
            return 1;
        }
        if (!net.outputs[y_idx].set_buffer(output_y)) {
            cerr << "Failed to set output y"<< endl;
            return 1;
        }
        if (!net.outputs[uv_idx].set_buffer(output_uv)) {
            cerr << "Failed to set output uv"<< endl;
            return 1;
        }

        bool success = net.predict();
        if (!success) {
            cout << "Upscaling failed" << endl;
            return 1;
        }
        cout << "Upscale time: " << tmr << endl;

        string output_filename_raw = input_filename + "_sr.dat";
        cout << "Writing super-res raw image to file: " << output_filename_raw << endl;
        ofstream wf;
        wf.open(output_filename_raw, ios::out | ios::binary);
        wf.write(static_cast<const char*>(output_bd.data()), out_y_size);
        wf.write(static_cast<const char*>(output_bd.data()) + out_uv_offset, out_y_size / 2);
        wf.close();
    }

    return 0;
}
