///
/// Buffer private implementation.
/// This is kept in a private structure to keep Buffer class declaration clean
/// while providing an extended Buffer interface for internal implementation.
///

#pragma once

#include "network_private.hpp"
#include "synap/allocator.hpp"
#include <map>


namespace synaptics {
namespace synap {

class Buffer;

/// Network private implementation.
class BufferPrivate {
    friend class Buffer;

public:
    bool register_network(NetworkPrivate* net, BufferAttachment handle);
    bool unregister_network(NetworkPrivate* net);
    BufferAttachment handle(NetworkPrivate* net) const;
    bool cache_flush() const;
    bool cache_invalidate() const;
    uint32_t mem_id() const;
    uint32_t bid() const;
    size_t offset() const;

private:
    /// Data offset (used only when referring to an existing memory area)
    size_t _offset{};

    /// Data size (allocated memory size may be bigger)
    size_t _size{};

    /// Allocator responsible for deallocating memory
    Allocator* _allocator{};

    /// Memory
    Allocator::Memory _mem{};

    /// Is the memory a wrapped mem_id
    bool _wrapped_mem_id{false};

    /// CPU can read/write buffer data
    bool _cpu_data_access_allowed{true};

    /// Networks using this buffer
    std::map<NetworkPrivate*, BufferAttachment> _networks;
};


}  // namespace synap
}  // namespace synaptics
