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

# CMake 3.19 is required for latest Android-NDK
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

option(ENABLE_VXK "Enable VX kernel compilation" OFF)
option(ENABLE_MODELS "Enable model compilations" OFF)
option(ENABLE_DOC "Enable document generator" OFF)
option(ENABLE_SYNAP_DEVICE_SIMULATOR "Enable synap_device simulator" ON)
option(ENABLE_LINUX_BASELINE "Enable Linux Baseline" OFF)
option(ENABLE_ACUITY "Enable acuity armorization" OFF)

option(ENABLE_OPENCV_JPEG "Enable OpenCV JPEG 3rd party compilation and support" OFF)
option(ENABLE_OPENCV_PNG "Enable OpenCV PNG 3rd party compilation and support" OFF)

option(ENABLE_CPU_PROFILING "Enable profiling with gcc/gprog" OFF)

# the difference between ${target} and ${target}_VSSDK is that
# the former is used to build target with more feature enabled as tflite, onnx and or the others
# and the latter is used to build the VSSDK with basic SyNAP feature of driver, sw stack and framework
option(ENABLE_LINUX_HOST "Enable Linux Host" OFF)
option(ENABLE_ANDROID_NDK "Enable Android NDK" OFF)
option(ENABLE_ANDROID64_NDK "Enable Android 64bit NDK" OFF)
option(ENABLE_LINUX_YOCTO_ARMv7a "Enable Linux Yocto 32bit" OFF)
option(ENABLE_LINUX_YOCTO_AARCH64 "Enable Linux Yocto 64bit" OFF)

option(ENABLE_LINUX_HOST_VSSDK "Enable VSSDK Linux Host" OFF)
option(ENABLE_ANDROID_VSSDK "Enable Android VSSDK" OFF)
option(ENABLE_ANDROID64_NDK_VSSDK "Enable Android 64bit NDK" OFF)
option(ENABLE_LINUX_YOCTO_ARMv7a_VSSDK "Enable Linux Yocto 32bit" OFF)
option(ENABLE_LINUX_YOCTO_AARCH64_VSSDK "Enable Linux Yocto 64bit" OFF)


set(PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
message("VSSDK_DIR: ${VSSDK_DIR}")
message("PROJECT_DIR: ${PROJECT_DIR}")

# FIXME: not robust
# with cmake 3.24 we could use
# if(${VSSDK_DIR}/synap PATH_EQUAL ${PROJECT_DIR})
if(EXISTS ${PROJECT_DIR}/../ampsdk)
    message("Building SyNAP within VSSDK")
    set(VSI_PREFIX vsi_)
endif()

set(FRAMEWORK_DIR ${PROJECT_DIR}/framework)
set(NPU_DRIVER_DIR ${PROJECT_DIR}/${VSI_PREFIX}npu_driver)
set(NPU_SW_STACK_DIR ${PROJECT_DIR}/${VSI_PREFIX}npu_sw_stack)
set(NPU_VXK_DIR ${PROJECT_DIR}/${VSI_PREFIX}npu_sw_stack/vcompiler)

set(DOC_DIR ${PROJECT_DIR}/framework/doc)
set(ACUITY_DIR ${PROJECT_DIR}/acuity)
set(MODELS_DIR ${PROJECT_DIR}/models)
set(OPENCV_DIR ${PROJECT_DIR}/opencv)
set(TFLITE_DIR ${PROJECT_DIR}/tensorflow/tensorflow/lite)
set(ONNXRT_DIR ${PROJECT_DIR}/onnxruntime/cmake)
set(TIMVX_DIR ${PROJECT_DIR}/TIM-VX)
set(VXDELEGATE_DIR ${PROJECT_DIR}/tflite-vx-delegate)
set(NCNN_DIR ${PROJECT_DIR}/ncnn)

list(APPEND CMAKE_MODULE_PATH ${FRAMEWORK_DIR}/cmake/modules ${CMAKE_CURRENT_LIST_DIR})
include(BuildType)

project(SyNAP LANGUAGES NONE)

if(ENABLE_ANDROID_NDK OR ENABLE_ANDROID64_NDK OR ENABLE_ANDROID_VSSDK OR ENABLE_ANDROID64_NDK_VSSDK OR ENABLE_LINUX_YOCTO_ARMv7a_VSSDK OR ENABLE_ANDROID64_NDK_VSSDK)
    find_package(VSSDK)
    # transform path to absolute paths to prevent problem in external projects
    file(REAL_PATH "${VSSDK_DIR}" VSSDK_DIR BASE_DIRECTORY ${CMAKE_BINARY_DIR})
    if(NOT EXISTS ${VSSDK_DIR}/build)
        message(FATAL_ERROR "VSSDK not found. Please add -DVSSDK_DIR=path/to/vssdk to the cmake command line.")
    endif()
endif()

# Print Installing but not Up-to-date messages.
set(CMAKE_INSTALL_MESSAGE LAZY)
# Default install dir
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "..." FORCE)
endif()

set(CMAKE_INSTALL_INCLUDEDIR include)

include(ExternalProject)

if(ENABLE_VXK)
    # VXKa are not built for all SOCs, just for VS680A0 and VS640A0
    set(VXK_SOC "")
    if ("VS680A0" IN_LIST SOC)
        list(APPEND VXK_SOC "VS680A0")
    endif()
    if ("VS640A0" IN_LIST SOC)
        list(APPEND VXK_SOC "VS640A0")
    endif()

    set(vxk_ARGS
        -DSOC:STRING=${VXK_SOC}
        -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_INSTALL_MESSAGE:STRING=LAZY
    )
    ExternalProject_Add(vxk
        BUILD_ALWAYS TRUE
        USES_TERMINAL_BUILD true
        SOURCE_DIR ${NPU_VXK_DIR}
        CMAKE_CACHE_ARGS ${vxk_ARGS}
        )
endif()

if(ENABLE_DOC)
    ExternalProject_Add(doc
        BUILD_ALWAYS TRUE
        USES_TERMINAL_BUILD TRUE
        SOURCE_DIR ${DOC_DIR}
        CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX} -DCMAKE_INSTALL_MESSAGE:STRING=LAZY
        )
endif()

if(ENABLE_ACUITY)
    ExternalProject_Add(acuity
        BUILD_ALWAYS TRUE
        USES_TERMINAL_BUILD TRUE
        SOURCE_DIR ${ACUITY_DIR}
        CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX} -DCMAKE_INSTALL_MESSAGE:STRING=LAZY
        )
endif()

include(build_target.cmake)

if(ENABLE_LINUX_HOST)
    build_target(x86_64-linux-gcc
        EBG
        MODELS
        FRAMEWORK_TOOLS OPENCV
        TFLITE TFLITE_SHARED
        ONNXRT
        TIMVX VX_DELEGATE
        NCNN
    )
endif()

if(ENABLE_LINUX_BASELINE)
    build_target(linux-gcc-arm
        EBG
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${VSSDK_DIR}/toolchain/aarch32/gcc-arm-linux-gnueabi-8.3/toolchain.cmake
             -DVSSDK_DIR:STRING=${VSSDK_DIR}
    )
endif()

function(check_ndk)
    if(NDK_DIR AND EXISTS ${NDK_DIR}/NOTICE)
        return()
    endif()
    set(ENV_NDK_DIR "$ENV{NDK_DIR}")
    if(EXISTS ${ENV_NDK_DIR}/NOTICE)
        set(NDK_DIR ${ENV_NDK_DIR} PARENT_SCOPE)
        return()
    endif()
    message(FATAL_ERROR "Android NDK not found. Please add -DNDK_DIR=path/to/ndk to the cmake command line or set NDK_DIR environment variable. To disable use of NDK use -DENABLE_ANDROID_NDK=OFF.")    
endfunction()

if(ENABLE_ANDROID_NDK)
    check_ndk()
    build_target(armv7a-android-ndk-api31
        EBG
        SYNAP_KO SYNAP_TA
        TFLITE TFLITE_SHARED TFLITE_NNAPI
        FRAMEWORK_TOOLS OPENCV
        ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
        TIMVX VX_DELEGATE NCNN
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${NDK_DIR}/build/cmake/android.toolchain.cmake ${NDK_ARGS}
            -DANDROID_PLATFORM:STRING=android-31
            -DANDROID_ABI:STRING=armeabi-v7a
            -DANDROID_STL:STRING=c++_static
            # Don't set that as this will hide linking issue and will require runtime debugging
            #-DANDROID_ALLOW_UNDEFINED_SYMBOLS:STRING=true
    )
endif()

if(ENABLE_ANDROID64_NDK)
    check_ndk()
    build_target(aarch64-android-ndk-api31
        EBG TFLITE TFLITE_SHARED TFLITE_NNAPI SYNAP_KO
        SYNAP_TA ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
        TIMVX VX_DELEGATE NCNN
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${NDK_DIR}/build/cmake/android.toolchain.cmake ${NDK_ARGS}
            -DANDROID_PLATFORM:STRING=android-31
            -DANDROID_ABI:STRING=arm64-v8a
            -DANDROID_STL:STRING=c++_static
    )
endif()

if(ENABLE_LINUX_HOST_VSSDK)
    # FIXME: TFLITE and ONNXRT cannot be enabled for now as CI build server gcc is too old,
    # confliction link with prebuilt tensorflow and onnxrt libraries
    # Note: this target is used not only inside VSSDK but also in the Toolkit Dockerfile.
    # if we add TFLITE and ONNXRT, we need to include there all the required dependencies.
    build_target(x86_64-linux-gcc
        EBG # TFLITE TFLITE_SHARED ONNXRT ONNXRT_XNNPACK
    )
endif()

if(ENABLE_ANDROID_VSSDK)
    build_target(armv7a-android
        EBG OPENCV FRAMEWORK_TOOLS TFLITE TFLITE_SHARED ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${NDK_DIR}/build/cmake/android.toolchain.cmake
            -DANDROID_PLATFORM:STRING=android-${VSSDK_CONFIG_RUNTIME_ANDROID_API_LEVEL}
            -DANDROID_ABI:STRING=armeabi-v7a
            -DANDROID_STL:STRING=c++_static
    )
endif()

if(ENABLE_ANDROID64_NDK_VSSDK)
    check_ndk()
    build_target(aarch64-android
        EBG TFLITE TFLITE_SHARED ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${NDK_DIR}/build/cmake/android.toolchain.cmake ${NDK_ARGS}
            -DANDROID_PLATFORM:STRING=android-31
            -DANDROID_ABI:STRING=arm64-v8a
            -DANDROID_STL:STRING=c++_static
    )
endif()

function(check_oe_toolchain)
    # http://10.70.24.134:8080/source/xref/sirius/sdk/poky/meta/recipes-devtools/cmake/cmake/OEToolchainConfig.cmake
    # yocto populate varialbes by souring env file
    # same for its cmake toolchain file, which set variables from env var, proj cannot set cmake toolchain file directly
    set(TOOLCHAIN_FILE $ENV{OE_CMAKE_TOOLCHAIN_FILE})
    if(NOT EXISTS ${TOOLCHAIN_FILE})
        message(FATAL_ERROR "Poky toolchain file not found: ${TOOLCHAIN_FILE}")
    endif()
endfunction()

if(ENABLE_LINUX_YOCTO_ARMv7a)
    check_oe_toolchain()
    build_target(armv7a-oe
        EBG
        TFLITE TFLITE_SHARED TFLITE_NNAPI
        ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
        TIMVX VX_DELEGATE NCNN
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${TOOLCHAIN_FILE}
    )
endif()

if(ENABLE_LINUX_YOCTO_AARCH64)
    check_oe_toolchain()
    build_target(aarch64-oe
        EBG
        TFLITE TFLITE_SHARED TFLITE_NNAPI
        ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
        TIMVX VX_DELEGATE NCNN
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${TOOLCHAIN_FILE}
    )
endif()

if(ENABLE_LINUX_YOCTO_ARMv7a_VSSDK)
    check_oe_toolchain()
    build_target(armv7a-oe
        EBG
    )
endif()

if(ENABLE_LINUX_YOCTO_AARCH64_VSSDK)
    check_oe_toolchain()
    build_target(aarch64-oe
        EBG TFLITE TFLITE_SHARED
    )
endif()


message("\nConfiguration:\n")
message("  CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("  VSSDK_DIR: ${VSSDK_DIR}")
message("  SOC: ${SOC}")

function(print_option OPTION_NAME OPTION_FLAG)
    if(${${OPTION_FLAG}})
        message("  [x] ${OPTION_NAME} (-D${OPTION_FLAG})")
    else()
        message("  [ ] ${OPTION_NAME} (-D${OPTION_FLAG})")
    endif()
endfunction()

message("\nEnabled components:\n")
print_option("vx kernels" ENABLE_VXK)
print_option("models" ENABLE_MODELS)
print_option("doc" ENABLE_DOC)
print_option("acuity" ENABLE_ACUITY)

print_option("Linux Baseline" ENABLE_LINUX_BASELINE)
print_option("Linux Host" ENABLE_LINUX_HOST)
print_option("Android NDK" ENABLE_ANDROID_NDK)
print_option("Android 64bit NDK" ENABLE_ANDROID64_NDK)
print_option("Linux Yocto ARMv7a" ENABLE_LINUX_YOCTO_ARMv7a)
print_option("Linux Yocto AARCH64" ENABLE_LINUX_YOCTO_AARCH64)

print_option("VSSDK Linux Host" ENABLE_LINUX_HOST_VSSDK)
print_option("VSSDK Android" ENABLE_ANDROID_VSSDK)
print_option("VSSDK Android 64bit NDK" ENABLE_ANDROID64_NDK_VSSDK)
print_option("VSSDK Linux Yocto ARMv7a" ENABLE_LINUX_YOCTO_ARMv7a_VSSDK)
print_option("VSSDK Linux Yocto AARCH64" ENABLE_LINUX_YOCTO_AARCH64_VSSDK)

message("")
