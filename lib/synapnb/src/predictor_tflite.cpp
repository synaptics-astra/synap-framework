#include "predictor_tflite.hpp"
#include "synap/string_utils.hpp"

#include "synap/allocator.hpp"
#include "synap/logging.hpp"

#include "tensorflow/lite/logger.h"
#include "tensorflow/lite/delegates/gpu/delegate.h"
#include "tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h"

using namespace std;

namespace synaptics {
namespace synap {


PredictorTFLite::PredictorTFLite()
{
}


PredictorTFLite::~PredictorTFLite()
{
    _interpreter.reset();
    if (_xnnpack_delegate) TfLiteXNNPackDelegateDelete(_xnnpack_delegate);
    if (_gpu_delegate) TfLiteGpuDelegateV2Delete(_gpu_delegate);
}


bool PredictorTFLite::load_model(const void* model, size_t size, NetworkMetadata* meta)
{
    if (model == nullptr || size == 0) {
        LOGE << "TFLite model is empty";
        return false;
    }

    const bool enable_gpu = format_parse::get_bool(meta->delegate, "gpu");
    bool use_xnnpack = format_parse::get_bool(meta->delegate, "use_xnnpack", !enable_gpu);
    if (enable_gpu && use_xnnpack) {
        use_xnnpack = false;
        LOGE << "TFLite GPU and XNNPACK delegates cannot be used together, will use GPU";
    }

    // Copy model data since we will need it later to perform inference
    const char* model_data = static_cast<const char*>(model);
    _model.assign(model_data, model_data + size);

    int log_level = format_parse::get_int(meta->delegate, "log_level", -1);
    if (log_level >= 0) {
        LOGI << "PredictorTFLite delegate using log_level: " << log_level;
        tflite::LoggerOptions::SetMinimumLogSeverity(tflite::LogSeverity(log_level));
    }

    // Load Model and create interpreter
    _tensor_model = tflite::FlatBufferModel::BuildFromBuffer(_model.data(), _model.size());
    LOGV << "TFLite model created";

    tflite::ops::builtin::BuiltinOpResolver resolver;
    if (!use_xnnpack) {
        resolver = tflite::ops::builtin::BuiltinOpResolverWithoutDefaultDelegates();
    } else {
        resolver = tflite::ops::builtin::BuiltinOpResolver();
    }
    tflite::InterpreterBuilder builder(*_tensor_model.get(), resolver);

    int num_threads = format_parse::get_int(meta->delegate, "num_threads");
    if (num_threads >= 0) {
        LOGI << "PredictorTFLite delegate using num_threads: " << num_threads;
        if (builder.SetNumThreads(num_threads) != kTfLiteOk) {
            LOGE << "Failed to set thread number";
            return false;
        }
    }

    builder(&_interpreter);
    if (!_interpreter) {
        LOGE << "Failed to initiate TFLite interpreter";
        return false;
    }

    const bool allow_fp16 = format_parse::get_bool(meta->delegate, "allow_fp16");
    const bool latest_ops = format_parse::get_bool(meta->delegate, "latest_operators");

    // tflite apply xnnpack by default
    // explicitly create xnnpack delegate only when extra options configured
    if (use_xnnpack && (allow_fp16 || latest_ops)) {
        TfLiteXNNPackDelegateOptions xnnpack_options = TfLiteXNNPackDelegateOptionsDefault();

        if (allow_fp16) {
            xnnpack_options.flags |= TFLITE_XNNPACK_DELEGATE_FLAG_FORCE_FP16;
            LOGI << "enable XNNPACK FORCE_FP16";
        }

        // enable it with quantized fully connected models
        if (latest_ops) {
            xnnpack_options.flags |= TFLITE_XNNPACK_DELEGATE_FLAG_ENABLE_LATEST_OPERATORS;
            LOGI << "enable XNNPACK LATEST OPERATORS";
        }

        LOGV << "enable XNNPACK delegate";
        _xnnpack_delegate = TfLiteXNNPackDelegateCreate(&xnnpack_options);
        if (_interpreter->ModifyGraphWithDelegate(_xnnpack_delegate) != kTfLiteOk) {
            // Report error and fall back to the default backend
            LOGE << "TFLite failed setting CPU XNNPACK delegate";
        }
    }

    if (enable_gpu) {
        TfLiteGpuDelegateOptionsV2 options = TfLiteGpuDelegateOptionsV2Default();
        options.is_precision_loss_allowed = allow_fp16;

        options.inference_preference = format_parse::get_int(
            meta->delegate, "gpu_inference_preference", TFLITE_GPU_INFERENCE_PREFERENCE_SUSTAINED_SPEED);

        options.inference_priority1 = format_parse::get_int(
            meta->delegate, "gpu_inference_priority1", TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY);
        options.inference_priority2 = format_parse::get_int(
            meta->delegate, "gpu_inference_priority1", TFLITE_GPU_INFERENCE_PRIORITY_AUTO);
        options.inference_priority3 = format_parse::get_int(
            meta->delegate, "gpu_inference_priority1", TFLITE_GPU_INFERENCE_PRIORITY_AUTO);

        LOGI << "TFLite GPU options, preference: " << options.inference_preference
             << " priorities: " << options.inference_priority1 << ", "
             << options.inference_priority2 << ", " << options.inference_priority3 << "\n"
             << "precision loss allowed: " << options.is_precision_loss_allowed;

        const string cache_dir = format_parse::get_string(meta->delegate, "cache_dir");
        const string model_token = format_parse::get_string(meta->delegate, "model_token");
        if (!cache_dir.empty() && !model_token.empty()) {
            LOGV << "GPU delegate cache_dir: " << cache_dir << " model_token: " << model_token;
            options.experimental_flags |= TFLITE_GPU_EXPERIMENTAL_FLAGS_ENABLE_SERIALIZATION;
            options.serialization_dir = cache_dir.c_str();
            options.model_token = model_token.c_str();
        }

        _gpu_delegate = TfLiteGpuDelegateV2Create(&options);

        if (_interpreter->ModifyGraphWithDelegate(_gpu_delegate) != kTfLiteOk) {
            LOGE << "TFLite failed setting GPU delegate";
        } else {
            LOGI << "TFLite using GPU delegate";
        }
    }

    if (_interpreter->AllocateTensors() != kTfLiteOk) {
        LOGE << "Failed to allocate tensor";
        return false;
    }

    return true;
}


bool PredictorTFLite::predict()
{
    if (_interpreter->Invoke() != kTfLiteOk) {
        LOGE << "TFLite Invoke failed";
        return false;
    }

    LOGV << "TFLite Invoke success";
    return true;
}


BufferAttachment PredictorTFLite::attach_buffer(Buffer* buffer, int32_t index, bool is_input)
{
    return 1;
}


bool PredictorTFLite::set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle)
{
    TfLiteCustomAllocation tf_alloc{buffer->data(), buffer->size()};
    TfLiteStatus status;
    if (is_input) {
        status = _interpreter->SetCustomAllocationForTensor(_interpreter->inputs()[index], tf_alloc);
    }
    else {
        status = _interpreter->SetCustomAllocationForTensor(_interpreter->outputs()[index], tf_alloc);
    }

    if (status != kTfLiteOk) {
        LOGE << "TFLite failed setting custom tensor allocation";
        return false;
    }

    return true;
}


bool PredictorTFLite::detach_buffer(BufferAttachment handle)
{
    return true;
}

}  // namespace synap
}  // namespace synaptics
