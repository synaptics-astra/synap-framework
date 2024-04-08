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

#include "synap/string_utils.hpp"
#include "synap/logging.hpp"
#include <sstream>
#include <stdlib.h>

using namespace std;

namespace synaptics {
namespace synap {
namespace format_parse {


string get_type(const string& s)
{
    auto pos = s.find_first_of(" \t=");
    return pos == string::npos || s[pos] != '=' ? s.substr(0, pos) : "";
}


size_t value_pos(const string& s, const char* key)
{
    string skey = string(key) + '=';
    auto pos = s.rfind(skey, 0);
    if (pos == string::npos) {
        skey = ' ' + skey;
        pos = s.find(skey);
    }
    return pos == string::npos ? string::npos : s.find_first_not_of(" \t", pos + skey.size());
}


int get_int(const string& s, const char* key, int default_value)
{
    auto pos = value_pos(s, key);
    return pos == string::npos ? default_value : atoi(&s[pos]);
}


float get_float(const string& s, const char* key, float default_value)
{
    auto pos = value_pos(s, key);
    return pos == string::npos ? default_value : atof(&s[pos]);
}

bool get_bool(const std::string& s, const char* key, bool default_value)
{
    auto pos = value_pos(s, key);
    return pos == string::npos ? default_value :
        atoi(&s[pos]) != 0 || s.substr(pos, 4) == "true";
}

string get_string(const std::string& s, const char* key, string default_value)
{
    auto pos = value_pos(s, key);
    return pos == string::npos ? default_value : s.substr(pos, s.find(" ", pos) - pos);
}


template<typename T>
struct StringParser {
    static T cvt(const char* str, char** endptr)
    {
        return strtol(str, endptr, 0);
    }
};


template<>
struct StringParser<float> {
    static float cvt(const char* str, char** endptr)
    {
        return strtod(str, endptr);
    }
};


template<typename T>
struct StringParser<vector<T>> {
    static vector<T> cvt(const char* str, char** endptr)
    {
        if (*str++ != '[') {
            return {};
        }
    
        // We use low level strtod() parsing as this is twice as fast as istringstream
        vector<T> v;
        char* new_cp = (char*)str;
        if (*new_cp != ']') {
            for(const char* cp = new_cp; ; cp = new_cp + 1) {
                v.push_back(StringParser<T>::cvt(cp, &new_cp));
                while(isspace(*new_cp)) new_cp++;
                if (*new_cp == ',') continue;
                if (*new_cp == ']') break;
                LOGE << "Array conversion error at position:" << new_cp - str << ": " << new_cp;
                v = {};
                break;
            }
        }
        if (endptr) {
            *endptr = new_cp + 1;
        }
        return v;
    }
};


template<typename T>
vector<T> get_list(const string& s, const char* key)
{
    auto pos = key? value_pos(s, key) : 0;
    return pos == string::npos? vector<T>{} : StringParser<vector<T>>::cvt(&s[pos], nullptr);
}


vector<float> get_floats(const string& s, const char* key)
{
    return get_list<float>(s, key);
}


vector<int> get_ints(const string& s, const char* key)
{
    return get_list<int>(s, key);
}


vector<vector<int>> get_ints2(const string& s, const char* key)
{
    return get_list<vector<int>>(s, key);
}


}  // namespace format_parse
}  // namespace synap
}  // namespace synaptics
