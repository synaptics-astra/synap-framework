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

#include "synap/file_utils.hpp"
#include "synap/logging.hpp"
#include "synap/label_info.hpp"

#include "json.hpp"

using namespace std;

namespace synaptics {
namespace synap {

using json = nlohmann::json;


LabelInfo::LabelInfo(const std::string& filename)
{
    if (!filename.empty())
        init(filename);
}


bool LabelInfo::init(const std::string& filename)
{
    if (!file_exists(filename)) {
        LOGE << "Info file not found: " << filename;
        return false;
    }
    std::string info_string = file_read(filename);
    if (info_string.empty()) {
        LOGE << "Failed to read info file: " << filename;
        return false;
    }

    try {
        json j = json::parse(info_string);
        j["labels"].get_to(_labels);
    }
    catch (json::parse_error& e)
    {
        LOGE << filename << ": " << e.what();
        return false;
    }
    return true;
}


const std::string& LabelInfo::label(int class_index) const
{
    static const string no_label;
    return class_index >= 0 && class_index < _labels.size() ? _labels[class_index] : no_label;
}


}  // namespace synap
}  // namespace synaptics
