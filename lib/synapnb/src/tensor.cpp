/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2020 Synaptics Incorporated. All rights reserved.
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
/// Synap tensor.
///

#include "network_private.hpp"
#include "quantization.hpp"

#include "synap/logging.hpp"
#include "synap/metadata.hpp"
#include "synap/string_utils.hpp"
#include "synap/tensor.hpp"
#include "synap/timer.hpp"

#include <cmath>


using namespace std;

namespace synaptics {
namespace synap {


struct Tensor::Private
{
    // Associated network
    NetworkPrivate* _np{};

    // Owning tensor.
    // This private representation can be shared between multiple (aliased) tensors, the owner
    // is the one actually responsible of deleting this internal Private struct.
    Tensor* _owner;

    // Tensor index
    int32_t _index;

    // Tensor in/out type
    Type _type{};

    // Tensor attributes
    unique_ptr<const TensorAttributes> _attr{};

    // Default buffer (used if no external buffer assigned to the tensor)
    Buffer _default_buffer{};

    // Current data buffer if any
    Buffer* _buffer{&_default_buffer};

    // Current data buffer set to the network if any.
    // Always equivalent to _buffer except at the beginning when it is null.
    Buffer* _set_buffer{};

    // Contains dequantized data if dequantization not done by the network itself
    std::vector<float> _dequantized_data;

    // Scaling factor to apply when assigning scalar tensors
    double _scalar_scale{};

    // Contains siblings tensors to propagate buffer changes
    std::vector<Tensor*> _siblings;
};


//
// Tensor
//

Tensor::Tensor(NetworkPrivate* np, int32_t ix, Type ttype, const TensorAttributes* attr): d{new Private}
{
    d->_np = np;
    d->_index = ix;
    d->_type = ttype;
    d->_attr.reset(new TensorAttributes(*attr));
    d->_owner = this;
    if (is_scalar()) {
        // Extract scaling factor for cropping tensors
        auto dim = format_parse::get_int(d->_attr->format, "tensor_dim", 0);
        if (dim) {
            d->_scalar_scale = 32768.0 / dim;
        }
    }
}

// Copy constructor.
// This creates an aliased tensor referring to the original one.
Tensor::Tensor(const Tensor& rhs)
{
    // Let's point to the same internal representation as the original tensor.
    d.reset(rhs.d.get());
}


Tensor::Tensor(Tensor&& rhs) noexcept
{
    LOGE << "Tensor move not allowed";
}


Tensor::~Tensor()
{
    if (d->_owner != this) {
        // Avoid deleting the internal representation if we are not the acutal owner
        d.release();
    }
}

const std::string& Tensor::name() const
{
    return d->_attr->name;
}


const Shape& Tensor::shape() const
{
    return d->_attr->shape;
}


const Dimensions Tensor::dimensions() const
{
    return Dimensions(d->_attr->shape, d->_attr->layout);
}


Layout Tensor::layout() const
{
    return d->_attr->layout;
}


string Tensor::format() const
{
    return d->_attr->format;
}


DataType Tensor::data_type() const
{
    return d->_attr->dtype;
}


Security Tensor::security() const
{
    return d->_attr->security;
}


size_t Tensor::size() const
{
    return item_count() * synap_type_size(d->_attr->dtype);
}


size_t Tensor::item_count() const {
    return d->_attr->shape.item_count();
}


bool Tensor::is_scalar() const
{
    return d->_attr->shape.size() == 1 && d->_attr->shape[0] == 1;
}


bool Tensor::set_buffer(Buffer* buffer)
{
    if (!buffer) {
        LOGI << "Unset buffer for: " << name();
        d->_buffer = d->_set_buffer = nullptr;
        return true;
    }
    if (buffer == d->_set_buffer) {
        LOGI << "Reassigning same buffer, nothing to do: " << name();
        return true;
    }
    if (buffer->size() == 0) {
        if (!buffer->resize(size())) {
            LOGE << "Unable to resize buffer for tensor " << name() << " size: " << size();
            return false;
        }
    }
    else if (buffer->size() != size()) {
        LOGE << "Bad buffer size for " << name() << ". Expected : " << size()
             << ", got: " << buffer->size();
        return false;
    }

    bool success{};
    switch (d->_type) {
    case Type::in:
        success = d->_np->register_buffer(buffer, d->_index, true);
        break;
    case Type::out:
        success = d->_np->register_buffer(buffer, d->_index, false);
        break;
    case Type::none:
        break;
    }

    // Propagate buffer change to our siblings if any
    for (Tensor* t : d->_siblings) {
        success &= t->set_buffer(buffer);
    }

    if (!success) {
        LOGE << "Failed to set buffer for tensor: " << name();
        return false;
    }

    LOGI << "Buffer set for tensor: " << name();
    d->_set_buffer = d->_buffer = buffer;

    return success;
}


Buffer* Tensor::buffer()
{
    return d->_buffer;
}


void* Tensor::data()
{
    if (d->_buffer == &d->_default_buffer && d->_buffer->size() == 0) {
        // Automatically allocate memory for default buffer
        d->_buffer->resize(size());
    }
    return d->_buffer ? d->_buffer->data() : nullptr;
}


const void* Tensor::data() const
{
    return d->_buffer ? d->_buffer->data() : nullptr;
}


bool Tensor::assign(int32_t value)
{
    if (!is_scalar()) {
        LOGE << "Not a scalar tensor: " << d->_attr->shape;
        return false;
    }
    if (d->_scalar_scale) {
        value = round(value * d->_scalar_scale);
        LOGV << "Scalar value rescaled to: " << value;
    }
    DataType synap_type = data_type();
    if (synap_type_is_integral(synap_type)) {
        const void* value_ptr = &value;
        return assign(value_ptr, synap_type_size(synap_type));
    }
    LOGE << "Unable to assign value to tensor of type: " << synap_type;
    return false;
}


bool Tensor::assign(const void* data, size_t sz)
{
    if (!d->_buffer) {
        LOGE << "Tensor has no associated buffer";
        return false;
    }
    if (sz != size()) {
        LOGE << "Bad data size. Expected: " << size() << ", got: " << sz;
        return false;
    }
    return d->_buffer->assign(data, sz);
}


static bool assignable(Tensor* t, size_t count) {
    if (!t->buffer()) {
        LOGE << "Tensor has no associated buffer";
        return false;
    }
    if (count != t->item_count()) {
        LOGE << "Bad data count. Expected: " << t->item_count() << ", got: " << count;
        return false;
    }
    return true;
}


bool Tensor::assign(const uint8_t* in_data, size_t count)
{
    return assignable(this, count) && normalize_quantize(data(), in_data, count, d->_attr.get());
}


bool Tensor::assign(const int16_t* in_data, size_t count)
{
    return assignable(this, count) && normalize_quantize(data(), in_data, count, d->_attr.get());
}


bool Tensor::assign(const float* in_data, size_t count)
{
    return assignable(this, count) && normalize_quantize(data(), in_data, count, d->_attr.get());
}


bool Tensor::assign(const Tensor& src) {
    if (!d->_buffer) {
        LOGE << "Tensor has no associated buffer";
        return false;
    }
    if (!src.d->_buffer) {
        LOGE << "Source tensor has no associated buffer";
        return false;
    }
    if (src.size() != size()) {
        LOGE << "Bad data size. Expected: " << size() << ", got: " << src.size();
        return false;
    }
    if (src.data_type() != data_type()) {
        LOGE << "Bad data type. Expected: " << data_type() << ", got: " << src.data_type();
        return false;
    }
    if (!d->_buffer->resize(size())) {
        LOGE << "Unable to resize tensor buffer";
        return false;
    }
    memcpy(data(), src.data(), size());
    return true;
}


// Convert C++ type to SyNAP DataType enum value
template <typename T> struct ToDataType { static constexpr DataType data_type{DataType::invalid}; };
template <> struct ToDataType<int8_t> { static constexpr DataType data_type{DataType::int8}; };
template <> struct ToDataType<uint8_t> { static constexpr DataType data_type{DataType::uint8}; };
template <> struct ToDataType<int16_t> { static constexpr DataType data_type{DataType::int16}; };
template <> struct ToDataType<uint16_t> { static constexpr DataType data_type{DataType::uint16}; };
template <> struct ToDataType<int32_t> { static constexpr DataType data_type{DataType::int32}; };
template <> struct ToDataType<uint32_t> { static constexpr DataType data_type{DataType::uint32}; };
template <> struct ToDataType<float> { static constexpr DataType data_type{DataType::float32}; };


template <typename T> T* Tensor::data()
{
    return data_type() == ToDataType<T>::data_type && normalize_quantize_is_copy(d->_attr.get()) ? 
        static_cast<T*>(data()) : nullptr;
}

template <typename T> const T* Tensor::data() const
{
    return data_type() == ToDataType<T>::data_type && normalize_quantize_is_copy(d->_attr.get()) ?
        static_cast<const T*>(data()) : nullptr;
}

template <> void* Tensor::data()
{
    return normalize_quantize_is_copy(d->_attr.get()) ? data() : nullptr;
}

template <> const void* Tensor::data() const
{
    return normalize_quantize_is_copy(d->_attr.get()) ? data() : nullptr;
}


const float* Tensor::as_float() const
{
    const void* raw_data = data();
    if (!raw_data) {
        LOGE << "Tensor contains no data";
        return nullptr;
    }

    if (data_type() == DataType::float32) {
        // Nothing to do, tensor is already in float
        return static_cast<const float*>(raw_data);
    }

    // Dequantize data
    auto raw_data_bytes = static_cast<const uint8_t*>(raw_data);
    size_t n = item_count();
    d->_dequantized_data.resize(n);
    dequantize(d->_dequantized_data.data(), raw_data_bytes, n, d->_attr.get());
    return d->_dequantized_data.data();
}


void Tensor::add_sibling(Tensor* t)
{
    d->_siblings.push_back(t);
}


//
// Tensors
//

const Tensor& Tensors::operator[](size_t index) const
{
    if (index >= _tensors->size()) {
        LOGE << "Invalid tensor index " << index << " of " << _tensors->size();
    }
    return (*_tensors).at(index);
}


//
// Explicitly instantiate the required data assign/access template methods
//

#define INSTANTIATE_DATA_METHODS(T) \
    template T* Tensor::data<T>(); \
    template const T* Tensor::data<T>() const;

INSTANTIATE_DATA_METHODS(int8_t)
INSTANTIATE_DATA_METHODS(uint8_t)
INSTANTIATE_DATA_METHODS(int16_t)
INSTANTIATE_DATA_METHODS(uint16_t)
INSTANTIATE_DATA_METHODS(int32_t)
INSTANTIATE_DATA_METHODS(uint32_t)
INSTANTIATE_DATA_METHODS(float)



}  // namespace synap
}  // namespace synaptics
