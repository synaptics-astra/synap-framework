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
#include "synap/bundle_parser.hpp"

#include "json.hpp"

using namespace std;

namespace synaptics {
namespace synap {

using json = nlohmann::json;

// bundle json format:
// {
//   "inputs": [ { "subgraph": -1, "in": 0 } ],  --> model/fullgraph input tensors, used to check input tensor number
//   "outputs": [ { "subgraph": 1, "out": 0 } ], --> model/fullgraph output tensors, spcify where comes from with which tensor
//   "graph": [
//     {
//       "inputs": [ { "subgraph": -1, "in": 0 } ], --> subgraph idx=-1 means tensor from bundle input
//       "model": "out_0.onnx",
//       "meta": "out_0.json"
//     },
//     {
//       "inputs": [ { "subgraph": 0, "in": 0 } ], --> subgraph input tensor, spcify where comes from with which tensor
//       "model": "out_1.onnx",
//       "meta": "out_0.json"
//     }
//   ]
// }


void from_json(const json& j, BundleParser::TensorInfo& p)
{
    if (j.contains("subgraph"))
        j.at("subgraph").get_to(p.subgraph_index);
    if (j.contains("in")) {
        j.at("in").get_to(p.tensor_index);
        p.is_input = true;
    }
    if (j.contains("out"))
        j.at("out").get_to(p.tensor_index);
}

void from_json(const json& j, BundleParser::SubGraphInfo& p)
{
    if (j.contains("inputs")) {
        for (auto& info : j.at("inputs")) {
            p.inputs.push_back(info.get<BundleParser::TensorInfo>());
        }
    }
    if (j.contains("model"))
        j.at("model").get_to(p.model);
    if (j.contains("meta"))
        j.at("meta").get_to(p.meta);
}

void from_json(const json& j, BundleParser::GraphBundle& p)
{
    if (j.contains("inputs")) {
        for (auto& info : j.at("inputs")) {
            p.inputs.push_back(info.get<BundleParser::TensorInfo>());
        }
    }
    if (j.contains("outputs")) {
        for (auto& info : j.at("outputs")) {
            p.outputs.push_back(info.get<BundleParser::TensorInfo>());
        }
    }
    if (j.contains("graph")) {
        for (auto & info : j.at("graph")) {
            p.items.push_back(info.get<BundleParser::SubGraphInfo>());
        }
    }
}

bool BundleParser::parse(const std::string& info)
{
    try {
        json j = json::parse(info);
        if (j.contains("inputs")) {
            j["inputs"].get_to(_inputs);
        }
        if (j.contains("outputs")) {
            j["outputs"].get_to(_outputs);
        }
        if (j.contains("graph")) {
            j["graph"].get_to(_graph_info);
        }
        if (j.contains("parallel_limit")) {
            j.at("parallel_limit").get_to(_parallel_limit);
        }
    } catch (json::parse_error& e) {
        LOGE << " parse json failed : " << e.what();
        return false;
    }

    for (auto& i : _graph_info) {
        LOGV << " model: " << i.model << " meta: " << i.meta;
    }

    for (auto& i : _inputs) {
        LOGV << "graph inputs to graph: " << i.subgraph_index << " with input: " << i.tensor_index;
    }

    for (auto& o : _outputs) {
        LOGV << "graph outputs from graph: " << o.subgraph_index << " with output: " << o.tensor_index;
    }

    return true;
}

bool BundleParser::init(const string& filename)
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

    return parse(info_string);
}

bool BundleParser::init(const void* data, const size_t size)
{
    assert(data != nullptr);
    assert(size > 0);

    std::string info_string = std::string((const char*)data, size);
    return parse(info_string);
}


const std::vector<BundleParser::SubGraphInfo>& BundleParser::graph_info() const
{
    return _graph_info;
}

const std::vector<BundleParser::TensorInfo>& BundleParser::inputs() const
{
    return _inputs;
}

const std::vector<BundleParser::TensorInfo>& BundleParser::outputs() const
{
    return _outputs;
}

int BundleParser::parallel_limit() const
{
    return _parallel_limit;
}


}  // namespace synap
}  // namespace synaptics
