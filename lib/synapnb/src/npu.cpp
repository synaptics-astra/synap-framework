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


#include "synap/npu.hpp"
#include "synap/logging.hpp"
#include "synap_device.h"
#include "synap_driver.hpp"

using namespace std;

namespace synaptics {
namespace synap {


/// Npu private implementation.
struct Npu::Private {
    bool _available{};
    bool _locked{};
};


Npu::Npu() : d{new Npu::Private}
{
    d->_available = synap_driver_available();
}


Npu::~Npu()
{
    // Release inference lock(s) if we had any
    unlock();
}


bool Npu::available() const
{
    return d->_available;
}


bool Npu::lock()
{
    if (d->_available && !d->_locked && synap_lock_npu()) {
        LOGV << "NPU Locked";
        d->_locked = true;
    }
    return d->_locked;
}


bool Npu::unlock()
{
    if (d->_locked) {
        LOGV << "Unlocking NPU";
        if (!synap_unlock_npu()) {
            return false;
        }
        d->_locked = false;
    }
    return true;
}


bool Npu::is_locked() const
{
    return d->_locked;
}


}  // namespace synap
}  // namespace synaptics
