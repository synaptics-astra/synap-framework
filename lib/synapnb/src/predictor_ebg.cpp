#include "predictor_ebg.hpp"
#include "synap/allocator.hpp"
#include "synap/logging.hpp"
#include "synap_device.h"
#include "synap_driver.hpp"

using namespace std;

namespace synaptics {
namespace synap {



bool PredictorEBG::init()
{
    if (!synap_driver_available()) {
        LOGE << "Failed to initialize synap device";
        return false;
    }
    if (setenv("VSI_NN_LOG_LEVEL", "0", 0) != 0) {
        LOGW << "Failed to disable ovxlib log messages";
    }
    return true;
}


bool PredictorEBG::load_model(const void* model, size_t size, NetworkMetadata* meta)
{
    // Init NPU
    if (!init()) {
        return false;
    }

    // Check if this looks like an ebg
    if (!model || size < 4) {
        LOGE << "Invalid network model";
        return false;
    }
    auto ebg_magic = static_cast<const char*>(model);
    if (strncmp(ebg_magic, "VPMN", 4) == 0) {
        LOGE << "NBG models not supported anymore. Please recompile your model.";
        return false;
    }
    if (strncmp(ebg_magic, "EBGX", 4) != 0) {
        LOGE << "Not an EBG network model.";
        return false;
    }

    LOGV << "Preparing network nbg: size: " << size;
    if (!synap_prepare_network(model, size, &_network)) {
        LOGE << "Failed to prepare network";
        return false;
    }

    return true;
}

bool PredictorEBG::predict()
{
    LOGV << "Predicting...";
    if (_network && !synap_run_network(_network)) {
        LOGE << "run network failed";
        return false;
    }
    return true;
}

PredictorEBG::PredictorEBG()
{
    LOGV << "PredictorEBG: " << this;
}


PredictorEBG::~PredictorEBG()
{
    if (_network && !synap_release_network(_network)) {
        LOGE << "Failed to release network for PredictorEBG: " << this;
    }
}


BufferAttachment PredictorEBG::attach_buffer(Buffer* buffer, int32_t index, bool is_input)
{
    assert(buffer);
    BufferAttachment tensor_handle{};
    uint32_t bid = buffer->bid();
    LOGV << "Attaching io buffer with bid " << bid;

    if (!synap_attach_io_buffer(_network, bid, &tensor_handle)) {
        LOGE << "Failed to attach bid: " << bid;
        return {};
    }
    return tensor_handle;
}


bool PredictorEBG::set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle)
{
    if (!(is_input ? synap_set_input(_network, handle, index) :
                     synap_set_output(_network, handle, index))) {
        LOGE << "Failed to set " << (is_input ? "input" : "output") << " buffer " << index;
        return false;
    }
    return true;
}


bool PredictorEBG::detach_buffer(BufferAttachment handle)
{
    if (!synap_detach_io_buffer(_network, handle)) {
        LOGE << "Failed to detach io buffer";
        return false;
    }
    return true;
}


}  // namespace synap
}  // namespace synaptics
