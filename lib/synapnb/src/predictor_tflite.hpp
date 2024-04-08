#pragma once

#include "predictor.hpp"

#include "synap/buffer.hpp"

#define CL_DELEGATE_NO_GL 1
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>

#include <cstdint>
#include <stddef.h>
#include <vector>

namespace synaptics {
namespace synap {

/// Predictor
class PredictorTFLite : public Predictor {
public:
    PredictorTFLite();
    ~PredictorTFLite();

    bool load_model(const void* model, size_t size, NetworkMetadata* meta) override;
    bool predict() override;
    BufferAttachment attach_buffer(Buffer* buffer, int32_t index, bool is_input) override;
    bool set_buffer(Buffer* buffer, int32_t index,  bool is_input, BufferAttachment handle) override;
    bool detach_buffer(BufferAttachment handle) override;

private:
    std::unique_ptr<tflite::Interpreter> _interpreter{};
    std::unique_ptr<tflite::FlatBufferModel> _tensor_model{};
    std::vector<char> _model{};

    TfLiteDelegate* _xnnpack_delegate{};
    TfLiteDelegate* _gpu_delegate{};
};

}  // namespace synap
}  // namespace synaptics
