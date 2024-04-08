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

#pragma once

///
/// Synap classifier postprocessor.
///

#include "synap/tensor.hpp"

namespace synaptics {
namespace synap {


/// Classification post-processor for Network output tensors.
/// Determine the top-N classifications of an image.
class Classifier {
public:
    /// Classification result
    struct Result {

        /// Classification item.
        struct Item {
            /// Index of the class
            int32_t class_index;

            /// Confidence of the classification, normally in the range [0, 1]
            float confidence;
        };

        /// True if classification successful, false if failed.
        bool success{};
        
        /// List of possible classifications for the input, sorted in descending confidence order,
        /// that is items[0] is the classification with the highest confidence.
        /// Empty if classification failed.
        std::vector<Item> items;
    };


    /// Constructor
    /// @param top_count: number of most probable classifications to return
    Classifier(size_t top_count = 1) : _top_count{top_count} {}

    /// Perform classification on network output tensors.
    /// 
    /// @param tensors: output tensors of the network
    /// tensors[0] is expected to contain a list of confidences, one for each image class
    /// @return classification results
    Result process(const Tensors& tensors);

private:
    size_t _top_count;
};


std::string to_json_str(const Classifier::Result& result);

}  // namespace synap
}  // namespace synaptics
