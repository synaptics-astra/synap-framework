///
/// Network private implementation.
/// This is kept in a private structure to keep Network class declaration clean
/// while providing an extended Network interface for Tensor class implementation.
///

#pragma once

#include "predictor.hpp"
#include "synap/buffer.hpp"
#include "synap/tensor.hpp"
#include "synap/types.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>


namespace synaptics {
namespace synap {

class Network;

/// Network private implementation.
class NetworkPrivate {
    friend class Network;

public:
    bool register_buffer(Buffer* buffer, size_t index, bool is_input);
    bool unregister_buffer(Buffer* buffer);
    bool load_model_file(const std::string& model_file, const std::string& meta_file);
    bool load_model_data(const void* data, size_t data_size, const char* meta_data);

protected:
    void unregister_buffers();
    bool do_predict();
    std::vector<Tensor> create_tensors(Tensor::Type ttype, const std::vector<TensorAttributes>& tattrs);

    std::unique_ptr<Predictor> _predictor{};

    std::vector<Tensor> _inputs;
    std::vector<Tensor> _outputs;
    std::set<Buffer*> _buffers;
};


}  // namespace synap
}  // namespace synaptics
