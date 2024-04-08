/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
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
/// Synap Neural Network.
///

#pragma once
#include "synap/tensor.hpp"
#include <memory>
#include <string>

namespace synaptics {
namespace synap {

class NetworkPrivate;

/// Load and execute a neural network on the NPU accelerator.
class Network {
    // Implementation details
    std::unique_ptr<NetworkPrivate> d;

public:
    Network();
    Network(Network&& rhs);
    Network(const Network& rhs) = delete;
    Network& operator=(Network&& rhs);
    Network& operator=(const Network& rhs) = delete;
    ~Network();


    /// Load model.
    /// In case another model was previously loaded it is disposed before loading the one specified.
    ///
    /// @param model_file      path to .synap model file.
    ///                        Can also be the path to a legacy .nb model file.
    /// @param meta_file       for legacy .nb models must be the path to the model's metadata file
    ///                        (JSON-formatted). In all other cases must be the empty string.
    /// @return                true if success
    bool load_model(const std::string& model_file, const std::string& meta_file = "");


    /// Load model.
    /// In case another model was previously loaded it is disposed before loading the one specified.
    ///
    /// @param model_data      model data, as from e.g. fread() of model.synap
    ///                        The caller retains ownership of the model data and can delete them
    ///                        at the end of this method.
    /// @param model_size      model size in bytes
    /// @param meta_data       for legacy .nb models must be the model's metadata (JSON-formatted).
    ///                        In all other cases must be nullptr.
    /// @return                true if success
    bool load_model(const void* model_data, size_t model_size, const char* meta_data = nullptr);


    /// Run inference.
    /// Input data to be processed are read from input tensor(s).
    /// Inference results are generated in output tensor(s).
    ///
    /// @return true if success, false if inference failed or network not correctly initialized.
    bool predict();


    /// Collection of input tensors that can be accessed by index and iterated.
    Tensors inputs;

    /// Collection of output tensors that can be accessed by index and iterated.
    Tensors outputs;
};


/// Get synap version.
///
/// @return version number
SynapVersion synap_version();


}  // namespace synap
}  // namespace synaptics
