/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2022 Synaptics Incorporated. All rights reserved.
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
/// Synap command line test application.
///

#include "synap/timer.hpp"
#include "synap/input_data.hpp"
#include "synap/preprocessor.hpp"
#include "synap/network.hpp"
#include "synap/classifier.hpp"
#include "synap/npu.hpp"
#include "synap/allocator.hpp"
#include "synap/buffer.hpp"
#include "synap/arg_parser.hpp"
#include "synap/file_utils.hpp"
#include "synap/enum_utils.hpp"

#ifdef ENABLE_PRIVATE_ALLOCATORS
#include "synap_allocators.hpp"
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <numeric>
#include <string>
#include <cstring>
#include <limits>
#include <thread>
#include <chrono>

using namespace std;
using namespace synaptics::synap;

/// Buffers security
enum class SecureMode {
    std,    ///< Automatically deduce buffer securty from tensor attribute
    none,   /// Use non-secure buffers for both inputs and outputs
    in,     /// Use secure buffers for inputs only
    out,    /// Use secure buffers for outputs only
    all     /// Use secure buffers for both inputs and outputs
};

static const EnumStr<SecureMode> secure_mode_values[] = {
    "auto", SecureMode::std,
    "none", SecureMode::none,
    "in", SecureMode::in,
    "out", SecureMode::out,
    "all", SecureMode::all,
    "", SecureMode::std,
};

/// Memory allocator to be used for Buffers
enum class AllocatorType {
    std,
    synap,
    dmabufheap,
    osal
};

static const EnumStr<AllocatorType> allocator_type_values[] = {
    "auto", AllocatorType::std,
#ifdef SYNAP_EBG_ENABLE
    "synap", AllocatorType::synap,
#endif
#ifdef ENABLE_PRIVATE_ALLOCATORS
    "dmabufheap", AllocatorType::dmabufheap,
    "osal", AllocatorType::osal,
#endif
    "", AllocatorType::std,
};

static const char* sec_msg[] = {
    "", " (secure)"
};

static InputData get_input_data(const string& input_filename, size_t size)
{
    // Automatically extend zero or random to all remaining inputs if no filename specified
    static string prev_filename;
    string filename = input_filename.empty() ? prev_filename : input_filename;
    prev_filename = filename == "zero" || filename == "random" ?  filename : "";

    if (filename.empty()) {
        cerr << "Error, input file not specified for network input" << endl;
        return InputData(nullptr, 0, InputType::raw);
    }
    if (filename == "zero") {
        // 0-filled input
        return InputData(vector<uint8_t>(size), InputType::raw);
    }
    if (filename == "random") {
        // Generate pseudo-random input
        vector<uint8_t> random_data(size);
        generate(begin(random_data), end(random_data), std::rand);
        return InputData(std::move(random_data), InputType::raw);
    }

    const char numstr_prefix[] = "value:";
    if (filename.find(numstr_prefix) == 0) {
        vector<uint8_t> value(max(sizeof(int), size));
        try {
            *(int*)&value[0] = stoi(string(&filename[sizeof(numstr_prefix) - 1]));
        }
        catch(...) {
            cerr << "Invalid input value: " << filename << endl;
        }
        return InputData(std::move(value), InputType::raw);
    }

    // Read input from file
    InputData in_file(filename);
    if (in_file.empty()) {
        cerr << "Error, unable to read data from file: " << filename << endl;
    }
    return in_file;
}


// Get a pointer to the global allocator with the specified attributes.
static Allocator* get_allocator(bool contiguous, bool secure, AllocatorType allocator_type) {
    struct AllocatorDesc {
        AllocatorType alloc_type;
        Allocator* (*alloc_fn[2][2])();
    };
    static const AllocatorDesc alloc_desc[] = {
        { AllocatorType::std, std_allocator },
#ifdef SYNAP_EBG_ENABLE
        { AllocatorType::synap, synap_allocator },
#endif
#ifdef ENABLE_PRIVATE_ALLOCATORS
        { AllocatorType::dmabufheap, dma_heap_allocator, dma_heap_secure_allocator, dma_heap_cma_allocator, dma_heap_cma_secure_allocator},
        { AllocatorType::osal, osal_allocator, osal_secure_allocator  }
#endif
    };
    Allocator* (*get_allocator)() = nullptr;
    for (const auto& desc: alloc_desc) {
        if (desc.alloc_type == allocator_type) {
            get_allocator = desc.alloc_fn[contiguous][secure];
            break;
        }
    }
    Allocator* allocator = get_allocator? get_allocator() : nullptr;
    if (!allocator) {
        cerr << "Error, selected memory allocation method not available." << endl;
        exit(1);
    }
    return allocator;
}


int main(int argc, char** argv)
{
    ArgParser args(argc, argv, "Synap network test program", "[options] data-file(s)");
    string model = args.get("-m", "<file> Model file (.synap or legacy .nb)");
    string nb = args.get("--nb", "<file> Deprecated, same as -m");
    string meta = args.get("--meta", "<file> json meta file for legacy .nb models");
    int num_predict = stoi(args.get("-r", "<n> Repeat count", "1"));
    int num_create = stoi(args.get("--rr", "<n> Repeat network creation count", "1"));
    int fps = stoi(args.get("--fps", "<n> Inferences per second", "0"));
#ifdef SYNAP_EBG_ENABLE
    string secure_mode_str = args.get("--secure", "<s> Buffer security: " + enum_tags(secure_mode_values));
    string allocator_type_str = args.get("--allocator", "<s> Allocator: " + enum_tags(allocator_type_values));
#endif
    bool use_contiguous = args.has("--cma", "Use contiguous memory allocator");
    bool cpu_access = !args.has("--noflush", "Skip cache flush/invalidate");
    bool top5 = args.has("--top5", "Print top5 classes");
    bool dump_raw = args.has("--dump-raw", "Save raw outputs to file");
    bool dump_float = args.has("--dump-out", "Save denormalized outputs to file");
    string profiling_filename = args.get("--profiling", "<file> Save sysfs profiling info to file");
#ifdef SYNAP_EBG_ENABLE
    bool lock = args.has("--lock", "Lock inference during execution");
#endif
    bool show_version = args.has("--version", "Show SyNAP version");
    args.check_help("--help", "Show help",
                    "Special file names:\n"
                    "  zero      -> automatically generate 0-filled input\n"
                    "  random    -> automatically generate random input\n"
                    "  value:<n> -> write the given value to scalar input\n");
    if (show_version) {
        cout << "SyNAP version: " << synap_version() << endl;
        return 0;
    }
    validate_model_arg(model, nb, meta);

    AllocatorType allocator_type;
    SecureMode secure_mode{};
#ifdef SYNAP_EBG_ENABLE
    if (!get_enum(allocator_type_str, allocator_type_values, allocator_type)) {
        cerr << "Unknown allocator: " << allocator_type_str << endl;
        return 1;
    }

    if (!get_enum(secure_mode_str, secure_mode_values, secure_mode)) {
        cerr << "Unknown secure mode: " << secure_mode_str << endl;
        return 1;
    }

    Npu npu;
    if (!npu.available()) {
        cerr << "Error, NPU unavailable" << endl;
        return 1;
    }
    if (lock) {
        if (!npu.lock()) {
            cerr << "Error, NPU lock failed" << endl;
            return 1;
        }
        cout << "NPU locked" << endl;
    }
#endif

    // Get input files
    vector<string> input_filenames;
    string filename;
    while ((filename = args.get()) != "") {
        input_filenames.push_back(filename);
    }

    for(int nc = 0; nc < num_create; nc++) {
        // Create and load the model
        Timer load_tmr;
        vector<uint8_t> model_data;
        string meta_data;
        if (file_exists(model)) {
            // Preload model if possible so we keep model load time separate from init time
            model_data = binary_file_read(model);
            if (model_data.empty()) {
                cerr << "Failed to load model file: " << model << endl;
                return 1;
            }
            if (!meta.empty()) {
                meta_data = file_read(meta);
                if (meta_data.empty()) {
                    cerr << "Failed to load metafile: " << meta << endl;
                    return 1;
                }
            }
        }
        auto load_time_ms = load_tmr.get() / 1e3;

        Preprocessor preprocessor;
        Network net;
        Timer init_tmr;
        cout << "Loading network: " << model << endl;
        if (model_data.empty()? !net.load_model(model, meta) :
                !net.load_model(model_data.data(), model_data.size(), meta_data.empty() ? nullptr: meta_data.data())) {
            cerr << "Failed to load network" << endl;
            return 1;
        }
        auto init_time_ms = init_tmr.get() / 1e3;
        auto delay = fps > 0 ? 1000 / fps : 0;

        cout << "Flush/invalidate: " << (cpu_access ? "yes" : "no") << endl;
        cout << "Loop period (ms): " << delay << endl;
        cout << "Network inputs: " << net.inputs.size() << endl;
        cout << "Network outputs: " << net.outputs.size() << endl;

        // Load input data
        // Set input tensors allocator and access
        vector<InputData> input_data;
        bool secure_inputs = false;
        for (Tensor& tensor : net.inputs) {
            string input_filename = input_data.size() < input_filenames.size()
                                    ? input_filenames[input_data.size()] : "";
            InputData data = get_input_data(input_filename, tensor.size());
            if (data.empty()) {
                return 1;
            }
            input_data.push_back(std::move(data));
            bool is_secure = secure_mode == SecureMode::all || secure_mode == SecureMode::in ||
                             (secure_mode == SecureMode::std && tensor.security() == Security::secure);
            cout << "Input buffer: " << tensor.name() << " size: " << tensor.size()
                 << sec_msg[is_secure] << " : "<< input_filename << endl;
            secure_inputs |= is_secure;
            tensor.buffer()->set_allocator(get_allocator(use_contiguous, is_secure, allocator_type));
            tensor.buffer()->allow_cpu_access(cpu_access);
            if (is_secure && !tensor.data() && !input_filename.empty() && input_filename != "random") {
                cerr << "Error, only 'random' input allowed with secure memory" << endl;
                return 1;
            }
        }

        // Set output tensors allocator and access
        for(Tensor& tensor : net.outputs) {
            bool is_secure = secure_mode == SecureMode::all || secure_mode == SecureMode::out ||
                             (secure_mode == SecureMode::std && (tensor.security() == Security::secure ||
                             (tensor.security() == Security::secure_if_input_secure && secure_inputs)));
            cout << "Output buffer: " << tensor.name() << " size: " << tensor.size()
                 << sec_msg[is_secure] << endl;
            tensor.buffer()->set_allocator(get_allocator(use_contiguous, is_secure, allocator_type));
            tensor.buffer()->allow_cpu_access(cpu_access);
        }

        // Run inference, given network binary graph, graph metadata, and inputs
        cout << " " << endl;
        vector<double> infer_times;
        for (int32_t i = 0; i < num_predict; i++) {
            cout << "Predict #" << i << ": ";

            // Set input data.
            // We do this at each iteration even if the data doesn't change to show the effect
            // of cache-flush operations.
            size_t input_index = 0;
            for(Tensor& tensor : net.inputs) {
                if (secure_inputs && !tensor.data()) {
                    // Tensor with inaccessible secure buffer and random input. Just skip assigment.
                    input_index++;
                    continue;
                }
                if (tensor.is_scalar()) {
                    const void* v = input_data[input_index++].data();
                    size_t sz = input_data.size();
                    int value = sz >= 4 ? *(int32_t*)v : sz >= 2 ? *(int16_t*)v : *(int8_t*)v;
                    if (!tensor.assign(value)) {
                        cerr << "Error assigning input to scalar tensor " << input_index - 1 << endl;
                        return 1;
                    }
                }
                else {
                    if (!preprocessor.assign(tensor, input_data[input_index++])) {
                        cerr << "Error assigning input to tensor " << input_index - 1 << endl;
                        return 1;
                    }
                }
            }

            // Do inference
            Timer tmr;
            if (!net.predict()) {
                cerr << "Prediction " << i << " failed" << endl;
                return 1;
            }
            auto t = tmr.get();
            auto t_ms = t / 1e3;
            infer_times.push_back(t_ms);
            cout << tmr;

            if (!profiling_filename.empty()) {
                string sysfs_file = "/sys/class/misc/synap/device/misc/synap/statistics/network_profile";
                string profiling = file_read(sysfs_file);
                if (profiling.empty()) {
                    cerr << "\nUnable to read profiling info from: " << sysfs_file << endl;
                    return 1;
                }
                if (!binary_file_write(profiling_filename, profiling.data(), profiling.size())) {
                    cerr << "\nUnable to write profiling info to: " << profiling_filename << endl;
                    return 1;
                }
            }

            if (top5) {
                // Print Top 5 probabilities
                Classifier classifier(5);
                auto top = classifier.process(net.outputs);
                if (!top.success) {
                    cerr << "    Error, classification result not available" << endl;
                    return 1;
                }
                cout << "    Top5: ";
                for (auto item: top.items) {
                    cout << setw(3) << item.class_index << ": " << fixed << setprecision(2)
                         << item.confidence << ", ";
                }
            }
            cout << endl;

            this_thread::sleep_for(chrono::milliseconds(max(0, int(delay - t_ms))));
        }
        cout << " " << endl;

        if (dump_raw)  {
            // Dump raw outputs
            int index = 0;
            for(const auto& out : net.outputs) {
                string filename = string("output_raw_") + to_string(index++) + ".dat";
                cout << "Writing raw out: " << out.name() << " to file: " << filename << endl;
                binary_file_write(filename, out.data(), out.size());
            }
        }

        if (dump_float) {
            // Save denormalized outputs
            int index = 0;
            for(const auto& out : net.outputs) {
                string filename = string("output_float_") + to_string(index++) + ".dat";
                cout << "Writing denormalized out: " << out.name() << " to file: " << filename << endl;
                binary_file_write(filename, out.as_float(), out.item_count() * sizeof(float));
            }
        }

        // Inference time statistics
        if (infer_times.empty()) {
            infer_times.push_back(0);
        }
        double time_mean = accumulate(begin(infer_times), end(infer_times), 0.0) / infer_times.size();
        double time2_mean = inner_product(begin(infer_times), end(infer_times), begin(infer_times), 0.0)
                            / infer_times.size();
        double time_std_dev = sqrt(time2_mean - time_mean * time_mean);
        sort(begin(infer_times), end(infer_times));
        cout << "Inference timings (ms):" << fixed << setprecision(2)
            << "  load: " << load_time_ms
            << "  init: " << init_time_ms
            << "  min: " << infer_times.front()
            << "  median: " << infer_times[infer_times.size() / 2]
            << "  max: " << infer_times.back()
            << "  stddev: " << time_std_dev
            << "  mean: " << time_mean << endl;
    }

    return 0;
}
