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

#include "synap/logging.hpp"
#include "synap/timer.hpp"
#include "quantization.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

using namespace std;

namespace synaptics {
namespace synap {


// Channel information
struct ChInfo {
    // Number of channels
    int count;
    // Stride between channels
    int stride;
};


// Tensor mean information
struct MeanInfo {
    // True if mean and channel info are valid
    bool valid;
    // True if any non-null mean
    bool has_mean;
    // True if mean is not the same for all channels
    bool per_channel_mean;
    // Channel information
    ChInfo ci;
};


// Shift data left (positive n) or right (negative n)
template<typename T1, typename T2>
inline auto shift(T1 val, T2 n)
{
    return n >= 0 ? val << n : val >> -n;
}


// Shift float data left (positive n) or right (negative n)
template<typename T2>
inline auto shift(float val, T2 n)
{
    return n >= 0 ? val * (1 << n) : val / (1 << n);
}


// Convert floating point to integer (with rounding)
inline int to_int(float val)
{
    constexpr bool round_to_nearest = true;
    return round_to_nearest ? val + 0.5 : val;
}

// Convert integer to integral number with fewer bits (with saturation)
template<typename T>
inline T limit(int val)
{
    constexpr bool saturate = true;
    if (saturate) {
        if (val < numeric_limits<T>::min()) {
            val = numeric_limits<T>::min();
        }
        else if (val > numeric_limits<T>::max()) {
            val = numeric_limits<T>::max();
        }
    }
    return val;
}


//
// Data dequantization
//

// Convert an integral value to 32 bits intger
// Assume little endian memory
static inline bool get_int32(const void* src, DataType src_type, int32_t* dest)
{
    switch (src_type) {
    case DataType::int8:
        *dest = *static_cast<const int8_t*>(src);
        return true;
    case DataType::uint8:
        *dest = *static_cast<const uint8_t*>(src);
        return true;
    case DataType::int16:
        *dest = *static_cast<const int16_t*>(src);
        return true;
    case DataType::uint16:
        *dest = *static_cast<const uint16_t*>(src);
        return true;
    case DataType::int32:
        *dest = *static_cast<const int32_t*>(src);
        return true;
    case DataType::uint32:
        *dest = *static_cast<const uint32_t*>(src);
        return true;
    default:
        LOGE << "int32 conversion not supported from: " << (int)src_type;
        return false;
    }

    LOGE << "int32 conversion not supported from: " << (int)src_type;
    return false;
}


// Convert dynamic-fixed-point integer to float
static inline float dfp_to_float(int32_t val, int fl)
{
    float fval = val;
    return fl > 0 ? fval / (1 << fl) : fval * (1 << -fl);
}


// Convert affine quantized integer to float
static inline float affine_to_float(int32_t val, float scale, int32_t zero_point)
{
    return (val - zero_point) * scale;
}


// Convert fp16 value to float
static inline float fp16_to_float(int16_t in)
{
    typedef union {
        uint32_t u;
        float f;
    } _fp32_t;

    constexpr _fp32_t magic = {(254 - 15) << 23};
    constexpr _fp32_t infnan = {(127 + 16) << 23};
    _fp32_t o;
    // Non-sign bits
    o.u = (in & 0x7fff) << 13;
    o.f *= magic.f;
    if (o.f >= infnan.f) {
        o.u |= 255 << 23;
    }
    // Sign bit
    o.u |= (in & 0x8000) << 16;
    return o.f;
}


// Convert value to float
static inline bool value_to_fp32(const void* src, float* dst, const TensorAttributes* src_attr)
{
    switch (src_attr->dtype) {
    case DataType::float32:
        *dst = *(float*)src;
        return true;
    case DataType::float16:
        *dst = fp16_to_float(*static_cast<const int16_t*>(src));
        return true;
    case DataType::int8:
    case DataType::uint8:
    case DataType::int16:
    case DataType::int32: {
        int32_t src_value = 0;
        if (!get_int32(src, src_attr->dtype, &src_value))
            return false;
        switch (src_attr->qi.scheme) {
        case QuantizationScheme::dynamic_fixed_point:
            *dst = dfp_to_float(src_value, src_attr->qi.fractional_length);
            return true;
        case QuantizationScheme::affine_asymmetric:
            *dst = affine_to_float(src_value, src_attr->qi.scale_factor, src_attr->qi.zero_point);
            return true;
        case QuantizationScheme::none:
            *dst = static_cast<float>(src_value);
            return true;
        }
        LOGE << "Conversion from unknown quantization scheme: " << src_attr->qi.scheme;
        return false;
    } break;
    case DataType::invalid:
    case DataType::byte:
    case DataType::uint16:
    case DataType::uint32:
        LOGE << "Conversion from dtype " << src_attr->dtype << " not implemented";
        return false;
    }
    LOGE << "Conversion from unknown data type: " << src_attr->dtype;
    return false;
}


// Convert (quantized) data buffer to float
bool dequantize(float* dst, const uint8_t* src, size_t size, const TensorAttributes* src_attr)
{
    bool success = true;
    size_t stride = synap_type_size(src_attr->dtype);
    for (int32_t j = 0; j < size; j++) {
        if (!value_to_fp32(&src[stride * j], &dst[j], src_attr)) {
            dst[j] = 0;
            success = false;
        }
    }

    if (!success) {
        LOGE << "Conversion to float failed";
    }
    return success;
}


//
// Data normalization and quantization
//


// Return info about number of channels and their stride
ChInfo get_channel_info(Layout layout, const Shape& shape)
{
    if (shape.size() == 4) {
        // Channels are meaningful only for 4D tensors
        switch(layout) {
        case Layout::none:
            break;
        case Layout::nchw:
            return { shape[1], shape[2] * shape[3] };
        case Layout::nhwc:
            return { shape[3], 1 };
        }
    }

    return {1, static_cast<int>(shape.item_count())};
}


// Return info about the mean for this tensor
MeanInfo get_mean_info(const TensorAttributes* attr)
{
    ChInfo ch = get_channel_info(attr->layout, attr->shape);
    const auto& mean = attr->mean;
    size_t mean_cnt = mean.size();
    if (mean_cnt > 1 && ch.count != mean_cnt) {
        LOGE << "Mean[] mismatch. Expected " << ch.count << " got:" << mean_cnt;
        return {};
    }
    bool has_mean = !all_of(begin(mean), end(mean), [](int v) { return v == 0; });
    bool per_channel = has_mean && any_of(begin(mean), end(mean), [&mean](int v) { return v != mean[0]; });
    return { true, has_mean, per_channel, ch };
}


// Convert 32bit float value to fp16
static inline uint16_t float_to_fp16(float in)
{
    uint32_t fp32 = *reinterpret_cast<uint32_t*>(&in);
    uint32_t sign = (fp32 & 0x80000000u) >> 16;
    uint32_t exponent = (fp32 & 0x7F800000u) >> 13;
    uint32_t mantissa = (fp32 & 0x007FE000u) >> 13;  // No rounding
    uint32_t fp16 = 0u;
    if (exponent >= 0x023c00u) {
        fp16 = sign | 0x7BFF;  // Don't round to infinity
    }
    else if (exponent <= 0x01c000u) {
        fp16 = sign;
    }
    else {
        exponent -= 0x01c000u;
        fp16 = sign | exponent | mantissa;
    }
    return fp16;
}


// Assign, normalize and convert data to fp16 tensor
template<typename S>
static bool assign_fp16(void* dst, const S* src, size_t size, const TensorAttributes* attr)
{
    Timer t;
    MeanInfo mi = get_mean_info(attr);
    if (!mi.valid) {
        return false;
    }
    uint16_t* dst_fp16 = static_cast<uint16_t*>(dst);
    float scale = attr->scale ? attr->scale : 1;
    const auto* src_end = &src[size];
    if (mi.has_mean) {
        size_t cnt = 0;
        while (src != src_end) {
            const float mean = attr->mean[(cnt++ / mi.ci.stride) % mi.ci.count];
            *dst_fp16++ = float_to_fp16((*src++ - mean) / scale);
        }
        LOGV << "Converted data to fp16 " << attr->dtype << " mean,scale in " << t;
    }
    else {
        while (src != src_end) {
            *dst_fp16++ = float_to_fp16(*src++ / scale);
        }
        LOGV << "Converted data to fp16 " << attr->dtype << " scale in " << t;
    }
    return true;
}


// Assign, normalize and convert data to fp32 tensor
template<typename S>
static bool assign_fp32(float* dst, const S* src, size_t size, const TensorAttributes* attr)
{
    Timer t;
    MeanInfo mi = get_mean_info(attr);
    if (!mi.valid) {
        return false;
    }
    float scale = attr->scale ? attr->scale : 1;
    const auto* src_end = &src[size];
    if (mi.has_mean) {
        size_t cnt = 0;
        while (src != src_end) {
            const float mean = attr->mean[(cnt++ / mi.ci.stride) % mi.ci.count];
            *dst++ = (*src++ - mean) / scale;
        }
        LOGV << "Converted data to float " << attr->dtype << " mean,scale in " << t;
    }
    else if (scale != 1){
        while (src != src_end) {
            *dst++ = *src++ / scale;
        }
        LOGV << "Converted data to float " << attr->dtype << " scale in " << t;
    }
    else {
         while (src != src_end) {
                *dst++ = *src++;
        }
        LOGV << "Converted data to float " << attr->dtype << " in " << t;
    }
    return true;
}


// Assign, normalize and quantize data to int8_t,uint8_t/affine tensor
template<typename T, typename S>
bool assign_affine(T* dst, const S* src, size_t size, const TensorAttributes* attr)
{
    // Combined normalize+quantize formula:
    //  qval = (val - mean[ch]) / (scale * qi.scale) + qi.zero_point
    // If mean[*] == qi.zero_point and (scale * qi.scale) == 1 no conversion is needed
    Timer t;
    MeanInfo mi = get_mean_info(attr);
    if (!mi.valid) {
        return false;
    }

    LOGV << "Converting data to affine attr->scale " << attr->scale << ", scale_factor: " << attr->qi.scale_factor;
    
    // Compute scale from scale and scale_factor
    float scale = (attr->scale ? attr->scale : 1) * (attr->qi.scale_factor? attr->qi.scale_factor : 1);
    constexpr float epsilon = 1.0 / 256;
    bool scale_is_one = scale > 1 - epsilon && scale < 1 + epsilon;
    auto end = &dst[size];
    if (mi.per_channel_mean) {
        if (!scale_is_one) {
            if (mi.ci.stride > 1) {
                // Note: this case seems to give results which are slighty different
                // from what we get with NPU normalization. To be checked.
                size_t cnt = 0;
                while(dst != end) {
                    const int mean = attr->mean[cnt++];
                    for (size_t s = 0; s < mi.ci.stride; s++) {
                        *dst++ = limit<T>(to_int((*src++ - mean) / scale + attr->qi.zero_point));
                    }
                }
                LOGV << "Converted data to affine " << attr->dtype << " smean[],scale(" << scale << "): " << t;
            }
            else {
                size_t cnt = 0;
                while(dst != end) {
                    const int mean = attr->mean[cnt++];
                    if (cnt == mi.ci.count) cnt = 0;
                    *dst++ = limit<T>(to_int((*src++ - mean) / scale + attr->qi.zero_point));
                }
                LOGV << "Converted data to affine " << attr->dtype << " mean[],scale(" << scale << "): " << t;
            }
        }
        else {
            if (mi.ci.stride > 1) {
                size_t cnt = 0;
                while(dst != end) {
                    const int bias = attr->mean[cnt++] - attr->qi.zero_point;
                    for (size_t s = 0; s < mi.ci.stride; s++) {
                        *dst++ = limit<T>(*src++ - bias);
                    }
                }
                LOGV << "Converted data to affine " << attr->dtype << " smean[]: " << t;
            }
            else {
                size_t cnt = 0;
                while(dst != end) {
                    const int bias = attr->mean[cnt++] - attr->qi.zero_point;
                    if (cnt == mi.ci.count) cnt = 0;
                    *dst++ = limit<T>(*src++ - bias);
                }
                LOGV << "Converted data to affine " << attr->dtype << " mean[]: " << t;
            }
        }
    }
    else {
        const int mean = attr->mean.empty() ? 0 : attr->mean[0];
        // LOGV << "Converting data to affine mean:" << mean << " zp: " << attr->qi.zero_point;
        if (!scale_is_one) {
            while(dst != end) {
                *dst++ = limit<T>(to_int((*src++ - mean) / scale + attr->qi.zero_point));
            }
            LOGV << "Converted data to affine " << attr->dtype << " mean,scale(" << scale << "): " << t;
        }
        else {
            // Compute bias from mean and zero_point
            int bias = mean - attr->qi.zero_point;
            if (bias) {
                while(dst != end) {
                    *dst++ = limit<T>(*src++ - bias);
                }
                LOGV << "Converted data to affine bias " << attr->dtype << ": " << t;
            }
            else {
                while(dst != end) {
                    *dst++ = *src++;
                }
                LOGV << "Converted data to affine " << attr->dtype << ": " << t;
            }
        }
    }
    return true;
}


// Assign, normalize and quantize data to int8_t,int16_t/dynamic_fixed_point tensor
template<typename T, typename S>
bool assign_dynfp(T* dst, const S* src, size_t size, const TensorAttributes* attr)
{
    Timer t;
    MeanInfo mi = get_mean_info(attr);
    if (!mi.valid) {
        return false;
    }

    float scale = attr->scale ? attr->scale : 1;
    auto end = &dst[size];
    if (mi.per_channel_mean) {
        if (mi.ci.stride > 1) {
            size_t cnt = 0;
            while(dst != end) {
                const int mean = attr->mean[cnt++];
                for (size_t s = 0; s < mi.ci.stride; s++) {
                    // Note: do not use to_int() to have the same result as NPU preprocessing (squeezenet_224_onnx_16bits_pp)
                    *dst++ = limit<T>(/*to_int*/(shift(*src++ - mean, attr->qi.fractional_length) / scale));
                }
            }
            LOGV << "Converted data to dynamic_fixed_point " << attr->dtype << " smean[],scale: " << t;
        }
        else {
            size_t cnt = 0;
            while(dst != end) {
                const int mean = attr->mean[cnt++];
                if (cnt == mi.ci.count) cnt = 0;
                *dst++ = limit<T>(to_int(shift(*src++ - mean, attr->qi.fractional_length) / scale));
            }
            LOGV << "Converted data to dynamic_fixed_point " << attr->dtype << " mean[],scale: " << t;
        }
    }
    else {
        const int mean = attr->mean.empty() ? 0 : attr->mean[0];
        if (scale != 1) {
            while(dst != end) {
                *dst++ = limit<T>(to_int(shift(*src++ - mean, attr->qi.fractional_length) / scale));
            }
            LOGV << "Converted data to dynamic_fixed_point " << attr->dtype << " mean,scale: " << t;
        }
        else {
            if (mean || attr->qi.fractional_length) {
                while(dst != end) {
                    *dst++ = limit<T>(shift(*src++ - mean, attr->qi.fractional_length));
                }
                LOGV << "Converted data to dynamic_fixed_point " << attr->dtype << " mean: " << t;
            }
            else {
                while(dst != end) {
                    *dst++ = *src++;
                }
                LOGV << "Converted data to dynamic_fixed_point " << attr->dtype << ": " << t;
            }
        }
    }
    return true;
}

template <typename S>
bool do_normalize_quantize(void* dst, const S* src, size_t size, const TensorAttributes* attr)
{
    switch(attr->dtype) {
    case DataType::byte:
        // Raw byte type doesn't support any conversion
        break;
    case DataType::uint8:
        switch(attr->qi.scheme) {
        case QuantizationScheme::none:
            // Plain uint8 is handled as affine quantized (with scale_factor 0 and zero_point 0)
        case QuantizationScheme::affine_asymmetric:
            return assign_affine(static_cast<uint8_t*>(dst), src, size, attr);
        case QuantizationScheme::dynamic_fixed_point:
            break;
        }
        break;
    case DataType::int8:
        switch(attr->qi.scheme) {
        case QuantizationScheme::none:
            break;
        case QuantizationScheme::affine_asymmetric:
            return assign_affine(static_cast<int8_t*>(dst), src, size, attr);
        case QuantizationScheme::dynamic_fixed_point:
            return assign_dynfp(static_cast<int8_t*>(dst), src, size, attr);
        }
        break;
    case DataType::int16:
        switch (attr->qi.scheme) {
        case QuantizationScheme::none:
        case QuantizationScheme::affine_asymmetric:
            break;
        case QuantizationScheme::dynamic_fixed_point:
            return assign_dynfp(static_cast<int16_t*>(dst), src, size, attr);
        }
        break;
    case DataType::float16:
        return assign_fp16(dst, src, size, attr);
    case DataType::float32:
        return assign_fp32(static_cast<float*>(dst), src, size, attr);
    default:
        break;
    }

    LOGE << "Unable to convert to tensor data type: " << attr->dtype;
    return false;
}


bool normalize_quantize(void* dst, const uint8_t* src, size_t size, const TensorAttributes* attr)
{
    return do_normalize_quantize(dst, src, size, attr);
}

bool normalize_quantize(void* dst, const int16_t* src, size_t size, const TensorAttributes* attr)
{
    return do_normalize_quantize(dst, src, size, attr);
}

bool normalize_quantize(void* dst, const float* src, size_t size, const TensorAttributes* attr)
{
    return do_normalize_quantize(dst, src, size, attr);
}


bool normalize_quantize_is_copy(const TensorAttributes* attr)
{
    MeanInfo mi = get_mean_info(attr);
    if (!mi.valid || mi.per_channel_mean) {
        // Each channel has a different mean, so normalization is needed for sure
        return false;
    }

    // If no quantization or not affine quantization, scale_factor and zero_point will be 0.
    // If no quantization or not fixed-point quantization, fractional_length will be 0.
    // So the check below works in all cases.
    float scale = (attr->scale ? attr->scale : 1) * (attr->qi.scale_factor? attr->qi.scale_factor : 1);
    constexpr float epsilon = 1.0 / 256;
    bool scale_is_one = scale > 1 - epsilon && scale < 1 + epsilon;
    int bias = (attr->mean.empty() ? 0 : attr->mean[0]) - attr->qi.zero_point;
    return scale_is_one && !bias && !attr->qi.fractional_length;
}


}  // namespace synap
}  // namespace synaptics
