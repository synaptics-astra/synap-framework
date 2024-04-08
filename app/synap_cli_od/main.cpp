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
/// Synap command line application for Object Detection.
///

#include "synap/arg_parser.hpp"
#include "synap/input_data.hpp"
#include "synap/preprocessor.hpp"
#include "synap/network.hpp"
#include "synap/detector.hpp"
#include "synap/timer.hpp"
#include "synap/label_info.hpp"
#include "synap/file_utils.hpp"
#include <iomanip>
#include <iostream>


using namespace std;
using namespace synaptics::synap;


int main(int argc, char* argv[])
{
    ArgParser args(argc, argv, "Object detection on image files", "[options] file(s)");
    string model = args.get("-m", "<file> Model file (.synap or legacy .nb)");
    string nb = args.get("--nb", "<file> Deprecated, same as -m");
    string meta = args.get("--meta", "<file> json meta file for legacy .nb models");
    float score_threshold = stof(args.get("--score-threshold", "<thr> Min confidence", "0.5"));
    int n_max = stoi(args.get("--max-detections", "<n> Max number of detections [0: all]", "0"));
    bool nms = !args.has("--disable-nms", "Disable Non-Max-Suppression algorithm");
    float iou_threshold = stof(args.get("--iou-threshold", "<thr> IOU threashold for NMS", "0.5"));
    bool iou_with_min = args.has("--iou-with-min", "Use min area instead of union to compute IOU");
    args.check_help("--help", "Show help");
    validate_model_arg(model, nb, meta);

    Preprocessor preprocessor;
    Network network;
    Detector detector(score_threshold, n_max, nms, iou_threshold, iou_with_min);
    LabelInfo info(file_find_up("info.json", filename_path(model)));
    cout << "Loading network: " << model << endl;
    if (!network.load_model(model, meta)) {
        cerr << "Failed to load model" << endl;
        return 1;
    }
    if (!detector.init(network.outputs)) {
        cerr << "Failed to initialize detector" << endl;
        return 1;
    }

    string input_filename;
    while ((input_filename = args.get()) != "") {
        // Load input data
        cout << " " << endl;
        InputData image(input_filename);
        if (image.empty()) {
            cerr << "Error, unable to read data from file: " << input_filename << endl;
            return 1;
        }

        Timer t;
        Rect assigned_rect;
        if (!preprocessor.assign(network.inputs, image, 0, &assigned_rect)) {
            cerr << "Error assigning input to tensor." << endl;
            return 1;
        }
        auto t0 = t.get();
        Dimensions dim = image.dimensions();
        cout << "Input image: " << input_filename << " (w = " << dim.w
             << ", h = " << dim.h << ", c = " << dim.c << ")" << endl;

        // Execute inference
        if (!network.predict()) {
            cerr << "Inference failed" << endl;
            return 1;
        }
        auto t1 = t.get();

        // Postprocess network outputs
        Detector::Result result = detector.process(network.outputs, assigned_rect);
        if (!result.success) {
            cout << "Detection failed" << endl;
            return 1;
        }
        auto t2 = t.get();
        cout << "Detection time: " << t
             << " (pre:" << t0 << ", inf:" << t1 - t0 << ", post:" << t2 - t1 << ")" << endl;

        if (output_redirected()) {
            // Generate output in json so that it can be easily parsed by other tools
            cout << to_json_str(result) << endl;
        }
        else {
            cout << "#   Score  Class   Position        Size  Description     Landmarks" << endl;
            int j = 0;
            for (const auto& item : result.items) {
                const auto& bb = item.bounding_box;
                cout << setw(3) << left << j++ << right << "  " << item.confidence << " " << setw(6)
                     << item.class_index << "  " << setw(4) << right << bb.origin.x << "," << setw(4)
                     << right << bb.origin.y << "   " << setw(4) << bb.size.x << "," << setw(4)
                     << right << bb.size.y <<  "  " << setw(16) << left << info.label(item.class_index);
                for (const auto lm: item.landmarks) {
                    cout << lm << " ";
                }
                cout << endl;
            }
        }
    }

    return 0;
}
