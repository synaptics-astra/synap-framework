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
#
# ////////////////////////////////////////////////////////////////////////////////
# //! \file CMakeLists.txt
# //!
# //! \brief 
# //!
# //!              Version   Date                                 Author
# //!              V 0.01,   Feb 3 2020                           alek
# //!
# //! Note:
# ////////////////////////////////////////////////////////////////////////////////
#
cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

set(projectname "benchmark_model")

PROJECT(${projectname})

ExternalProject_Add(${projectname}
  GIT_REPOSITORY    https://github.com/VeriSilicon/tensorflow.git
  GIT_TAG           nnapi-delegate-2.0
  GIT_SHALLOW       ON
  SOURCE_DIR        ""
  BUILD_ALWAYS      OFF
  CONFIGURE_COMMAND <SOURCE_DIR>/tensorflow/lite/tools/make/download_dependencies.sh
  COMMAND           cp ${CMAKE_CURRENT_SOURCE_DIR}/as371_makefile.inc <SOURCE_DIR>/tensorflow/lite/tools/make/targets
  BUILD_COMMAND     $(MAKE) TARGET_TOOLCHAIN_ROOT=/opt/bin/toolchain/$ENV{TOOLCHAIN_NAME} TARGET_TOOLCHAIN_PREFIX=${TOOLCHAIN_PREFIX} TARGET=${TARGET_OS} BUILD_WITH_NNAPI=true -C <SOURCE_DIR> -f tensorflow/lite/tools/make/Makefile
  INSTALL_COMMAND   mkdir -p /opt/target/bin
  COMMAND           cp <SOURCE_DIR>/tensorflow/lite/tools/make/gen/as371_armv7-a/bin/benchmark_model /opt/target/bin
  TEST_COMMAND      ""
  )

install(CODE "MESSAGE(\"benchmark_model installed.\")")
