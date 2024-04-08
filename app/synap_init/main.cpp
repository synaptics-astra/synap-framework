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
/// Synap command line initialization application.
/// 

#include <iostream>
#include <future>

#include "synap/arg_parser.hpp"
#include "synap/npu.hpp"

using namespace std;
using namespace synaptics::synap;

int main(int argc, char** argv)
{
    ArgParser args(argc, argv, "Synap NPU driver initialization", "[options]");
    bool interactive = args.has("-i", "Interactive mode, wait for user input to terminate");
    bool lock = args.has("--lock", "Keep NPU locked");
    args.check_help("--help", "Show help");
 
    Npu npu;
    if (!npu.available()) {
        cerr << "Error, NPU unavailable" << endl;
        return 1;
    }
    if (lock) {
        if (!npu.lock()) {
            cerr << "Error, NPU lock failed" << endl;
            return 1;
        }
        cout << "NPU locked" << endl;
    }

    if (interactive) {
        cout << "NPU ready, press enter to exit." << endl;
        cin.get();
        return 0;
    }

    // Wait forever
    cout << "NPU ready." << endl;
    std::promise<void>().get_future().wait();
    
    return 0;
}
