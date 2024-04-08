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

#pragma once

///
/// Synap object detection postprocessor.
///

#include "synap/tensor.hpp"
#include <vector>
#include <memory>

namespace synaptics {
namespace synap {


/// Object-detector
/// 
/// The output format of object-detection networks is not always the same but depends
/// on the network architecture used. The format type must be specified in the *format*
/// field of the output tensor in the network metafile when the network is compiled.
/// This following formats are currently supported: "retinanet_boxes", "tflite_detection_boxes",
/// "yolov5"
class Detector {
public:
    /// Object-detector result
    struct Result {
        /// Detection item.
        struct Item {
            /// Index of the object class
            int32_t class_index;

            /// Confidence of the detection in the range [0, 1]
            float confidence;

            /// Top,left corner plus horizontal and vertical size (in pixels)
            Rect bounding_box;

            /// One entry for each landmark.
            /// Empty if no landmark available.
            std::vector<Landmark> landmarks;
        };

        /// True if detection successful, false if detection failed.
        bool success{};

        /// One entry for each detection.
        /// Empty if nothing detected or detection failed.
        std::vector<Item> items;
    };


    /// Constructor
    ///
    /// @param score_threshold: detections below this score are discarded
    /// @param n_max: max number of detections (0: all)
    /// @param nms: if true apply non-max-suppression to remove duplicate detections
    /// @param iou_threshold: intersection-over-union threshold (used if nms is true)
    /// @param iou_with_min: use min area instead of union to compute intersection-over-union 
    Detector(float score_threshold = 0.5, int n_max = 0,
             bool nms = true, float iou_threshold = .5, bool iou_with_min = false);


    // Destructor
    ~Detector();


    /// Initialize detector.
    /// If not called the detector is automatically initialized the 1st time process() is called.
    /// 
    /// @param tensors: output tensors of the network (after the network has been loaded)
    /// @return true if success
    bool init(const Tensors& tensors);


    /// Perform detection on network output tensors.
    /// 
    /// @param tensors: output tensors of the network
    /// @param input_rect: coordinates of the (sub)image provided in input (to compute bounding-boxes)
    /// @return detection results
    Result process(const Tensors& tensors, const Rect& input_rect);

    // Implementation class
    class Impl;

private:
    // Detection options
    float _score_threshold{};
    int _max_detections{};
    bool _nms{};
    float _iou_threshold{};
    bool _iou_with_min{};

    // Implementation details
    std::unique_ptr<Impl> d;
};

std::string to_json_str(const Detector::Result& result);

}  // namespace synap
}  // namespace synaptics
