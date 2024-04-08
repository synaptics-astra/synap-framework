/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2022 Synaptics Incorporated. All rights reserved.
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
/// Data [de]quantization and normalization.
///

#pragma once

#include "synap/metadata.hpp"


namespace synaptics {
namespace synap {

/// @return true if normalize_quantize operation is actually just a direct copy
/// This is the case if attr has no normalization and no quantization
/// or normalization and quantization simplify each other.
bool normalize_quantize_is_copy(const TensorAttributes* attr);

/// Convert, normalize and quantize uint8_t data to tensor
bool normalize_quantize(void* dst, const uint8_t* src, size_t size, const TensorAttributes* attr);

/// Convert, normalize and quantize int16_t data to tensor
bool normalize_quantize(void* dst, const int16_t* src, size_t size, const TensorAttributes* attr);

/// Convert, normalize and quantize float data to tensor
bool normalize_quantize(void* dst, const float* src, size_t size, const TensorAttributes* attr);

/// Convert quantized tensor data buffer to float
bool dequantize(float* dst, const uint8_t* src, size_t size, const TensorAttributes* src_attr);


}  // namespace synap
}  // namespace synaptics
