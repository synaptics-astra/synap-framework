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

#include "synap/input_data.hpp"
#include "synap/file_utils.hpp"
#include "synap/image_utils.hpp"
#include "synap/logging.hpp"
#include "synap/timer.hpp"
#include <stb_image.h>

#include <cmath>
#include <cstring>

using namespace std;

namespace synaptics {
namespace synap {


InputType InputData::input_type(const std::string& filename, std::string* fmt, float* channels)
{
    InputType type = InputType::invalid;
    string ext = filename_extension(filename);
    for (auto& c : ext)
        c = tolower(c);

    // Use file extension to deduce the fata format (eg: nv12, rgb etc)
    const char* format = ext.c_str();
    
    float nch{};
    if (ext == "jpg" || ext == "jpeg" || ext == "png") {
        type = InputType::encoded_image;
    }
    else if (ext == "bin" || ext == "dat") {
        type = InputType::raw;
        // For raw data files data format is unavailable
        format = "";
    }
    else if (ext == "nv21" || ext == "yuv420sp") {
        type = InputType::nv21;
        nch = 1.5;
    }
    else if (ext == "nv12" || ext == "yvu420sp") {
        type = InputType::nv12;
        nch = 1.5;
    }
    else if (ext == "rgb" || ext == "bgr") {
        type = InputType::image_8bits;
        nch = 3;
    }
    else if (ext == "bgra") {
        type = InputType::image_8bits;
        nch = 4;
    }
    else if (ext == "gray") {
        type = InputType::image_8bits;
        nch = 1;
    }

    if (fmt) {
        *fmt = format;
    }
    if (channels) {
        *channels = nch;
    }
    LOGV << "Input file extension " << ext << ", type " << (int)type << ", ch: " << nch;
    return type;
}


void InputData::init()
{
    if (_type == InputType::encoded_image) {
        int x{}, y{}, c{};
        if (_data_ptr && stbi_info_from_memory(_data_ptr, _data_size, &x, &y, &c)) {
            _shape = {1, y, x, c};
            _layout = Layout::nhwc;
            constexpr bool decode_now = true;
            if (decode_now) {
                // Convert image to RGB
                Timer tmr;
                ImageType img_type{};
                _data = image_read(_data_ptr, _data_size, &x, &y, &img_type);
                _data_ptr = _data.data();
                _data_size = _data.size();
                _type = InputType::image_8bits;
                _format = "rgb";
                LOGV << "Input image decoded in " << tmr;
            }
        }
        else if (_shape.empty()) {
            LOGE << "Unable to determine image dimensions";
        }
    }
}


InputData::InputData(const std::string& filename)
{
    if (!file_exists(filename)) {
        LOGE << "Input file not found: " << filename;
        return;
    }
    float channels{};
    _type = input_type(filename, &_format, &channels);
    if (_type == InputType::invalid) {
        LOGE << "Unknown file type " << filename;
        return;
    }

    // Read data from file
    _data = binary_file_read(filename);
    _data_ptr = _data.data();
    _data_size = _data.size();

    if (_type == InputType::image_8bits) {
        _layout = Layout::nhwc;
        // Deduce image dimensions from file name if possible
        // Filename should end with "_wxh" dimensions, eg: image_640x480.rgb
        std::string file_base_name = filename_without_extension(filename);
        size_t img_dim_start  = file_base_name.rfind('_');
        if (img_dim_start != string::npos) {
            int w, h;
            if (sscanf(&file_base_name[img_dim_start + 1], "%dx%d", &w, &h) == 2) {
                if (channels == 0 || round(h * w * channels) == _data_size) {
                    LOGV << "Deduced image dimensions: " << w << "x" << h;
                    _shape = {1, h, w, (int)round(channels)};
                }
                else {
                    LOGW << "Deduced image dimensions " << w << "x" << h
                         << " don't match file size: " << _data_size;
                }
            }
        }
    }

    init();
}


InputData::InputData(std::vector<uint8_t>&& buffer, InputType type, Shape shape, Layout layout) :
    _data{std::move(buffer)}, _data_ptr{_data.data()}, _data_size{_data.size()}, _type{type},
    _shape{shape}, _layout{layout}
{
    init();
}


InputData::InputData(const uint8_t* buffer, size_t size, InputType type, Shape shape, Layout layout)
    : _data_ptr{buffer}, _data_size{size}, _type{type}, _shape{shape}, _layout{layout}
{
    init();
}


InputData::InputData(InputData&& rhs) : InputData()
{
    *this = std::move(rhs);
}


InputData& InputData::operator=(InputData&& rhs)
{
    swap(_data_size, rhs._data_size);
    swap(_data_ptr, rhs._data_ptr);
    swap(_type, rhs._type);
    swap(_shape, rhs._shape);
    swap(_layout, rhs._layout);
    _format.swap(rhs._format);
    _data.swap(rhs._data);

    return *this;
}


bool InputData::empty() const
{
    return _data_ptr == nullptr;
}


const void* InputData::data() const
{
    return _data_ptr;
}


size_t InputData::size() const
{
    return _data_size;
}


InputType InputData::type() const
{
    return _type;
}


Layout InputData::layout() const
{
    return _layout;
}


const Shape& InputData::shape() const
{
    return _shape;
}


Dimensions InputData::dimensions() const
{
    return Dimensions(_shape, _layout);
}


const string& InputData::format() const
{
    return _format;
}

}  // namespace synap
}  // namespace synaptics
