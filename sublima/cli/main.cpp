/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
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
/// HDR image processing test application
///

#include "synap/arg_parser.hpp"
#include "synap/file_utils.hpp"
#include "synap/timer.hpp"
#include "synap/sublima.hpp"

#include <iostream>
#include <string>


using namespace synaptics::synap;
using namespace std;


int main(int argc, char **argv)
{
    ArgParser args(argc, argv, "Sublima image processing sample program", "[options] image-file");
    string model = args.get("-m", "<file> Model file (.synap or legacy .nb)");
    string nb = args.get("--nb", "<file> Deprecated, same as -m");
    string meta = args.get("--meta", "<file> Json meta file for legacy .nb models");
    string lut = args.get("--lut", "<file> CSV lookup table file", "lut.csv");
    string hdrjson = args.get("--hdr-info", "<file> Hdr json parameter file", "hdrinfo.json");
    string out_dir = args.get("--out-dir", "<dir> Output directory");
    bool hdr_disable = args.has("--hdr-off", "Disable hdr processing, it is enabled by default");
    bool only_y = args.has("--only-y", "Only Y channel processing, no UV conversion");
    int r = stoi(args.get("-r", "<n> Repeat count", "1"));

    args.check_help("--help", "Show help");
    validate_model_arg(model, nb, meta);
    
    // Check arguments
    if (lut != "-" && !file_exists(lut)) {
        cerr << "LUT2D file not found: " << lut << endl;
        return {};
    }
    string input_filename = args.get();
    if (input_filename.empty()) {
        cerr << "No input file specified" << endl;
        return 1;
    }

    // Init Sublima processor
    Sublima sublima;
    if (!sublima.init(lut, model, meta, hdrjson)) {
        cerr << "Failed to initialize Sublima" << endl;
        return 1;
    }

    if (hdr_disable) {
        sublima.set_hdr(false);
	cout << "disable hdr" << endl;
    }

    if (only_y) {
        sublima.set_only_y(true);
	cout << "only Y processing, no UV model processing" << endl;
    }

    // Perform conversion
    const uint32_t width = 1920, height = 1080;
    const uint32_t y_size = width * height;
    vector<uint8_t> nv15((y_size * 3 / 2) * 5 / 4);
    for (int i = 0; i < r; i++) {
        auto yuv_input = binary_file_read(input_filename);
        if (yuv_input.size() != y_size * 3 / 2) {
            cerr << "Failed to load image file: " << input_filename << endl;
            return 1;
        }
        sublima.process(yuv_input.data(), yuv_input.data() + y_size, nv15.data(), nv15.data() + y_size * 5 / 4, width, height);
    }

    if (r > 0) {
        // Show timings
        const Sublima::Timings* timings = sublima.timings();
        cout << "assign: " << timings->assign / r
             << ", infer: " << timings->infer / r
             << ", nv15y: " << timings->nv15y / r
             << ", nv15uv: " << timings->nv15uv / r
             << ", tot: " << timings->tot / r << endl;

        // Generate nv15 output file
        out_dir += out_dir.empty()? "" : "/";
        string out_file = out_dir + "outimage0_" + to_string(width) + 'x' + to_string(height) + ".nv15";
        cout << "Writing output to file: " << out_file << endl;
        if (!binary_file_write(out_file, nv15.data(), nv15.size())) {
            cerr << "Failed to write output file" << endl;
            return 1;
        }
    }

    return 0;
}

