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

if(NOT VSSDK_DIR)
    message(FATAL_ERROR "VSSDK_DIR is mandatory")
endif()

# Convert to Absolute path just in case...
get_filename_component(VSSDK_DIR_ABS_ ${VSSDK_DIR} ABSOLUTE)
if(NOT EXISTS ${VSSDK_DIR_ABS_})
    message(FATAL_ERROR "missing ${VSSDK_DIR_ABS_}")
endif()
set(VSSDK_DIR_ABS ${VSSDK_DIR_ABS_} CACHE PATH "..." FORCE)

set(VSSDK_AVAILABLE TRUE)

set(AMPSDK_DIR ${VSSDK_DIR_ABS}/ampsdk)
if(NOT EXISTS ${AMPSDK_DIR})
    message(FATAL_ERROR "missing ${AMPSDK_DIR}")
endif()

if(NOT VSSDK_CONFIG_FILE)
    set(VSSDK_CONFIG_FILE ${VSSDK_DIR_ABS}/out/.config CACHE PATH "..." FORCE)
    if(NOT EXISTS ${VSSDK_CONFIG_FILE})
        message(FATAL_ERROR "missing ${VSSDK_CONFIG_FILE}\nSyNAP needs a VSSDK build tree\n")
    endif()
endif()

include(parse_config)
parse_config(${VSSDK_CONFIG_FILE} "VSSDK" OFF)

message("VSSDK_CONFIG_PRODUCT_NAME ${VSSDK_CONFIG_PRODUCT_NAME}")
message("VSSDK_CONFIG_TA_REL_PATH ${VSSDK_CONFIG_TA_REL_PATH}")
message("VSSDK_CONFIG_AMP_VERSION_STRING ${VSSDK_CONFIG_AMP_VERSION_STRING}")
message("VSSDK_CONFIG_TA_REL_PATH ${VSSDK_CONFIG_TA_REL_PATH}")
message("VSSDK_CONFIG_LINUX_SRC_PATH ${VSSDK_CONFIG_LINUX_SRC_PATH}")

set(VSSDK_BUILD_DIR ${VSSDK_DIR_ABS}/out/${VSSDK_CONFIG_PRODUCT_NAME}/build CACHE INTERNAL "" FORCE)
set(VSSDK_BUILD_TARGET_DIR ${VSSDK_DIR_ABS}/out/${VSSDK_CONFIG_PRODUCT_NAME}/target CACHE INTERNAL "" FORCE)
set(VSSDK_BUILD_REL_DIR ${VSSDK_DIR_ABS}/../syna-release CACHE INTERNAL "" FORCE)

set(VSSDK_SECURITY_MODULE EXISTS "${VSSDK_BUILD_DIR}/SECURITY/.stamp_target_built")
set(VSSDK_TEE_DEV_MODULE EXISTS "${VSSDK_BUILD_DIR}/TEE_DEV/.stamp_target_built")

if("${BUILD_SYSROOT}" STREQUAL "")
    # Check if VSSDK was built
    set(BUILD_SYSROOT_ABS ${VSSDK_BUILD_TARGET_DIR}/${VSSDK_CONFIG_SYNA_SDK_OUT_SYSYROOT} CACHE INTERNAL "" FORCE)
else()
    # BUILD_SYSROOT passed to cmake, make sure it is absolute
    get_filename_component(BUILD_SYSROOT_ABS ${BUILD_SYSROOT} ABSOLUTE)
endif()

set(VSSDK_CLANG_TOOLCHAIN_DIR ${VSSDK_DIR_ABS}/toolchain/${VSSDK_CONFIG_TOOLCHAIN_CLANG_PATH} CACHE INTERNAL "" FORCE)
set(VSSDK_SYSROOT ${BUILD_SYSROOT_ABS})
set(ANDROID_TOOLCHAIN_SYSROOT ${VSSDK_DIR_ABS}/sysroot/android/sysroot CACHE INTERNAL "" FORCE)
set(GCC_TOOLCHAIN_DIR ${VSSDK_DIR_ABS}/toolchain/${VSSDK_CONFIG_TOOLCHAIN_APPLICATION_PATH} CACHE INTERNAL "" FORCE)

if(${VSSDK_CONFIG_LINUX_SRC_PATH} MATCHES "5_15")
    set(VSSDK_KERNEL_DMABUF_ENABLED ON CACHE INTERNAL "" FORCE)
else()
    set(VSSDK_KERNEL_DMABUF_ENABLED OFF CACHE INTERNAL "" FORCE)
endif()

# Linux kernel and modules compilation support
set(VSSDK_LINUX_SRC_DIR ${VSSDK_DIR_ABS}/${VSSDK_CONFIG_LINUX_SRC_PATH})
set(VSSDK_LINUX_TOOLCHAIN_DIR ${VSSDK_DIR_ABS}/toolchain/${VSSDK_CONFIG_LINUX_CROSS_COMPILE_PATH}/bin)
set(VSSDK_LINUX_CC_DIR ${VSSDK_DIR_ABS}/toolchain/${VSSDK_CONFIG_TOOLCHAIN_CLANG_PATH}/bin)
set(kernel_cmd_env "PATH=$ENV{PATH}:${VSSDK_LINUX_TOOLCHAIN_DIR}:${VSSDK_LINUX_CC_DIR}")

# FIXME: If/when we switch to LLVM utils we need to pass LLVM=1 here (see vssdk/build/linux/build.sh)
set(VSSDK_LINUX_CMD_PREFIX ${CMAKE_COMMAND} -E env ${kernel_cmd_env} make  CC=${VSSDK_CONFIG_TOOLCHAIN_APPLICATION_CC} ARCH=${VSSDK_CONFIG_LINUX_ARCH} CROSS_COMPILE=${VSSDK_CONFIG_LINUX_CROSS_COMPILE} O=${VSSDK_BUILD_TARGET_DIR}/${VSSDK_CONFIG_LINUX_REL_PATH}/intermediate)

# Linux kernel compilation
add_custom_target(vssdk_kernel
    COMMAND ${VSSDK_LINUX_CMD_PREFIX} ${VSSDK_CONFIG_LINUX_DEFCONFIG}
    COMMAND ${VSSDK_LINUX_CMD_PREFIX} Image modules -j 8
    WORKING_DIRECTORY ${VSSDK_LINUX_SRC_DIR}
    )

# Documentation: Out of tree kernel module compilation can be integrated like this:
#   add_custom_target(<module target name>
#      COMMAND ${VSSDK_LINUX_CMD_PREFIX} -C ${VSSDK_LINUX_SRC_DIR} M=<path/to/module> modules -j 8
#      WORKING_DIRECTORY ${VSSDK_LINUX_SRC_DIR}
#      )


if(EXISTS ${BUILD_SYSROOT_ABS})
    message("Found BUILD_SYSROOT: ${BUILD_SYSROOT_ABS}")
    set(BUILD_SYSROOT_AVAILABLE TRUE)
    set(AMP_INCLUDE_DIRS
        ${BUILD_SYSROOT_ABS}/include
        ${BUILD_SYSROOT_ABS}/usr/include/amp
        )

    set(OSAL_LIBRARIES ${BUILD_SYSROOT_ABS}/usr/lib/libOSAL.so)
    set(TEEC_LIBRARIES ${BUILD_SYSROOT_ABS}/usr/lib/libteec.so)

    set(AMPCLIENT_LIBRARIES
        ${BUILD_SYSROOT_ABS}/usr/lib/libampclient.so
        ${OSAL_LIBRARIES}
        )

    set(SHMSRV_LIBRARIES ${BUILD_SYSROOT_ABS}/usr/lib/libshmserver.so)

elseif("${BUILD_SYSROOT}" STREQUAL "")
    message(WARNING "BUILD_SYSROOT not available.")
else()
    message(FATAL_ERROR "BUILD_SYSROOT not found: ${BUILD_SYSROOT_ABS}")
endif()

set(OSAL_INCLUDE_DIRS ${AMPSDK_DIR}/osal/include)

list(APPEND AMP_INCLUDE_DIRS
    ${AMPSDK_DIR}/amp/inc
    ${AMPSDK_DIR}/amp/tools/flick/flick-2.1/runtime/headers/
    ${AMPSDK_DIR}/amp/src/hal/common/include
    ${AMPSDK_DIR}/amp/src/hal/common/include/Firmware_Berlin_VS680_A0  # For mc_performance.h
    ${OSAL_INCLUDE_DIRS}
    )

set(TEEC_INCLUDE_DIRS
    ${VSSDK_DIR_ABS}/tee/tee/include
    ${VSSDK_DIR_ABS}/tee/tee/tee/common
    ${VSSDK_DIR_ABS}/tee/tee/tee/client_api
    )

add_library(osal INTERFACE)
target_include_directories(osal INTERFACE ${OSAL_INCLUDE_DIRS})
target_link_libraries(osal INTERFACE ${OSAL_LIBRARIES})

add_library(ampclient INTERFACE)
target_include_directories(ampclient INTERFACE ${AMP_INCLUDE_DIRS})
target_link_libraries(ampclient INTERFACE ${AMPCLIENT_LIBRARIES})

add_library(teec INTERFACE)
target_include_directories(teec INTERFACE ${TEEC_INCLUDE_DIRS})
target_link_libraries(teec INTERFACE ${TEEC_LIBRARIES})

add_library(shmserver INTERFACE)
target_include_directories(shmserver INTERFACE ${AMP_INCLUDE_DIRS})
target_link_libraries(shmserver INTERFACE ${SHMSRV_LIBRARIES})

if(DEFINED VSSDK_CONFIG_BERLIN_VS680_A0)
    set(SOC VS680A0 CACHE STRING "Target SOC")
elseif(DEFINED VSSDK_CONFIG_BERLIN_VS640_A0)
    set(SOC VS640A0 CACHE STRING "Target SOC")
else()
    set(SOC VS640A0 VS680A0 CACHE STRING "Target SOC")
endif()

if(${VSSDK_CONFIG_TOOLCHAIN_APPLICATION} MATCHES "aarch64")
    set(ENABLE_64 1 CACHE INTERNAL "Enable 64bit compilation")
else()
    set(ENABLE_64 0 CACHE INTERNAL "Enable 64bit compilation")
endif()
