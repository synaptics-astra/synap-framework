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
# or implied licenses to any intellectual property rights belonging`
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
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)
cmake_minimum_required(VERSION 3.19)
project(SyNAP_FRAMEWORK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_CROSSCOMPILING)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie  -Wl,--no-undefined -Wl,--no-allow-shlib-undefined")
endif()
# Release build: Strip shared libraries
add_link_options($<$<CONFIG:RELEASE>:-s>)
if(ENABLE_CPU_PROFILING)
    add_compile_options(-pg)
endif()

set(SYNAP_SYSROOT_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
set(SYNAP_SYSROOT_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

# yocto runtime build and toolchain build cmake sysroot are different
# 1. runtime build, each recipe has its own sysroot, cmake install prefix `/usr` is relative path to `recipe-sysroot`
#    better keep runtime build cmake config to make use of yocto recipe sysroot
# 2. toolchain build, we can explicitly set cmake install prefix, as the normal cmake build
if(CMAKE_FIND_ROOT_PATH)
    foreach(root_path IN LISTS CMAKE_FIND_ROOT_PATH)
        if(${root_path} MATCHES "recipe-sysroot*")
            set(SYNAP_SYSROOT_INCLUDEDIR ${root_path}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
            set(SYNAP_SYSROOT_LIBDIR ${root_path}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
            break()
        endif()
    endforeach()
endif()

link_directories(${SYNAP_SYSROOT_LIBDIR})

option(ENABLE_EBGRUNTIME "Enable EBGRUNTIME compilation and associated SyNAP support" ON)
option(ENABLE_ONNXRUNTIME "Enable ONNXRUNTIME compilation and associated SyNAP support" OFF)
option(ENABLE_TFLITERUNTIME "Enable TFLITE runtime compilation and associated SyNAP support" OFF)

option(ENABLE_PRIVATE_ALLOCATORS "Enable custom private allocators" OFF)

# used in app/synap_cli_nb
find_package(ebg_utils REQUIRED)

# used in lib/synap_nb
find_package(synap_device REQUIRED)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    message("Old GCC version detected, disabling C++-17 standard filesystem library")
    set(ENABLE_CXX_STD_FILESYSTEM OFF)
else()
    set(ENABLE_CXX_STD_FILESYSTEM ON)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    # Enable file-based bundle models
    set(ENABLE_FILE_BASED_BUNDLE ON)
    add_compile_definitions(SYNAP_FILE_BASED_BUNDLE=1)
endif()

# add common dmabuf allocator options here for lib/app
option(ENABLE_DMA_HEAP_ALLOCATOR "Enable synap DMA-BUF heap allocator" ${VSSDK_KERNEL_DMABUF_ENABLED})
message("ENABLE_DMA_HEAP_ALLOCATOR: ${ENABLE_DMA_HEAP_ALLOCATOR}")

message("ENABLE_EBGRUNTIME: ${ENABLE_EBGRUNTIME}")
message("ENABLE_ONNXRUNTIME: ${ENABLE_ONNXRUNTIME}")
message("ENABLE_TFLITERUNTIME: ${ENABLE_TFLITERUNTIME}")

if(ENABLE_ONNXRUNTIME)
    find_package(Onnxruntime REQUIRED)
endif()

message("ENABLE_NCNN: ${ENABLE_NCNN}")
if(ENABLE_NCNN)
    find_package(NCNN REQUIRED)
endif()

message("ENABLE_TFLITERUNTIME: ${ENABLE_TFLITERUNTIME}")
if(ENABLE_TFLITERUNTIME)
    find_package(tensorflow_lite REQUIRED)
endif()

add_subdirectory(lib)

if(ENABLE_PRIVATE_ALLOCATORS)
    add_subdirectory(private/synap_allocators)
endif()

add_subdirectory(app)

option(ENABLE_SUBLIMA "Enable B<>COM Sublima support" OFF)

if(ENABLE_SUBLIMA)
    add_subdirectory(sublima/lib)
    add_subdirectory(sublima/cli)
endif()

if(ENABLE_TEST)
    add_subdirectory(test)
endif()
