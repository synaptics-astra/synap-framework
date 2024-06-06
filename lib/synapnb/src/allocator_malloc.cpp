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

#include "allocator_malloc.hpp"
#include "synap/logging.hpp"


using namespace std;

namespace synaptics {
namespace synap {


Allocator::Memory AllocatorMalloc::alloc(size_t size)
{
    void* ptr = aligned_alloc(64, Allocator::align(size, 64));
    LOGV << "Allocated memory of size: " << size << " at address: " << ptr;
    // Use an invalid BID since this memory doesn't come from synap allocator
    return Memory{ptr, 1, -1, 0, 0, (uint32_t)size};
}


void AllocatorMalloc::dealloc(const Memory& mem)
{
    if (mem.address) {
        LOGV << "Releasing memory at address: " << mem.address;
        free(mem.address);
    }
}


bool AllocatorMalloc::cache_flush(const Memory& mem, size_t size)
{
    // This allocator is used only when the entire inference is done in SW
    // so there is no need to flush the cache
    LOGV << "cache_flush ignored";
    return true;
}


bool AllocatorMalloc::cache_invalidate(const Memory& mem, size_t size)
{
    // This allocator is used only when the entire inference is done in SW
    // so there is no need to invalidate the cache
    LOGV << "cache_invalidate ignored";
    return true;
}


Allocator* malloc_allocator()
{
    static AllocatorMalloc allocator{};
    return &allocator;
}

}  // namespace synap
}  // namespace synaptics
