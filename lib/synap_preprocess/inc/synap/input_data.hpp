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
/// Input data for synap network.
///

#pragma once

#include <string>
#include <vector>

#include "synap/types.hpp"

namespace synaptics {
namespace synap {

class Tensor;


/// Input type
enum class InputType {
    /// Unsupported input file type
    invalid,

    /// Raw binary data
    raw,

    /// Encoded image: jpeg, png
    encoded_image,
    
    /// 8-bits image (RGB[A], grayscale) interleaved or planar
    image_8bits,

    /// YUV420semiplanar: YYYY..UVUV...
    nv12,

    /// As nv12 but with UV order reversed: YYYY..VUVU...
    nv21
};


/// Input data container
class InputData {
public:
    InputData() {}
    InputData(InputData&& rhs);
    InputData(const InputData& rhs) = delete;
    InputData& operator=(InputData&& rhs);
    InputData& operator=(const InputData& rhs) = delete;
    ~InputData() {}

    ///  Construct with a file.
    ///  The content type is deduced from the filename extension.
    ///  File types supported:
    ///    encoded images (.jpeg, .png),
    ///    raw binary (.dat, .bin)
    ///
    ///  @param filename: Filename to load data from
    InputData(const std::string& filename);

    /// Construct from a memory buffer.
    ///
    /// @param buffer: pointer to input data.
    ///        Note: the data are NOT copied and must remain available as long as this InputData
    ///        object is used.
    /// @param buffer_size: data size
    /// @param type: data type
    /// @param shape: data shape (not needed for encoded_image)
    ///        The order of the items in shape[] must correspond to the layout used, so for
    ///        example for a 640x480 RGB image with nhwc layout use shape = [1,480,640,3]
    /// @param layout: data layout  (not needed for encoded_image).
    ///        Use Layout::nchw for planar images, and Layout::nhwc for interleaved images.
    InputData(const uint8_t* buffer, size_t buffer_size, InputType type, Shape shape = {},
              Layout layout = Layout::none);

    /// Construct from a memory buffer.
    /// This is equivalent to the constructor taking a pointer, the difference is that in this case
    /// the InputData object will take ownership of the data and release them when the object
    /// is destroyed.
    ///
    /// @param buffer: input data
    /// @param type: data type
    /// @param shape: data shape (not needed for encoded_image)
    /// @param layout: data layout  (not needed for encoded_image)
    InputData(std::vector<uint8_t>&& buffer, InputType type, Shape shape = {},
              Layout layout = Layout::none);

    ///  Check if data present or not.
    ///
    ///  @return true if no data (e.g. read from file failed)
    bool empty() const;

    ///  Get pointer to data.
    ///
    ///  @return data address, or nullptr if none
    const void* data() const;

    ///  Get data size in bytes.
    ///
    ///  @return data size, or 0 if empty
    size_t size() const;

    /// @return input type
    InputType type() const;

    /// @return data layout
    Layout layout() const;

    /// @return data shape
    const Shape& shape() const;

    /// @return data dimensions (all 0s if the shape is not 4D)
    Dimensions dimensions() const;

    /// @return data format
    const std::string& format() const;

    
    /// Get input type from file name
    /// 
    /// @param filename: file name
    /// @param[out] fmt: file format
    /// @param[out] channels: number of channels for this file type
    /// @return input type (invalid if unsupported file extension)
    static InputType input_type(const std::string& filename, std::string* fmt, float* channels);

private:
    void init();
    std::vector<uint8_t> _data;
    const uint8_t* _data_ptr{};
    size_t _data_size{};
    InputType _type{};
    Shape _shape{};
    Layout _layout{};
    std::string _format;
};

}  // namespace synap
}  // namespace synaptics
