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

#include "allocator_synap.hpp"
#include "synap/logging.hpp"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include "synap_device.h"
#include "synap_driver.hpp"


using namespace std;

namespace synaptics {
namespace synap {


bool AllocatorSynap::suspend_cpu_access(int fd)
{
    LOGI << "Suspending cpu access on dmabuf: " << fd;
    if (!synap_unlock_io_buffer(fd)) {
        LOGE << "Cannot suspend access to dmabuf " << fd << " error: " << strerror(errno);
        return false;
    }
    return true;
}


bool AllocatorSynap::resume_cpu_access(int fd)
{
    LOGI << "Resuming cpu access on dmabuf: " << fd;
    if (!synap_lock_io_buffer(fd)) {
        LOGE << "Cannot resume access to dmabuf " << fd << " error: " << strerror(errno);
        return false;
    }
    return true;
}


Allocator::Memory AllocatorSynap::alloc(size_t size)
{
    LOGV << "Allocating memory, size: " << size << dec;

    uint32_t mem_id;
    int32_t fd;
    uint32_t bid;

    if (!synap_allocate_io_buffer(size, &bid, &mem_id, (uint32_t *) &fd)) {
        LOGE << "Unable create buffer from fd " << fd;
        return {};
    }

    void *ptr;

    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        LOGE << "Unable to map dmabuf handle " << fd;
        close(fd);
        return {};
    }

    if (!resume_cpu_access(fd)) {
        LOGE << "Unable to resume cpu access to fd " << fd;
        munmap(ptr, size);
        close(fd);
        return {};
    }

    LOGV << "allocated fd is: " << fd;
    return Memory{ptr, 1, fd, bid, mem_id, (uint32_t) size};
}


void AllocatorSynap::dealloc(const Memory& mem)
{
    if (!mem.handle) {
        LOGV << "dealloc null, ignore";
        return;
    }
    if (mem.fd >= 0) {
        if (mem.address) {
            if(munmap(mem.address, mem.size) != 0) {
                LOGE << "munmap failed for fd: " << mem.fd;
            }
            else {
                LOGV << "munmap completed successfully for fd: " << mem.fd;
            }
        }
        if (close(mem.fd) != 0) {
            LOGE << "close fd=" << mem.fd << " failed, errno= " << strerror(errno);
        }
    }

    synap_destroy_io_buffer(mem.bid);
}


bool AllocatorSynap::cache_flush(const Memory& mem, size_t size)
{
    if (!mem.handle) {
        LOGV << "cache_flush null, ignore";
        return true;
    }

    LOGV << "Flushing cache at address: " << mem.address;
    if (!suspend_cpu_access(mem.fd)) {
        return {};
    }
    return true;
}


bool AllocatorSynap::cache_invalidate(const Memory& mem, size_t size)
{
    if (!mem.handle) {
        LOGV << "cache_invalidate null, ignore";
        return true;
    }

    LOGV << "Invalidating cache at address: " << mem.address;
    if (!resume_cpu_access(mem.fd)) {
        return {};
    }
    return true;
}


Allocator* synap_allocator()
{
    if (!synap_driver_available()) {
        return nullptr;
    }
    static AllocatorSynap allocator{};
    return &allocator;
}

}  // namespace synap
}  // namespace synaptics
