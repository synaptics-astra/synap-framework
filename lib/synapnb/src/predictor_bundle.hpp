#pragma once
#include <cstdint>
#include <stddef.h>
#include "predictor.hpp"
#include "synap/network.hpp"
#include <mutex>
#include <future>
#include <condition_variable>

namespace synaptics {
namespace synap {

class BundleParser;

/// PredictorBundle
/// This predictor is actually a container of sub-Networks.
/// It doesn't do much in itself, it just initializes, connects and executes the subnetworks
/// that will do the actual inference (execution order is definition order).
/// Tensors of a bundle network are simply aliases (references to) the actual tensors in the
/// contained subnetworks. Subnetworks are connected internally by sharing the buffers of the
/// corresponding out/in tensors.
/// This allows to have no overhead in bundled execution.
/// The special case of one input Tensor going to multiple separate subnetworks is handled by
/// exposing one of them and internally linking the others (subnetworks cannot share Tensors
/// between them since they may use different delegates with different buffer attachment methods).
class PredictorBundle : public Predictor{
public:
    PredictorBundle();
    ~PredictorBundle();

    bool load_model(const void* model, size_t size, NetworkMetadata* meta) override;
    bool predict() override;

    BufferAttachment attach_buffer(Buffer* buffer, int32_t index, bool is_input) override;
    bool set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle) override;
    bool detach_buffer(BufferAttachment handle) override;
    Tensor* get_tensor(int32_t index, bool is_input) override;

private:
    // Subgraph information
    struct Graph {
        // Network representing this subgraph
        Network net;

        // Graphs on which this graph depends (must be completed before we can start this one)
        std::vector<Graph*> dependencies;

        // True if inference completed successfully
        std::shared_future<bool> success;

        // Graph own index (handy to have)
        int index{};
    };

    // Set the tensor as input of the bundle model
    bool set_model_input(size_t input_index, Tensor& in_tensor);

    // Execute all the subgraphs sequentially in definition order
    bool predict_sequential();

    // Execute all the subgraphs in parallel whenever possible
    bool predict_parallel();

    // Graph execution handler, perform inference and send notification when completed
    bool run_graph(Graph& graph);

    // Max number of parallel executions (0: unlimited)
    int _parallel_limit{};

    // Input/output tensors, pointing to actual tensors in the subnetworks
    std::vector<Tensor*> _inputs;
    std::vector<Tensor*> _outputs;

    // Subgraphs in the bundle
    std::vector<Graph> _graphs;

    // Condition variables to syncronize parallel inferences
    std::mutex _inference_mutex;
    std::condition_variable _inference_done;

    // Number of inferences in progress
    int _in_progress{};
};

}  // namespace synap
}  // namespace synaptics
