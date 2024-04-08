#
# NDA AND NEED-TO-KNOW REQUIRED
#
# Copyright (C) 2013-2020 Synaptics Incorporated. All rights reserved.
#
# This file contains information that is proprietary to Synaptics
# Incorporated ("Synaptics"). The holder of this file shall treat all
# information contained herein as confidential, shall use the
# information only for its intended purpose, and shall not duplicate,
# disclose, or disseminate any of this information in any manner
# unless Synaptics has otherwise provided express, written
# permission.
#
# Use of the materials may require a license of intellectual property
# from a third party or from Synaptics. This file conveys no express
# or implied licenses to any intellectual property rights belonging
# to Synaptics.
#
# INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS", AND
# SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
# INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
# INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
# CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
# OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
# BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
# COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
# DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
# TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.

if(NOT EXISTS ${ONNXRT_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "Cannot find ${ONNXRT_DIR}/CMakeLists.txt")
endif()


if(BUILD_ONNXRT_STATIC_LIB)
    list(APPEND onnxrt_args -Donnxruntime_BUILD_SHARED_LIB=OFF)
else()
    list(APPEND onnxrt_args -Donnxruntime_BUILD_SHARED_LIB=ON)
endif()

if(BUILD_ONNXRT_XNNPACK)
    list(APPEND onnxrt_args -Donnxruntime_USE_XNNPACK:STRING=ON -Donnxruntime_ENABLE_CPUINFO:STRING=ON)
else()
    list(APPEND onnxrt_args -Donnxruntime_USE_XNNPACK:STRING=OFF -Donnxruntime_ENABLE_CPUINFO:STRING=OFF)
endif()

list(APPEND onnxrt_args
    -Donnxruntime_USE_FULL_PROTOBUF:STRING=ON
    -Donnxruntime_ENABLE_LTO:STRING=ON
    -Donnxruntime_GENERATE_TEST_REPORTS:STRING=OFF
    -Donnxruntime_BUILD_UNIT_TESTS:STRING=OFF
    -DBUILD_PKGCONFIG_FILES:STRING=OFF
    -Donnxruntime_USE_NNAPI_BUILTIN:STRING=${BUILD_ONNXRT_NNAPI}
    # -Donnxruntime_ENABLE_MEMORY_PROFILE=ON
    # -DOnnxruntime_GCOV_COVERAGE=ON
    #-DPython_EXECUTABLE:FILEPATH=/usr/bin/python3
    #-DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/python3
    )

set(ONNXRT_BUILD_DIR ${CMAKE_BINARY_DIR}/onnxrt-${TARGET_NAME}-prefix/src/onnxrt-${TARGET_NAME}-build)
