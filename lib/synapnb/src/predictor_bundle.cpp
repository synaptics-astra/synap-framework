#include "predictor_bundle.hpp"
#include "synap/string_utils.hpp"

#ifdef SYNAP_FILE_BASED_BUNDLE
#include "synap/bundle_parser_dir.hpp"
#endif
#include "synap/bundle_parser_zip.hpp"

#include "synap/logging.hpp"
#include "synap/metadata.hpp"

using namespace std;

namespace synaptics {
namespace synap {


PredictorBundle::PredictorBundle() : Predictor()
{
    LOGV << "PredictorBundle: " << this;
}


PredictorBundle::~PredictorBundle()
{
}


bool PredictorBundle::set_model_input(size_t input_index, Tensor& in_tensor)
{
    _inputs.resize(max(_inputs.size(), input_index + 1));
    if (_inputs[input_index] == nullptr) {
        // Make the bundle input refer to this Tensor
        _inputs[input_index] = &in_tensor;
    }
    else {
        // Bundle input already referring to another Tensor.
        // This happens when the same input of the bundle goes to multiple subgraphs,
        // notify current tensor that he has a sibiling so that it can forward any buffer change.
        LOGI << "Bundle connect tensor to existing input: " << input_index;
        _inputs[input_index]->add_sibling(&in_tensor);
    }
    return true;
}


bool PredictorBundle::load_model(const void* model, size_t size, NetworkMetadata* meta)
{
    // Get bundle root path from meta info
    size_t root_dir_ix = format_parse::value_pos(meta->delegate, "dir");
    unique_ptr<BundleParser> bundle;
    if (root_dir_ix == string::npos) {
        auto bundle_zip = new BundleParserZip;
        bundle.reset(bundle_zip);
        if (!bundle_zip->init(model, size)) {
            LOGE << "Error parsing bundle model";
            return false;
        }
    }
    else {
        #ifdef SYNAP_FILE_BASED_BUNDLE
        // Use filesystem-based bundle
        auto bundle_dir = new BundleParserDir;
        bundle.reset(bundle_dir);
        const char* root_bundle_dir = &meta->delegate[root_dir_ix];
        LOGV << root_bundle_dir;
        if (!bundle->init(string(root_bundle_dir) + "/bundle.json")) {
            LOGE << "Error parsing bundle model information from directory: " << root_bundle_dir;
            return false;
        }
        #else
        LOGE << "File-based bundle model not enabled";
        return false;
        #endif
    }

    // Create subgraphs
    for (const auto& graph_info : bundle->graph_info()) {
        Graph graph;
        if (!graph_info.model_data.empty()) {
            if (!graph.net.load_model(graph_info.model_data.data(), graph_info.model_data.size(),
                                graph_info.meta_data.c_str())) {
                LOGE << "Missing model data for bundle graph";
                return false;
            }
        } else {
            if (!graph.net.load_model(graph_info.model, graph_info.meta)) {
                LOGE << "Loading bundle graph " << graph_info.model << " failed";
                return false;
            }
        }
        _graphs.push_back(std::move(graph));
    }

    // Connect subgraphs and collect the list of model inputs
    bool success = true;
    for (size_t graph_ix = 0; graph_ix < _graphs.size(); graph_ix++) {
        _graphs[graph_ix].index = graph_ix;
        const auto& graph_info = bundle->graph_info()[graph_ix];
        for (size_t in_ix = 0; in_ix < graph_info.inputs.size(); in_ix++) {
            const auto& in = graph_info.inputs[in_ix];
            Tensor& in_tensor = _graphs[graph_ix].net.inputs[in_ix];
            if (in.subgraph_index < 0) {
                // This input comes directly from the inputs of the bundle model
                success &= set_model_input(in.tensor_index, in_tensor);
            }
            else {
                // Share the buffers of the interconnected out/in tensors
                // so no data copy is required at inference time.
                Tensor& out_tensor = _graphs[in.subgraph_index].net.outputs[in.tensor_index];
                success &= in_tensor.set_buffer(out_tensor.buffer());
                
                // Create the graphs depedency
                _graphs[graph_ix].dependencies.push_back(&_graphs[in.subgraph_index]);
            }
        }
    }

    // Get list of model outputs
    for (const auto& out : bundle->outputs()) {
        _outputs.push_back(&_graphs[out.subgraph_index].net.outputs[out.tensor_index]);
    }

    // Get max parallelism level
    // @todo: implement a more intelligent "parallelizablility" analysis or rely on
    // converter to set _parallel_limit at 1 if the bundle is not parallelizable
    bool parallelizable = _graphs.size() > 1;
    _parallel_limit = parallelizable ? bundle->parallel_limit() : 1;

    
    // Adjust the size of the in/out tensor info in meta.
    // Actual info will not be used since we will alias the in/out tensors of the subgraphs.
    meta->inputs.resize(_inputs.size());
    meta->outputs.resize(_outputs.size());

    return success;
}


Tensor* PredictorBundle::get_tensor(int32_t index, bool is_input)
{
    vector<Tensor*>& tensors = is_input ? _inputs : _outputs;
    if (index < 0 || index >= tensors.size()) {
        LOGE << "Error getting bundle " << (is_input? "input " : "output ") << index;
        return nullptr;
    }
    return tensors[index];
}


BufferAttachment PredictorBundle::attach_buffer(Buffer* buffer, int32_t index, bool is_input)
{
    // This method shall never be called. Attach will be done by the subgraph owning the tensor.
    LOGE << "Bundle internal error";
    assert(buffer != nullptr);
    return 0;
}


bool PredictorBundle::detach_buffer(BufferAttachment handle)
{
    // This method shall never be called. Detach will be done by the subgraph owning the tensor.
    LOGE << "Bundle internal error";
    assert(handle != 0);
    return false;
}


bool PredictorBundle::set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle)
{
    // This method shall never be called. Actual set will be done by the subgraph owning the tensor.
    LOGE << "Bundle internal error";
    assert(buffer != nullptr);
    return false;
}


bool PredictorBundle::predict_sequential()
{
    LOGI << "Starting bundle sequential inference";
    for (size_t i = 0; i < _graphs.size(); i++) {
        if (!_graphs[i].net.predict()) {
            LOGE << "Inference with subgraph " << i << " failed";
            return false;
        }
    }
    return true;
}


bool PredictorBundle::predict_parallel()
{
    // Start execution of all graphs
    LOGI << "Starting bundle parallel inference";
    {
        lock_guard lock(_inference_mutex);
        for (Graph& graph: _graphs) {
            graph.success = async(launch::async, [&graph, this]{ return run_graph(graph); } );
        }
    }

    // Wait until all subgraphs have finished execution and check that everything went well
    LOGV << "Waiting for bundle inference to complete";
    bool success = true;
    for (Graph& graph: _graphs) {
        success &= graph.success.get();
    }
    assert(_in_progress == 0);
    return success;
}


bool PredictorBundle::predict()
{
    bool success  = _parallel_limit == 1 ? predict_sequential() : predict_parallel();
    LOGV << "Bundle inference completed, success: " << success;
    return success;
}


bool PredictorBundle::run_graph(Graph& graph)
{
    bool success = true;

    // We can proceed only after init so that all graphs have been started all our inputs
    // processed; taking the lock and release it immediately achieves that.
    { lock_guard lock{_inference_mutex}; }
  
    for (auto& in: graph.dependencies) {
        success &= in->success.get();
    }

    if (_parallel_limit) {
        // Make sure not to exceed max specified parallelism
        unique_lock lock(_inference_mutex);
        _inference_done.wait(lock, [this]{ return _in_progress < _parallel_limit; });
        _in_progress++;
    }

    success = success && graph.net.predict();
    if (success) {
        LOGV << "Inference for bundle graph: " << graph.index << " completed";
    }
    else {
        LOGE << "Inference for bundle graph: " << graph.index << " failed";
    }

    if (_parallel_limit) {
        lock_guard lock(_inference_mutex);
        _in_progress--;
        assert(_in_progress >= 0);
        _inference_done.notify_all();
    }
    return success;
}


}  // namespace synap
}  // namespace synaptics
