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


#include "synap/image_convert.hpp"
#include "synap/logging.hpp"

#include <cstring>

using namespace std;


namespace synaptics {
namespace synap {


bool image_convert_yuv420sp_to_y(const uint8_t* in_data,  int32_t height,  int32_t width,
                           uint8_t* out_data)
{
    // YUV420SP (NV21) to y:
    // YYY..UVUV... -> YYY...

    // Copy Y
    const uint32_t size_y = height * width;
    memcpy(out_data, in_data, size_y * sizeof(uint8_t));
    return true;
}


bool image_convert_yuv420sp_to_uv(const uint8_t* in_data,  int32_t height,  int32_t width,
                           uint8_t* out_data, bool planar)
{
    // YUV420SP (NV21) to uv planar:
    // YYY..UVUV... -> UUU...VVV...

    // Set source pointer to the start of interleaved uv section
    const uint32_t size_y = height * width;
    const uint8_t* psrc = &in_data[size_y];

    if (planar) {
        // Split and copy
        uint8_t* pdst_u = out_data;
        uint8_t* pdst_v = pdst_u + size_y / 4;
        for (int32_t i = 0; i < size_y / 4; i++) {
            *pdst_u++ = *(psrc + i * 2);
            *pdst_v++ = *(psrc + i * 2 + 1);
        }
    }
    else {
        memcpy(out_data, psrc, size_y / 2);
    }
    return true;
}

bool image_convert_yuv420sp_to_planar(const uint8_t* in_data,  int32_t height,  int32_t width,
                           uint8_t* out_data)
{
    // YUV420SP (NV21) to planar:
    // YYY..UVUV... -> YYY...UUU...VVV...

    uint32_t size_y = height * width;
    return image_convert_yuv420sp_to_y(in_data, height, width, out_data) &&
        image_convert_yuv420sp_to_uv(in_data, height, width, out_data + size_y, true);
}


bool image_convert_to_yuv420sp(const uint8_t* y, const uint8_t* uv, bool planar, int32_t height,
                          int32_t width, uint8_t* out_data)
{
    // planar to YUV420SP (NV21):
    // YYY...UUU...VVV... -> YYY..UVUV...

    const uint8_t* psrc = y;
    uint8_t* pdst_y = out_data;
    const uint32_t size_y = height * width;
    uint8_t* pdst_u = pdst_y + size_y;

    // Copy Y
    memcpy(pdst_y, psrc, size_y * sizeof(uint8_t));

    if (uv) {
        if (planar) {
            // Copy interleaved UV
            const uint8_t* psrc_u = uv;
            const uint8_t* psrc_v = psrc_u + size_y / 4;
            uint8_t* pdst_v = pdst_u + 1;
            for (int32_t i = 0; i < size_y / 4; i++) {
                *pdst_u = *psrc_u++;
                *pdst_v = *psrc_v++;
                pdst_u += 2;
                pdst_v += 2;
            }
        }
        else {
            memcpy(pdst_u, uv, size_y / 2);
        }
    }
    else {
        memset(pdst_u, 128, size_y / 2);
    }
    return true;
}


}  // namespace synap
}  // namespace synaptics
