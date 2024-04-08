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

#include "synap/image_utils.hpp"
#include "synap/file_utils.hpp"
#include "synap/logging.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>


using namespace std;

namespace synaptics {
namespace synap {


static inline int get_stbi_comp(ImageType type)
{
    // 1=Y, 2=YA, 3=RGB, 4=RGBA
    int comp;
    switch (type) {
    case ImageType::invalid:
        comp = -1;
        break;
    case ImageType::grayscale:
        comp = 1;
        break;
    case ImageType::rgb:
        comp = 3;
        break;
    case ImageType::rgba:
        comp = 4;
        break;
    }
    return comp;
}


static inline ImageType from_stbi_comp(int comp)
{
    // 1=Y, 2=YA, 3=RGB, 4=RGBA
    ImageType type = ImageType::invalid;
    switch (comp) {
    case 1:
        type = ImageType::grayscale;
        break;
    case 3:
        type = ImageType::rgb;
        break;
    case 4:
        type = ImageType::rgba;
        break;
    }
    return type;
}

vector<uint8_t> image_file_read(const string& file_name, int32_t* width, int32_t* height,
                                ImageType* type)
{
    int comp;
    uint8_t* data = stbi_load(file_name.c_str(), width, height, &comp, 0);
    if (data == nullptr) {
        LOGE << "Error reading file " << file_name;
        *type = ImageType::invalid;
        return vector<uint8_t>();
    }
    *type = from_stbi_comp(comp);
    vector<uint8_t> v{data, data + (*width) * (*height) * comp};
    stbi_image_free(data);
    return v;
}

vector<uint8_t> image_read(const uint8_t* input, size_t inputSize, int32_t* width, int32_t* height,
                           ImageType* type)
{
    int comp;
    *type = ImageType::invalid;
    uint8_t* data = stbi_load_from_memory(input, inputSize, width, height, &comp, 0);
    if (data == nullptr) {
        LOGE << "Error reading data";
        return vector<uint8_t>();
    }
    *type = from_stbi_comp(comp);
    vector<uint8_t> v{data, data + (*width) * (*height) * comp};
    stbi_image_free(data);
    return v;
}

bool bmp_file_write(const uint8_t* data, const string& file_name, int32_t width, int32_t height,
                    ImageType type)
{
    const auto comp = get_stbi_comp(type);
    if (!stbi_write_bmp(file_name.c_str(), width, height, comp, data)) {
        LOGE << "bmp_write failed: " << file_name;
        return false;
    }
    return true;
}

bool jpg_file_write(const uint8_t* data, const string& file_name, int32_t width, int32_t height,
                    ImageType type, int quality)
{
    const auto comp = get_stbi_comp(type);
    if (!stbi_write_jpg(file_name.c_str(), width, height, comp, data, quality)) {
        LOGE << "jpg_write failed: " << file_name;
        return false;
    }
    return true;
}

bool png_file_write(const uint8_t* data, const string& file_name, int32_t width, int32_t height,
                    ImageType type)
{
    const auto comp = get_stbi_comp(type);
    const int stride_in_bytes = width * comp;
    if (!stbi_write_png(file_name.c_str(), width, height, comp, data, stride_in_bytes)) {
        LOGE << "png_write failed: " << file_name;
        return false;
    }
    return true;
}


}  // namespace synap
}  // namespace synaptics
