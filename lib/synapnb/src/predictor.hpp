#pragma once

#include "synap/buffer.hpp"
#include "synap/metadata.hpp"

namespace synaptics {
namespace synap {

typedef uint32_t BufferAttachment;
class Tensor;

/// Predictor
class Predictor {
public:

    virtual ~Predictor() {}

    /// Load a model.
    /// @param model     model data, as from e.g. fread()
    ///                  model data will be deallocated at the end of this method, if the
    ///                  predictor needs model data to be persistent it should make a local copy.
    /// @param size      model size in bytes
    /// @param meta      model's metadata.
    ///                  these information can be read/verified/updated as needed.
    /// @return          true if success
    virtual bool load_model(const void* model, size_t size, NetworkMetadata* meta) = 0;

    /// Run inference.
    /// @return          true if success
    virtual bool predict() = 0;

    /// Attach buffer.
    /// Called the first time a buffer is used on this network.
    /// This is a place where expensive operation can be done if needed.
    /// It is guaranteed that buffer memory address nor size will be changed after attachment.
    /// @param buffer    Buffer to be attached (never nullptr)
    /// @param index     Tensor index where the Buffer is attached
    /// @param is_input  Select between input/output
    /// @return          Non-null attachment id for this network if success.
    virtual BufferAttachment attach_buffer(Buffer* buffer, int32_t index, bool is_input) = 0;

    /// Set current buffer.
    /// Called each time a buffer is set to be used for an input/output tensor of the network.
    /// @param buffer    Buffer pointer (never nullptr)
    /// @param index     Tensor index where the Buffer is set
    /// @param is_input  Select between input/output
    /// @param handle    attachment-id returned by attach_buffer() for this buffer
    /// @return          true if success
    virtual bool set_buffer(Buffer* buffer, int32_t index, bool is_input, BufferAttachment handle) = 0;

    /// Detach buffer.
    /// Called for all attached buffers when the Network or the buffer itself is deleted.
    /// This must undo all operations done in attach_buffer()
    /// @param handle    attachment-id fo the buffer to detach
    /// @return          true if success
    virtual bool detach_buffer(BufferAttachment handle) = 0;

    /// Get I/O Tensor.
    /// Allows to override standard Tensor creation.
    /// @param index     index of Tensor to get
    /// @param is_input  select between input/output tensors
    /// @return          Tensor pointer to override default Tensor creation, nullptr otherwise.
    virtual Tensor* get_tensor(int32_t index, bool is_input) { return nullptr; }

};

}  // namespace synap
}  // namespace synaptics
