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
/// Synap logging utilities
///

#pragma once

#include <iostream>
#include <ostream>
#include <sstream>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#define LOGV SYNAP_LOG(0)
#define LOGI SYNAP_LOG(1)
#define LOGW SYNAP_LOG(2)
#define LOGE SYNAP_LOG(3)

#define SYNAP_LOG(level)                                                              \
    synaptics::synap::Logger::enabled(level) && synaptics::synap::Logger{level}.out() \
                                                    << __FUNCTION__ << "():" << __LINE__ << ": "

namespace synaptics {
namespace synap {

class Logger {
public:
    Logger(int level) : _level(level) {}
    std::ostream& out() { return _log_message; }

    ~Logger()
    {
        static const char module[] = "SyNAP";
        static const char level_msg[] = "VIWE????";
        const std::string& log_string = _log_message.str();
#if defined(SYNAP_LOGS_TO_STDERR) || !defined(__ANDROID__)
        std::cerr << level_msg[_level] << ':' << module << ": " << log_string << std::endl;
#endif
#ifdef __ANDROID__
        __android_log_print(to_android_priority(_level), module, "%s", log_string.c_str());
#endif
    }

    /// @return true if the specified log level is enabled
    static bool enabled(int level)
    {
        static char* envs = getenv("SYNAP_NB_LOG_LEVEL");
        if (!envs)
            return level >= 2;
        static int env_level = atoi(envs);
        return env_level >= 0 && level >= env_level;
    }


private:
    int _level;
    std::ostringstream _log_message;
#ifdef __ANDROID__
    static int to_android_priority(int level)
    {
        switch (level) {
        case 0:
            return ANDROID_LOG_VERBOSE;
        case 1:
            return ANDROID_LOG_INFO;
        case 2:
            return ANDROID_LOG_WARN;
        case 3:
            return ANDROID_LOG_ERROR;
        default:
            return ANDROID_LOG_UNKNOWN;
        }
    }
#endif
};


}  // namespace synap
}  // namespace synaptics
