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
/// Synap NPU control class.
///

#pragma once
#include "synap/types.hpp"
#include <memory>

namespace synaptics {
namespace synap {


/// Reserve NPU usage.
class Npu {
public:
    Npu();
    ~Npu();

    /// @return true if NPU successfully initialized.
    bool available() const;

    /// Lock exclusive right to perform inference for the current process.
    /// All other processes attemping to execute inference will fail, including those using NNAPI.
    /// The lock will stay active until unlock() is called or the Npu object is deleted.
    /// @return true if NPU successfully locked,
    /// false if NPU unavailable or locked by another process.
    /// Calling this method on an Npu object that is already locked has no effect, just returns true
    bool lock();

    /// Release exclusive right to perform inference
    /// @return true if success.
    /// Calling this method on an Npu object that is not locked has no effect, just returns true
    bool unlock();

    /// @return true if we currently own the NPU lock.
    ///
    /// Note: the only way to test if the NPU is locked by someone else is to try to lock() it.
    bool is_locked() const;


private:
    // Implementation details
    struct Private;
    std::unique_ptr<Private> d;
};



}  // namespace synap
}  // namespace synaptics
