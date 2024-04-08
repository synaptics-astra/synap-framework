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

#include "synap/image_postprocessor.hpp"
#include "synap/file_utils.hpp"
#include "synap/string_utils.hpp"
#include "synap/image_convert.hpp"
#include "synap/logging.hpp"
#include "synap/tensor.hpp"

#include <cctype>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

using namespace std;

namespace synaptics {
namespace synap {


ImagePostprocessor::Result ImagePostprocessor::process(const Tensors& tensors)
{
    Result result;

    if (tensors.size() == 2) {
        const Tensor* y{};
        const Tensor* uv{};
        if (tensors[0].format() == "y8" && tensors[1].format() == "uv8") {
            y = &tensors[0];
            uv = &tensors[1];
        }
        else if (tensors[1].format() == "y8" && tensors[0].format() == "uv8") {
            y = &tensors[1];
            uv = &tensors[0];
        }
        else {
            LOGE << "Unsupported formats: " << tensors[0].format() << ", " << tensors[1].format();
            return {};
        }
        
        Dimensions dim = y->dimensions();
        result.dim.x = dim.w;
        result.dim.y = dim.h;
        result.ext = "nv12";
        result.data.resize(y->size() * 3 / 2);
        image_convert_to_yuv420sp((uint8_t*)y->data(), (uint8_t*)uv->data(),
                                  uv->layout() == Layout::nchw, dim.h, dim.w, result.data.data());
    }
    else if (tensors.size() == 1) {
        const Tensor* t = &tensors[0];
        result.ext = format_parse::get_type(t->format());
        if (result.ext.empty()) {
            Dimensions dim = t->dimensions();
            result.dim.x = dim.w;
            result.dim.y = dim.h;
            // Try to deduce image type from the number of channels
            switch(dim.c) {
            case 1: result.ext = "gray"; break;
            case 3: result.ext = "rgb"; break;
            case 4: result.ext = "bgra"; break;
            default: result.ext = "dat"; break;
            }
        }

        result.data.resize(t->size());
        bool interleaved_fmt = result.ext == "rgb" || result.ext == "bgr" || result.ext == "bgra";
        if (interleaved_fmt && t->layout() == Layout::nchw) {
            switch(synap_type_size(t->data_type())) {
            case 1:
                if (!nchw_to_nhwc(t->shape(), static_cast<const uint8_t*>(t->data()), result.data.data())) {
                    LOGE << "Error converting image to nhwc";
                    return {};
                }
                break;
            case 2:
                if (!nchw_to_nhwc(t->shape(), static_cast<const uint16_t*>(t->data()), reinterpret_cast<uint16_t*>(result.data.data()))) {
                    LOGE << "Error converting image to nhwc";
                    return {};
                }
            case 4:
                if (!nchw_to_nhwc(t->shape(), static_cast<const uint32_t*>(t->data()), reinterpret_cast<uint32_t*>(result.data.data()))) {
                    LOGE << "Error converting image to nhwc";
                    return {};
                }
                break;
            default:
                LOGE << "Error converting image to nhwc, item size: " << synap_type_size(t->data_type());
                return {};
            }
        }
        else {
            memcpy(result.data.data(), t->data(), result.data.size());
        }
    }
    else {
        LOGE << "Unsupported number of tensors: " << tensors.size();
    }

    return result;
}


}  // namespace synap
}  // namespace synaptics
