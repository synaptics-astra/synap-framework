#pragma once

#include "predictor.hpp"
#include "synap/tensor.hpp"

#include <onnxruntime/onnxruntime_cxx_api.h>

#include <cstdint>
#include <string>
#include <vector>

namespace synaptics {
namespace synap {

class NetworkPrivate;

/// PredictorONNX
class PredictorONNX: public Predictor {
public:
    PredictorONNX();
    ~PredictorONNX();

    bool load_model(const void* model, size_t size, NetworkMetadata* meta) override;

    bool predict() override;

    BufferAttachment attach_buffer(Buffer* buffer, int32_t index, bool is_input) override;
    bool set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle) override;
    bool detach_buffer(BufferAttachment handle) override;

private:
    Ort::Env _ort_env{};
    Ort::SessionOptions _session_options{};
    std::unique_ptr<Ort::Session> _session{};

    std::vector<Ort::Value> _input_tensors;
    std::vector<std::string> _input_names;
    std::vector<std::vector<std::int64_t>> _input_shapes;

    std::vector<Ort::Value> _output_tensors;
    std::vector<std::string> _output_names;
    std::vector<std::vector<std::int64_t>> _output_shapes;

};

}  // namespace synap
}  // namespace synaptics
