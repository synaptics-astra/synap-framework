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
///
/// Bundle information
///

#pragma once

#include <string>
#include <vector>


namespace synaptics {
namespace synap {


/// Bundle information
class BundleParser {
public:

    struct TensorInfo {
        int subgraph_index{};
        bool is_input{};
        size_t tensor_index{};
    };

    struct SubGraphInfo {
        std::vector<TensorInfo> inputs;

        // graph and meta file path
        std::string model;
        std::string meta;

        // graph data and meta data in memory
        std::vector<uint8_t> model_data;
        std::string meta_data;
    };

    struct GraphBundle {
        std::vector<TensorInfo> inputs;
        std::vector<TensorInfo> outputs;
        std::vector<SubGraphInfo> items;
    };

    ///  Constructor.
    ///
    BundleParser() {}
    virtual ~BundleParser() {}

    ///  init with file.
    ///
    ///  @param data: data containing bundle information
    virtual bool init(const std::string& filename);

    ///  init with a data buffer.
    ///
    ///  @param data: data containing bundle information
    virtual bool init(const void* data, const size_t size);

    const std::vector<SubGraphInfo>& graph_info() const;

    const std::vector<TensorInfo>& inputs() const;

    const std::vector<TensorInfo>& outputs() const;

    int parallel_limit() const;

protected:
    std::vector<SubGraphInfo> _graph_info;
    std::vector<TensorInfo> _inputs;
    std::vector<TensorInfo> _outputs;
    int _parallel_limit{};

private:
    bool parse(const std::string& info);
};

}  // namespace synap
}  // namespace synaptics
