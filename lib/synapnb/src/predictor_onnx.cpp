#include "predictor_onnx.hpp"
#include "synap/string_utils.hpp"

#include "synap/logging.hpp"
#include "synap/metadata.hpp"
#include "synap/timer.hpp"

using namespace std;

namespace synaptics {
namespace synap {

bool PredictorONNX::load_model(const void* model, size_t size, NetworkMetadata* meta)
{
    if (!model || size <= 0) {
        LOGE << "invalid model data or size";
        return false;
    }

    LOGV << "Preparing network onnxruntime";

    int log_level = format_parse::get_int(meta->delegate, "log_level", -1);
    if (log_level >= 0) {
        LOGI << "PredictorONNX delegate using log_level: " << log_level;
        _ort_env.UpdateEnvWithCustomLogLevel(OrtLoggingLevel(log_level));
    }
    int num_threads = format_parse::get_int(meta->delegate, "num_threads", -1);
    if (num_threads >= 0) {
        // https://onnxruntime.ai/docs/performance/tune-performance/threading.html
        // Just set intra op num threads for now. Keep inter op num threads as default (sequential)
        LOGI << "PredictorONNX delegate using num_threads: " << num_threads;
        _session_options.SetIntraOpNumThreads(num_threads);
    }
    _session = make_unique<Ort::Session>(_ort_env, model, size, _session_options);
    if (!_session) {
        LOGE << "Construct ORT session failed";
        return false;
    }

    Ort::AllocatorWithDefaultOptions allocator;

    size_t input_count = _session->GetInputCount();
    if (!input_count) {
        LOGE << "Network Input count is zero";
        return false;
    }

    LOGV << "Input Node Name/Shape (" << input_count << "):";
    for (size_t i = 0; i < input_count; i++) {
        _input_names.emplace_back(_session->GetInputNameAllocated(i, allocator).get());

        LOGV << "input[" << i << "] name: " << _input_names.at(i);

        std::vector<std::int64_t> input_shape =
            _session->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        if (input_shape.empty()) {
            LOGE << "input shape is empty";
            return false;
        }

        // dynamic shape, negative means variable batch size
        for (auto& s : input_shape) {
            if (s < 0) s = abs(s);
        }

        _input_shapes.emplace_back(input_shape);
    }

    size_t output_count = _session->GetOutputCount();
    if (!output_count) {
        LOGE << "Network output count is zero";
        return false;
    }

    LOGV << "Output Node Name/Shape (" << output_count << "):";
    for (std::size_t i = 0; i < output_count; i++) {
        _output_names.emplace_back(_session->GetOutputNameAllocated(i, allocator).get());

        LOGV << "output[" << i << "] name: " << _output_names.at(i);

        std::vector<std::int64_t> output_shape =
            _session->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
        if (output_shape.empty()) {
            LOGE << "output shape is empty";
            return false;
        }
        _output_shapes.emplace_back(output_shape);
    }

    // Initialize input and output arrays with emtpy tensors
    for (int i = 0; i < input_count; i++) {
        _input_tensors.emplace_back(nullptr);
    }
    for (int i = 0; i < output_count; i++) {
        _output_tensors.emplace_back(nullptr);
    }

    return true;
}

const std::unordered_map<ONNXTensorElementDataType, DataType> DATA_TYPE_NAME_MAP = {
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, DataType::float32},
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8, DataType::uint8},
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8, DataType::int8},
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16, DataType::uint16},
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16, DataType::int16},
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, DataType::int32},
    // {ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, DataType::int64},
    // {ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING, ?},
    // {ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL, ?},
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16, DataType::float16},
    // {ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE, DataType::float64},
    {ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32, DataType::uint32},
    // {ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64, DataType::uint64}
};


// @TODO: extracting tensor info from ONNX model can be called from load_model() if we want to
// to support this functionality, otherwise this code can be removed
#if 0
bool PredictorONNX::set_inputs(NetworkPrivate* np, std::vector<Tensor>& inputs)
{
    size_t input_count = _input_names.size();
    if (!input_count) {
        LOGE << "Input count: 0";
        return false;
    }
    inputs.clear();
    inputs.reserve(input_count);

    for (size_t i = 0; i < input_count; i++) {
        Shape shape;
        for (auto& s : _input_shapes.at(i)) {
            shape.push_back(static_cast<int32_t>(abs(s)));
        }
        LOGV << " input shape: " << shape;

        ONNXTensorElementDataType tensor_type =
            _session->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetElementType();
        auto d = DATA_TYPE_NAME_MAP.find(tensor_type);
        if (d == DATA_TYPE_NAME_MAP.end()) {
            LOGE << "tensor type unsupported: " << tensor_type;
            return false;
        }
        DataType dtype = d->second;
        LOGV << "input shape: " << shape << " dtype: " << dtype;

        TensorAttributes attr{_input_names.at(i), dtype, Layout::nchw, Security::none, shape};
        inputs.emplace_back(np, i, Tensor::Type::in, &attr);
    }
    return true;
}

bool PredictorONNX::set_outputs(NetworkPrivate* np, std::vector<Tensor>& outputs)
{
    size_t output_count = _output_names.size();
    if (!output_count) {
        LOGE << "output count: 0";
        return false;
    }
    outputs.clear();
    outputs.reserve(output_count);

    for (size_t i = 0; i < output_count; i++) {
        Shape shape;
        for (auto& s : _output_shapes.at(i)) {
            shape.push_back(static_cast<int32_t>(abs(s)));
        }

        ONNXTensorElementDataType tensor_type =
            _session->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetElementType();

        auto d = DATA_TYPE_NAME_MAP.find(tensor_type);
        if (d == DATA_TYPE_NAME_MAP.end()) {
            LOGE << "tensor type unsupported: " << tensor_type;
            return false;
        }
        DataType dtype = d->second;
        LOGV << "output shape: " << shape << " dtype: " << dtype;

        TensorAttributes attr{_output_names.at(i), dtype, Layout::nchw, Security::none, shape};
        outputs.emplace_back(np, i, Tensor::Type::out, &attr);
    }
    return true;
}
#endif


BufferAttachment PredictorONNX::attach_buffer(Buffer* buffer, int32_t index, bool is_input)
{
    return 1;
}


bool PredictorONNX::detach_buffer(BufferAttachment handle)
{
    return true;
}


bool PredictorONNX::set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle)
{
    // CreateTensor is cheap, we can do on the fly
    assert (buffer);
    Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator,
                                                          OrtMemType::OrtMemTypeDefault);
    if (is_input) {
        const auto& shape = _input_shapes.at(index);
        auto tensor_type = _session->GetInputTypeInfo(index).GetTensorTypeAndShapeInfo().GetElementType();
        _input_tensors.at(index) = Ort::Value::CreateTensor(
            mem_info, buffer->data(), buffer->size(), shape.data(), shape.size(), tensor_type
        );
        LOGV << "ONNX input " << index << " " << " tensor type: " << tensor_type;
    }
    else {
        const auto& shape = _output_shapes.at(index);
        auto tensor_type = _session->GetOutputTypeInfo(index).GetTensorTypeAndShapeInfo().GetElementType();
        _output_tensors.at(index) = Ort::Value::CreateTensor(
            mem_info, buffer->data(), buffer->size(), shape.data(), shape.size(), tensor_type
        );
        LOGV << "ONNX output " << index << " " << " tensor type: " << tensor_type;
    }
    return true;
}


bool PredictorONNX::predict()
{
    LOGV << "Predicting...";

    vector<const char*> input_names_char(_input_names.size(), nullptr);
    transform(begin(_input_names), end(_input_names), begin(input_names_char),
                   [&](const string& str) { return str.c_str(); });

    vector<const char*> output_names_char(_output_names.size(), nullptr);
    transform(begin(_output_names), end(_output_names), begin(output_names_char),
                   [&](const string& str) { return str.c_str(); });

    try {
        _session->Run(Ort::RunOptions{nullptr}, input_names_char.data(), _input_tensors.data(),
                      _input_tensors.size(), output_names_char.data(), _output_tensors.data(),
                      _output_tensors.size());
    }
    catch (const Ort::Exception& exception) {
        LOGE << "ERROR running model inference: " << exception.what();
        return false;
    }

    return true;
}


PredictorONNX::PredictorONNX() : Predictor()
{
    LOGV << "PredictorONNX: " << this;
}


PredictorONNX::~PredictorONNX()
{
}


}  // namespace synap
}  // namespace synaptics
