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

#include "synap/bundle_parser_dir.hpp"

#include "synap/file_utils.hpp"
#include "synap/logging.hpp"

#include <filesystem>

using namespace std;

namespace synaptics {
namespace synap {

namespace fs = std::filesystem;

static string get_absolute_path(const string& parent_path, const string& filename)
{
    fs::path tmp(filename);
    if (tmp.is_relative()) {
        return parent_path + "/" + filename;
    }
    return filename;
}

bool BundleParserDir::init(const std::string& filename)
{
    if (!BundleParser::init(filename)) {
        LOGE << "Failed to parse info file: " << filename;
        return false;
    }

    auto absolute_json_path = fs::canonical(filename);
    string cur_parent_path = absolute_json_path.parent_path();

    for (auto& i : _graph_info) {
        i.model = get_absolute_path(cur_parent_path, i.model);
        i.meta = get_absolute_path(cur_parent_path, i.meta);

        LOGV << " model: " << i.model << " meta: " << i.meta;
    }

    return true;
}


}  // namespace synap
}  // namespace synaptics
