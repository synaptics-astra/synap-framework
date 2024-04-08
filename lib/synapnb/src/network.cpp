/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2023 Synaptics Incorporated. All rights reserved.
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

#include "predictor.hpp"
#include "synap/metadata.hpp"
#include "synap/network.hpp"

#include "synap/file_utils.hpp"
#include "synap/string_utils.hpp"
#include "synap/logging.hpp"
#include "synap/timer.hpp"

#include "buffer_private.hpp"
#include "network_private.hpp"
#ifdef SYNAP_EBG_ENABLE
#include "predictor_ebg.hpp"
#endif
#ifdef SYNAP_ONNX_ENABLE
#include "predictor_onnx.hpp"
#endif
#ifdef SYNAP_TFLITE_ENABLE
#include "predictor_tflite.hpp"
#endif
#include "predictor_bundle.hpp"


using namespace std;

namespace synaptics {
namespace synap {


//
// NetworkPrivate
//


bool NetworkPrivate::load_model_file(const std::string& model_file, const std::string& meta_file)
{
    bool have_metafile = !meta_file.empty() && meta_file != "-";
    if (directory_exists(model_file)) {
        // Directory containing an unzipped bundle model
        if (have_metafile) {
            LOGE << "Metafile cannot be specified when model is a directory";
            return false;
        }
        return load_model_data(nullptr, 0, string("bundle dir=" + model_file).c_str());
    }

    std::string model_extension = filename_extension(model_file);
    bool is_synap_model = model_extension == "synap";
    if (is_synap_model && have_metafile) {
        LOGE << "SyNAP models don't require a metafile";
        return false;
    }
    else if (!is_synap_model && !have_metafile) {
        LOGE << "Metafile required for non-SyNAP models";
        return false;
    }

    vector<uint8_t> model_data = binary_file_read(model_file);

    if (model_data.empty()) {
        LOGE << "Failed to load model: " << model_file;
        return false;
    }
    const char* model_meta_data_ptr = nullptr;
    string model_meta_data;
    if (have_metafile) {
        model_meta_data = file_read(meta_file);
        if (model_meta_data.empty()) {
            LOGE << "Failed to load metafile: " << meta_file;
            return false;
        }
        model_meta_data_ptr = model_meta_data.c_str();
    }

    return load_model_data(model_data.data(), model_data.size(), model_meta_data_ptr);
}


static unique_ptr<Predictor> make_predictor(const string& delegate)
{
    LOGI << "Creating delegate "  << delegate;
    if (delegate == "bundle") {
        return make_unique<PredictorBundle>();
    }
#ifdef SYNAP_EBG_ENABLE
    if (delegate == "npu") {
        return make_unique<PredictorEBG>();
    }
#endif
#ifdef SYNAP_ONNX_ENABLE
    if (delegate == "onnx") {
        return make_unique<PredictorONNX>();
    }
#endif
#ifdef SYNAP_TFLITE_ENABLE
    if (delegate == "tflite") {
        return make_unique<PredictorTFLite>();
    }
#endif

    LOGE << "Delegate " << delegate << " not supported";
    return {};
}


bool NetworkPrivate::load_model_data(const void* data, size_t data_size, const char* meta_data)
{
    // Remove current predictor instance if any
    unregister_buffers();
    _predictor.reset();

    // If no metafile given use a Bundle predictor by default
    NetworkMetadata meta;
    meta.delegate = "bundle";
    Timer tmr;

    if (!data) {
        // Try to get delegate string directly from meta_data
        if (data_size || format_parse::get_type(meta_data) != "bundle") {
            LOGE << "Null model_data pointer";
            return false;
        }
        meta.delegate = meta_data;
    }
    else if (meta_data) {
        meta = load_metadata(meta_data);
        if (!meta.valid) {
            LOGE << "Failed to load metadata";
            return false;
        }
    }

    // Create new predictor according to the selected delegate
    string delegate = format_parse::get_type(meta.delegate);
    _predictor = make_predictor(delegate);
    if (!_predictor) {
        LOGE << "Failed to create delegate: "  << delegate;
        return false;
    }

    // Load model and check/update meta info
    if (!_predictor->load_model(data, data_size, &meta)) {
        LOGE << "Failed to load model";
        _predictor = nullptr;
        return false;
    }

    LOGI << "Loaded Network (" << meta.delegate << "): "  << tmr;
    LOGI << "Network inputs: " << meta.inputs.size();
    LOGI << "Network outputs: " << meta.outputs.size();

    // Create input and output tensors
    _inputs = create_tensors(Tensor::Type::in, meta.inputs);
    _outputs = create_tensors(Tensor::Type::out, meta.outputs);

    return true;
}


vector<Tensor> NetworkPrivate::create_tensors(Tensor::Type ttype, const vector<TensorAttributes>& tattrs)
{
    vector<Tensor> tensors;
    tensors.reserve(tattrs.size());
    for (int32_t i = 0; i < tattrs.size(); i++) {
        Tensor* t = _predictor->get_tensor(i, ttype == Tensor::Type::in);
        if (t) {
            // Create a reference to the Tensor provided by the Predictor
            tensors.emplace_back(*t);
        }
        else {
            // Default Tensor creation according to attributes
            tensors.emplace_back(this, i, ttype, &tattrs[i]);
        }
    }
    return tensors;
}


bool NetworkPrivate::do_predict()
{
    if (!_predictor) {
        LOGE << "Network not correctly initialized";
        return false;
    }

    // Check that input buffers have been assigned
    for (auto& in_tensor : _inputs) {
      auto buffer = in_tensor.buffer();
      if (!buffer || !buffer->size() || !in_tensor.set_buffer(buffer)) {
            LOGE << "Input data error for tensor " << in_tensor.name();
            return false;
        }
    }

    // Be sure output buffers are allocated and assigned
    for (auto& out_tensor : _outputs) {
      out_tensor.data();
      auto buffer = out_tensor.buffer();
      if (!buffer || !buffer->size() || !out_tensor.set_buffer(buffer)) {
            LOGE << "Output buffer error for tensor: " << out_tensor.name();
            return false;
        }
    }

    LOGI << "Start inference";
    Timer tmr;

    // Flush cache for inputs
    for (auto& t : _inputs) {
        if (!t.buffer()->priv()->cache_flush()) {
            LOGE << "Cache flush failed for input: " << t.name();
            return false;
        }
    }

    bool success = _predictor->predict();
    LOGI << "Inference time: " << tmr;
    if (!success) {
        LOGE << "Inference failed";
        return false;
    }

    // Invalidate cache for outputs
    for (auto& t : _outputs) {
        if (!t.buffer()->priv()->cache_invalidate()) {
            LOGE << "Cache invalidate failed for output: " << t.name();
            return false;
        }
    }

    return true;
}


bool NetworkPrivate::register_buffer(Buffer* buffer, size_t index, bool is_input)
{
    BufferAttachment handle = buffer->priv()->handle(this);
    if (!handle) {
        handle = _predictor->attach_buffer(buffer, index, is_input);
        if (!handle) {
            LOGE << "Can't create buffer handle for " << (is_input ? "input " : "output ") << index;
            return false;
        }
        LOGV << "Created buffer handle for " << buffer << " : " << handle;

        _buffers.insert(buffer);
        buffer->priv()->register_network(this, handle);
    }

    // Set this as current buffer
    return _predictor->set_buffer(buffer, index, is_input, handle);
}


bool NetworkPrivate::unregister_buffer(Buffer* buffer)
{
    if (_buffers.find(buffer) == _buffers.end()) {
        LOGE << "Buffer not registered " << buffer;
        return false;
    }
    _buffers.erase(buffer);

    // Be sure it is not currently used as input or output buffer
    for (auto& t : _inputs) {
        if (buffer == t.buffer()) {
            LOGI << "Detaching buffer from input tensor " << t.name();
            t.set_buffer(nullptr);
        }
    }
    for (auto& t : _outputs) {
        if (buffer == t.buffer()) {
            LOGI << "Detaching buffer from output tensor " << t.name();
            t.set_buffer(nullptr);
        }
    }

    BufferAttachment buffer_handle = buffer->priv()->handle(this);
    if (!buffer_handle) {
        LOGE << "Invalid buffer handle " << buffer;
        return false;
    }

    buffer->priv()->unregister_network(this);
    if (!_predictor->detach_buffer(buffer_handle)) {
        LOGE << "Error deleting buffer handle " << buffer;
        return false;
    }

    return true;
}


void NetworkPrivate::unregister_buffers()
{
    // Unregister all associated buffers
    while (!_buffers.empty()) {
        unregister_buffer(*_buffers.begin());
    }
}


//
// Network
//


Network::Network() : d{new NetworkPrivate()}, inputs(d->_inputs), outputs(d->_outputs) {}


Network::Network(Network&& rhs) : Network()
{
    *this = std::move(rhs);
}


Network& Network::operator=(Network&& rhs)
{
    swap(d, rhs.d);
    inputs.swap(rhs.inputs);
    outputs.swap(rhs.outputs);
    return *this;
}


Network::~Network()
{
    d->unregister_buffers();
}

bool Network::load_model(const std::string& model_file, const std::string& meta_file)
{
    return d->load_model_file(model_file, meta_file);
}

bool Network::load_model(const void* data, size_t data_size, const char* meta_data)
{
      return d->load_model_data(data, data_size, meta_data);
}

bool Network::predict()
{
    return d->do_predict();
}

SynapVersion synap_version()
{
    return {3, 0, 0};
}


}  // namespace synap
}  // namespace synaptics
