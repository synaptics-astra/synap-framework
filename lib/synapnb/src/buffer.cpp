/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2020 Synaptics Incorporated. All rights reserved.
 *
 * This file contains information that is proprietary to Synaptics
 * Incorporated ("Synaptics"). The holder of this file shall treat all
 * information contained herein as confidential, shall use the
 * information only for its intended purpose, and shall not duplicate,
 * disclose, or disseminate any of this information in any manner
 * unless Synaptics has otherwise provided express, written
 * permission.
 *
 * Use of the materials may require a license of intellectual property
 * from a third party or from Synaptics. This file conveys no express
 * or implied licenses to any intellectual property rights belonging
 * to Synaptics.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */
///
/// Synap data buffer.
///


#include "synap/buffer.hpp"
#include "buffer_private.hpp"
#include "network_private.hpp"
#include "synap/logging.hpp"

#ifdef SYNAP_EBG_ENABLE
#include "synap_device.h"
#endif

#include <cstring>

using namespace std;

namespace synaptics {
namespace synap {

//
// Buffer
//

Buffer::Buffer(Allocator* allocator) : d{new BufferPrivate()}
{
    d->_allocator = allocator ? allocator : std_allocator();
}

Buffer::Buffer(size_t size, Allocator* allocator) : Buffer(allocator)
{
    resize(size);
}

Buffer::Buffer(uint32_t mem_id, size_t offset, size_t size) : d{new BufferPrivate()}
{
#ifdef SYNAP_EBG_ENABLE
    if (!synap_create_io_buffer_from_mem_id(mem_id, offset, Allocator::align(size), &d->_mem.bid)) {
        return;
    }

    d->_wrapped_mem_id = true;
    d->_mem.address = nullptr;
    d->_mem.mem_id = mem_id;
    d->_offset = offset;
    d->_size = size;
    d->_cpu_data_access_allowed = false;
#else
    LOGE << "only supported when mem_id exist" << endl;
#endif
}


Buffer::Buffer(const Buffer& rhs, size_t offset, size_t size) : d{new BufferPrivate()}
{
#ifdef SYNAP_EBG_ENABLE
    if (offset > rhs.d->_size) {
        LOGE << "Offset " << offset << " is bigger than original tensor size: " << rhs.d->_size;
        return;
    }
    offset += rhs.d->_offset;
    if (offset + size > rhs.d->_size) {
        LOGE << "Offset+size beyond the original tensor size: " << rhs.d->_size;
        return;
    }

    if (!synap_create_io_buffer_from_mem_id(rhs.d->_mem.mem_id, offset, Allocator::align(size), &d->_mem.bid)) {
        return;
    }

    d->_wrapped_mem_id = true;
    d->_mem.address = nullptr;
    d->_mem.mem_id = rhs.d->_mem.mem_id;
    d->_offset = offset;
    d->_size = size;
    d->_cpu_data_access_allowed = false;
#else
    LOGE << "only supported when mem_id exist" << endl;
#endif
}


Buffer::Buffer(uint32_t handle, size_t offset, size_t size,
               bool is_mem_id) : d{new BufferPrivate()}
{
#ifdef SYNAP_EBG_ENABLE
    if (is_mem_id) {
        if (!synap_create_io_buffer_from_mem_id(handle, offset, Allocator::align(size),
                                                &d->_mem.bid)) {
            return;
        }

        d->_mem.mem_id = handle;

    } else {
        if (!synap_create_io_buffer(handle, offset, Allocator::align(size),
                                    &d->_mem.bid, &d->_mem.mem_id)) {
            return;
        }
    }

    d->_wrapped_mem_id = true;
    d->_mem.address = nullptr;
    d->_offset = offset;
    d->_size = size;
    d->_cpu_data_access_allowed = false;
#else
    LOGE << "only supported when mem_id exist" << endl;
#endif
}


int32_t Buffer::memory_fd() const
{
    return 0;
}


Buffer::Buffer(Buffer&& rhs) noexcept
{
    if (!rhs.d->_networks.empty()) {
        // @todo: not yet implemented
        LOGE << "Error moving buffer used by a network " << &rhs;
        return;
    }
    d = std::move(rhs.d);
}


Buffer& Buffer::operator=(Buffer&& rhs) noexcept
{
    if (!rhs.d->_networks.empty()) {
        LOGE << "Error moving buffer used by a network " << &rhs;
        return *this;
    }
    if (!d->_networks.empty()) {
        LOGE << "Error moving into buffer used by a network " << this;
        return *this;
    }
    if (d->_wrapped_mem_id || !resize(0)) {
        LOGE << "Error moving into already allocated buffer " << this;
        return *this;
    }
    d = std::move(rhs.d);
    return *this;
}


Buffer::~Buffer()
{
    if (!d) {
        // Empty buffer
        return;
    }
    // Unregister from all networks
    while (!d->_networks.empty()) {
        auto net = d->_networks.begin()->first;
        net->unregister_buffer(this);
    }

    // Release memory
    if (d->_allocator) {
        d->_allocator->dealloc(d->_mem);
    }

#ifdef SYNAP_EBG_ENABLE
    // Unregister the mem_id
    if (d->_wrapped_mem_id) {
        synap_destroy_io_buffer(d->_mem.bid);
    }
#endif
}


bool Buffer::resize(size_t size)
{
    if (size == d->_size) {
        return true;
    }
    if (!d->_allocator) {
        LOGE << "Resize failed: no allocator";
        return false;
    }

    if (!d->_networks.empty()) {
        // @todo: not yet implemented
        LOGE << "Error resizing buffer used by a network " << this;
        return false;
    }

    d->_allocator->dealloc(d->_mem);
    d->_mem = {};

    if (size) {
        d->_mem = d->_allocator->alloc(size);

        if (!d->_mem.address && !d->_mem.handle) {
            LOGE << "Error resizing buffer from " << d->_size << " to " << size;
            d->_size = 0;
            return false;
        }
    }

    d->_size = size;
    return true;
}


bool Buffer::assign(const void* data, size_t sz)
{
    if (!resize(sz)) {
        return false;
    }
    memcpy(d->_mem.address, data, sz);
    return true;
}


size_t Buffer::size() const
{
    return d->_size;
}


const void* Buffer::data() const
{
    return d->_mem.address;
}


void* Buffer::data()
{
    return d->_mem.address;
}


bool Buffer::allow_cpu_access(bool allow)
{
    bool was_allowed = d->_cpu_data_access_allowed;
    if (!allow && was_allowed) {
        // We are making the buffer inaccessible by the CPU.
        // Flush any data written by CPU until now, this also ensures flush will not happen later
        // in case of write-back cache.
        d->cache_flush();
    }
    d->_cpu_data_access_allowed = allow;
    if (allow && !was_allowed) {
        // We just made a buffer accessible by the CPU.
        // Invalidate cache to be able to see whatever content was in memory.
        d->cache_invalidate();
    }
    return was_allowed;
}


bool Buffer::set_allocator(Allocator* allocator)
{
    if (allocator == d->_allocator) {
        return true;
    }
    if (d->_size != 0) {
        LOGE << "Can't change allocator, buffer not empty";
        return false;
    }
    d->_allocator = allocator;
    return true;
}


uint32_t Buffer::mem_id() const
{
    return d->mem_id();
}

uint32_t Buffer::bid() const
{
    return d->bid();
}


//
// BufferPrivate
//

bool BufferPrivate::register_network(NetworkPrivate* net, BufferAttachment handle)
{
    if (_networks.find(net) != _networks.end()) {
        LOGW << "Network already registered " << net;
        return true;
    }

    _networks[net] = handle;
    return true;
}


bool BufferPrivate::unregister_network(NetworkPrivate* net)
{
    auto it = _networks.find(net);
    if (it == _networks.end()) {
        LOGE << "Error network not registered " << net;
        return false;
    }
    auto handle = it->second;
    _networks.erase(it);
    return true;
}


bool BufferPrivate::cache_flush() const
{
    if (!_cpu_data_access_allowed) {
        return true;
    }
    if (!_allocator) {
        LOGE << "Unable to flush buffer (no allocator)";
        return false;
    }
    return _allocator->cache_flush(_mem, _size);
}


bool BufferPrivate::cache_invalidate() const
{
    if (!_cpu_data_access_allowed) {
        return true;
    }
    if (!_allocator) {
        LOGE << "Unable to invalidate buffer (no allocator)";
        return false;
    }
    return _allocator->cache_invalidate(_mem, _size);
}


BufferAttachment BufferPrivate::handle(NetworkPrivate* net) const
{
    auto it = _networks.find(net);
    if (it == _networks.end()) {
        LOGV << "Buffer " << this << " not yet registered on net " << net;
        return {};
    }
    return it->second;
}

uint32_t BufferPrivate::mem_id() const
{
    return _mem.mem_id;
}

uint32_t BufferPrivate::bid() const
{
    return _mem.bid;
}


size_t BufferPrivate::offset() const
{
    return _offset;
}


}  // namespace synap
}  // namespace synaptics
