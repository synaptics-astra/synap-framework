/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2023 Synaptics Incorporated. All rights reserved.
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
/// Synap command line application for Image Classification using a split (partitioned) network.
///

#include "synap/input_data.hpp"
#include "synap/preprocessor.hpp"
#include "synap/network.hpp"
#include "synap/classifier.hpp"
#include "synap/label_info.hpp"
#include "synap/timer.hpp"
#include "synap/arg_parser.hpp"
#include "synap/file_utils.hpp"

using namespace std;
using namespace synaptics::synap;


#define BASE_PATH "/vendor/firmware/models/image_classification/imagenet/"
static string sample_default = BASE_PATH "sample/space_shuttle_224x224.jpg";


int main(int argc, char** argv)
{
    ArgParser args(argc, argv, "Image classification using split network", "[options] file(s)");
    string model = args.get("-m", "<file> Model file (.synap or legacy .nb)");
    string nb = args.get("--nb", "<file> Deprecated, same as -m");
    string meta = args.get("--meta", "<file> json meta file for legacy .nb models");
    string model2 = args.get("-m2", "<file> 2nd model file (.synap or legacy .nb)");
    string nb2 = args.get("--nb2", "<file> Deprecated, same as -m2");
    string meta2 = args.get("--meta2", "<file> 2nd json meta file for legacy .nb models");
    bool partial_buffer_sharing = args.has("--partial", "Use partial buffer sharing");
    int num_predict = stoi(args.get("-r", "<n> Repeat count", "1"));
    args.check_help("--help", "Show help");
    validate_model_arg(model, nb, meta);
    validate_model_arg(model2, nb2, meta2);

    // Get sample images (or use default image if none specified)
    vector<string> images;
    string input_filename;
    while ((input_filename = args.get()) != "") {
        images.push_back(input_filename);
    }
    if (images.empty()) {
        images.push_back(sample_default);
    }

    // Create the preprocessor, models, postprocessor and label description 
    Preprocessor preprocessor;
    Network network2;
    Network network1;
    Classifier classifier(5);
    LabelInfo info(file_find_up("info.json", filename_path(model2)));

    // Load the 2 models
    cout << "Loading network 1: " << model << endl;
    if (!network1.load_model(model, meta)) {
        cerr << "Network 1 init failed" << endl;
        return 1;
    }
    cout << "Loading network 2: " << model2 << endl;
    if (!network2.load_model(model2, meta2)) {
        cerr << "Network 2 init failed" << endl;
        return 1;
    }

    if (partial_buffer_sharing) {
        // Check that the two networks have compatible in/out tensors
        if (network1.outputs[0].size() > network2.inputs[0].size()) {
            cerr << "Tensor size mismatch: network1 output size: " << network1.outputs[0].size() <<
                    ", network2 input size: " << network2.outputs[0].size() << endl;
            return 1;
        }

        // Initialize the entire destination tensor now that we still have CPU access to it
        memset(network2.inputs[0].data(), 0, network2.inputs[0].size());

        // Replace the output buffer with a new one using (part of) the memory of the target input buffer
        *network1.outputs[0].buffer() = Buffer(*network2.inputs[0].buffer(), 0, network1.outputs[0].size());
        if (!network1.outputs[0].buffer()->size()) {
            cerr << "Error sharing tensor buffer memory" << endl;
            return 1;
        }
        // Disable CPU access to both buffers to avoid the un-necessary overhead of cache flushing
        network1.outputs[0].buffer()->allow_cpu_access(false);
        network2.inputs[0].buffer()->allow_cpu_access(false);
    }
    else {
        // Full buffer memory sharing
        // Check that the two networks have compatible in/out tensors
        if (network1.outputs[0].shape() != network2.inputs[0].shape()) {
            cerr << "Tensor mismatch: network1 output shape: " << network1.outputs[0].shape() <<
                    ", network2 input shape: " << network2.inputs[0].shape() << endl;
            return 1;
        }

        // Disable CPU access for intermediate out to avoid the un-necessary overhead of cache flushing.
        network1.outputs[0].buffer()->allow_cpu_access(false);
    
        // Use network1 output as network2 input.
        if (!network2.inputs[0].set_buffer(network1.outputs[0].buffer())) {
            cerr << "Error sharing tensor buffers" << endl;
            return 1;
        }
    }
    
    // Run inference on all input images.
    // Inference can be repeated multiple times for more accurate timings.
    for (const auto& image_name : images) {
        // Read input image
        cout << "Input image: " << image_name << endl;
        InputData input_image(image_name);
        if (input_image.empty()) {
            cerr << "Error, unable to read data from file: " << image_name << endl;
            return 1;
        }

        auto min_t_ms = numeric_limits<double>::max();
        for (int32_t i = 0; i < num_predict; i++) {
            // Load input data to input tensor(s)
            if (!preprocessor.assign(network1.inputs, input_image)) {
                cerr << "Error assigning input to tensor." << endl;
                return 1;
            }
            // Execute inference with network1 and network2
            Timer t;
            if (!network1.predict()) {
                cerr << "Inference using the first network failed." << endl;
                return 1;
            }
            if (!network2.predict()) {
                cerr << "Inference using the second network failed." << endl;
                return 1;
            }
            min_t_ms = min(t.get() / 1e3, min_t_ms);
        }
        cout << "Inference time: " << fixed << setprecision(2) <<  min_t_ms << " ms " << endl;

        // Postprocess network outputs
        Classifier::Result result = classifier.process(network2.outputs);
        if (!result.success) {
            cerr << "Classification failed." << endl;
            return 1;
        }

        // Print classification results
        cout << "Class  Confidence  Description" << fixed << setprecision(2) << endl;
        for (auto item: result.items) {
            cout << setw(5) << item.class_index << setw(12) << item.confidence << "  "
                 << info.label(item.class_index) << endl;
        }
    }

    return 0;
}
