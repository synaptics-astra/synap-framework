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
#include "synap/bundle_parser_zip.hpp"
#include "synap/zip_tool.hpp"

using namespace std;

namespace synaptics {
namespace synap {


bool BundleParserZip::init(const void* data, const size_t size)
{
    ZipTool ztool;
    if (!ztool.open(data, size)) {
        LOGE << "Failed to open zip archive";
        return false;
    }

    vector<uint8_t> bundle_info = ztool.extract_archive("bundle.json");
    if (bundle_info.empty() || !BundleParser::init(bundle_info.data(), bundle_info.size())) {
        LOGE << "Failed to parse model bundle information";
        return false;
    }

    for (auto& gi: _graph_info) {
        vector<uint8_t> meta_data = ztool.extract_archive(gi.meta);
        gi.meta_data = string(meta_data.begin(), meta_data.end());
        if (gi.meta_data.empty()) {
            LOGE << "Failed to extract meta data for graph: " << gi.meta;
            return false;
        }

        gi.model_data = ztool.extract_archive(gi.model);
        if (gi.model_data.empty()) {
            LOGE << "Failed to extract model data for graph: " << gi.model;
            return false;
        }
    }

    return true;
}


}  // namespace synap
}  // namespace synaptics
