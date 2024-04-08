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

#include "synap/types.hpp"
#include "synap/logging.hpp"
#include "synap/string_utils.hpp"

using namespace std;

namespace synaptics {
namespace synap {


size_t Shape::item_count() const
{
    size_t c = 1;
    for (const auto& d : *this) {
        c *= d;
    }
    return c;
}

bool Shape::valid() const
{
    for (const auto& d : *this) {
        if (d <= 0) {
            return false;
        }
    }
    return true;
}

size_t synap_type_size(DataType synap_type)
{
    switch (synap_type) {
    case DataType::byte:
    case DataType::int8:
    case DataType::uint8:
        return 1;
    case DataType::int16:
    case DataType::uint16:
    case DataType::float16:
        return 2;
    case DataType::int32:
    case DataType::uint32:
    case DataType::float32:
        return 4;
    case DataType::invalid:
        break;
    }

    LOGE << "Invalid data type";
    return 0;
}


bool synap_type_is_integral(DataType synap_type)
{
    switch (synap_type) {
    case DataType::int8:
    case DataType::uint8:
    case DataType::int16:
    case DataType::uint16:
    case DataType::int32:
    case DataType::uint32:
        return true;
    case DataType::byte:
    case DataType::float16:
    case DataType::float32:
        return false;
    case DataType::invalid:
        break;
    }

    LOGE << "Invalid data type";
    return false;
}


Dimensions::Dimensions(const Shape& s, Layout layout)
{
    if (s.size() == 4) {
        *this = layout == Layout::nchw? Dimensions{s[0], s[2], s[3], s[1]} :
                layout == Layout::nhwc? Dimensions{s[0], s[1], s[2], s[3]} : Dimensions{};
    }
}


std::ostream& operator<<(std::ostream& os, const SynapVersion& v)
{
    os << v.major << "."<< v.minor << "."<< v.subminor;
    return os;
}


std::ostream& operator<<(std::ostream& os, const Shape& v)
{
    os << '[';
    for (int i = 0; i < v.size(); ++i) {
        os << (i ? ", " : "") << v[i];
    }
    os << ']';
    return os;
}


std::ostream& operator<<(std::ostream& os, const Dimensions& d)
{
    os << "{'n':" << d.n << ", 'h':" << d.h << ", 'w':" << d.w << ", 'c':" << d.c << '}';
    return os;
}


std::ostream& operator<<(std::ostream& os, const Layout& layout)
{
    switch (layout) {
    case Layout::none:
        os << "none";
        return os;
    case Layout::nchw:
        os << "nchw";
        return os;
    case Layout::nhwc:
        os << "nhwc";
        return os;
    }
    os << "Layout?";
    return os;
}


std::ostream& operator<<(std::ostream& os, const DataType& type)
{
    switch (type) {
    case DataType::invalid:
        os << "invalid";
        return os;
    case DataType::byte:
        os << "byte";
        return os;
    case DataType::int8:
        os << "int8";
        return os;
    case DataType::uint8:
        os << "uint8";
        return os;
    case DataType::int16:
        os << "int16";
        return os;
    case DataType::uint16:
        os << "uint16";
        return os;
    case DataType::int32:
        os << "int32";
        return os;
    case DataType::uint32:
        os << "uint32";
        return os;
    case DataType::float16:
        os << "float16";
        return os;
    case DataType::float32:
        os << "float32";
        return os;
    }
    os << "DataType?(" << static_cast<int>(type) << ")";
    return os;
}


std::ostream& operator<<(std::ostream& os, const QuantizationScheme& qs) {
    switch (qs) {
    case QuantizationScheme::none:
        os << "none";
        return os;
    case QuantizationScheme::dynamic_fixed_point:
        os << "dynamic_fixed_point";
        return os;
    case QuantizationScheme::affine_asymmetric:
        os << "affine_asymmetric";
        return os;
    }
    os << "QuantizationScheme?(" << static_cast<int>(qs) << ")";
    return os;
}


std::ostream& operator<<(std::ostream& os, const Dim2d& d)
{
    os << d.x << ',' << d.y;
    return os;
}


std::ostream& operator<<(std::ostream& os, const Landmark& l)
{
    os << l.x << ',' << l.y << ',' << l.z << ',' << l.visibility;
    return os;
}


std::ostream& operator<<(std::ostream& os, const Rect& r)
{
    os << r.origin << ',' << r.size;
    return os;
}


bool from_string(Rect& r, const std::string& str)
{
    auto v = format_parse::get_ints(str, nullptr);
    if (v.size() != 4) {
        LOGE << "Error converting string: " << str << "to Rect";
        return false;
    }
    r = {v[0], v[1], v[2], v[3]};
    return true;
}


}  // namespace synap
}  // namespace synaptics
