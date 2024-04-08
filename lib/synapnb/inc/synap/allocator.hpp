/*
 * NDA AND NEED-TO-KNOW REQUIRED
 *
 * Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
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
/// Synap allocator.
///

#pragma once

#include <algorithm>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <vector>

namespace synaptics {
namespace synap {


/// Buffer allocator.
/// Allows to allocate aligned memory from different areas.
/// Memory must be allocated such that it completely includes all the cache-lines used for the
/// actual data. This ensures that no cache-line used for data is also used for something else.
class Allocator {
public:
    struct Memory {
        /// Aligned memory pointer
        void* address{};

        /// Memory block handle, allocator specific.
        uintptr_t handle{};

        /// file descriptor used to pass a reference to other drivers/processes that support
        /// dmabuf buffers
        int32_t fd{-1};

        /// handle used to attach this memory area to a network as input/output buffer
        uint32_t bid{};

        /// handle used to pass a reference to this memory area to TAs, invalid after bid is
        /// destroyed
        uint32_t mem_id{};

        /// memory size
        size_t size{};
    };

    /// Allocate memory.
    /// @param size: required memory size in bytes
    /// @return allocated memory information
    virtual Memory alloc(size_t size) = 0;

    /// Deallocate memory.
    /// @param mem: memory block information
    virtual void dealloc(const Memory& mem) = 0;

    /// Flush cache for the entire memory block.
    /// @param mem: memory block information
    /// @param size: memory block size
    /// @return true if success
    virtual bool cache_flush(const Memory& mem, size_t size) = 0;

    /// Invalidate cache for the entire memory block.
    /// @param mem: memory block information
    /// @param size: memory block size
    /// @return true if success
    virtual bool cache_invalidate(const Memory& mem, size_t size) = 0;

    /// The allocator can be used
    /// @return true : true the allocator is ready to allocate, false otherwise
    virtual bool available() const { return true; }

    virtual ~Allocator() {}

    /// Required alignment. This corresponds to the size of a NPU MMU page.
    static constexpr size_t alignment = 4096;

    /// @return val rounded upward to NPU MMU alignment
    static uintptr_t align(uintptr_t val) { return (val + alignment - 1) & ~(alignment - 1); }

    /// @return val rounded upward to alignment num
    static uintptr_t align(uintptr_t val, uint32_t num) { return (val + num - 1) & ~(num - 1); }

    /// @return addr rounded upward to alignment
    static void* align(void* addr) { return (void*)align((uintptr_t)addr); }
    static const void* align(const void* addr) { return (void*)align((uintptr_t)addr); }

protected:
    // Prevent explicit delete since we are using only global instances
    void operator delete(void*) {}
};


/// Get a pointer to the global standard (paged) allocator.
/// @return pointer to standard allocator
Allocator* std_allocator();

// Specific allocators
Allocator* synap_allocator();
Allocator* malloc_allocator();

}  // namespace synap
}  // namespace synaptics
