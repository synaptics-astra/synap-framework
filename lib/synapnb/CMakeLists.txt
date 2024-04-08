#
# NDA AND NEED-TO-KNOW REQUIRED
#
# Copyright (C) 2013-2023 Synaptics Incorporated. All rights reserved.
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

add_library(synapnb SHARED)

set_target_properties(synapnb PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/synapnb.map)
target_link_libraries(synapnb PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/synapnb.map -Wl,--gc-sections)

if(CMAKE_CROSSCOMPILING)
    # Some code may benefit from NEON instruction for ARM
    target_compile_definitions(synapnb PRIVATE SYNAP_NB_NEON=1)
endif()

find_package(Threads REQUIRED)
target_link_libraries(synapnb PUBLIC ${CMAKE_THREAD_LIBS_INIT})

target_sources(synapnb PRIVATE
    src/allocator.cpp
    src/buffer.cpp
    src/network.cpp
    src/predictor_bundle.cpp
    src/quantization.cpp
    src/tensor.cpp
)

if(ENABLE_EBGRUNTIME)
    message("EBG runtime is enable")
    target_sources(synapnb PRIVATE
        src/allocator_synap.cpp
        src/npu.cpp
        src/predictor_ebg.cpp
        src/synap_driver.cpp
    )
    target_compile_definitions(synapnb PUBLIC SYNAP_EBG_ENABLE=1)
    target_link_libraries(synapnb PUBLIC synap_device)
else()
    # Alloc-based allocator is not available when EBG is also supported since it does not
    # provide aligned allocations
    target_sources(synapnb PRIVATE src/allocator_malloc.cpp)
endif()


if(ENABLE_ONNXRUNTIME)
    target_sources(synapnb PRIVATE src/predictor_onnx.cpp)
    target_link_libraries(synapnb PUBLIC onnxruntime)
    target_compile_definitions(synapnb PRIVATE SYNAP_ONNX_ENABLE=1)
endif()


if(ENABLE_TFLITERUNTIME)
    target_sources(synapnb PRIVATE src/predictor_tflite.cpp)
    target_link_libraries(synapnb PUBLIC tensorflow-lite)
    target_compile_definitions(synapnb PRIVATE SYNAP_TFLITE_ENABLE=1)
endif()


target_link_libraries(synapnb
    PUBLIC synap_utils
    PUBLIC synap_base
)

set(VSI_DIR ${CMAKE_SOURCE_DIR}/external/vsi_acuity/cmdtools)

target_include_directories(synapnb
    PUBLIC inc
    PRIVATE android
)

file(GLOB HEADERS inc/synap/*.hpp)

set_target_properties(synapnb PROPERTIES PUBLIC_HEADER "${HEADERS}")

if(ENABLE_SECURE_DEBUG AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/src/allocator_secure_debug.cmake)
    # Use debug allocator instead of standard one
    include(${CMAKE_CURRENT_LIST_DIR}/src/allocator_secure_debug.cmake)
endif()

install(TARGETS synapnb)
