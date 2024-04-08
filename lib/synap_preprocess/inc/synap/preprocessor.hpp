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
/// Preprocessor for synap network.
///

#pragma once

#include <string>
#include <vector>

#include "synap/input_data.hpp"

namespace synaptics {
namespace synap {

class Tensor;
class Tensors;

/// Preprocessor to convert an InputData and assign it to a Tensor
class Preprocessor {
public:
    ///  Construct with default preprocessing options.
    /// There are no customizable preprocessing options at the moment
    Preprocessor() {}

    /// Write InputData to tensor.
    /// Perform conversions to adapt the data to the tensor layout and format if possible.
    /// The tensor format can be specified in the conversion metafile when the network is compiled.
    /// The following formats are currently supported:
    /// - rgb: 8-bits RGB image
    /// - bgr: 8-bits BGR image
    /// - y8: 8-bits y component of yuv image
    /// - uv8: 8-bits uv interleaved components of yuv image
    /// - vu8: 8-bits vu interleaved components of yuv image
    ///
    /// @param t: destination tensor
    /// @param data: input data to be assigned
    /// @param[out] assigned_rect: if not nullptr will contain the coordinates of the part of the
    /// input image actually assigned to the tensor, keeping into account the aspect ratio used
    /// @return true if success
    bool assign(Tensor& t, const InputData& data, Rect* assigned_rect = nullptr) const;

    /// Write InputData to tensors.
    /// Perform conversions to adapt the data to the tensors layout and format if possible.
    /// Equivalent to assigning the InputData to all destination tensors.
    ///
    /// @param ts: destination tensors
    /// @param data: input data to be assigned
    /// @param start_index: index of the first tensor to assign
    /// @param[out] assigned_rect: if not nullptr will contain the coordinates of the part of the
    /// input image actually assigned to the tensor, keeping into account the aspect ratio used
    /// @return true if success
    bool assign(Tensors& ts, const InputData& data, size_t start_index = 0, Rect* assigned_rect = nullptr) const;

    /// Set region of interest.
    /// The input images will be cropped to the specified rectangle if not empty.
    /// Note: requires the model to have been compiled with cropping enabled,
    /// otherwise assignment will fail.
    /// 
    /// @param roi: cropping rectangle (no cropping if empty)
    void set_roi(const Rect& roi);

private:
    // Region of interest in the input image
    Rect _roi{};
};


}  // namespace synap
}  // namespace synaptics
