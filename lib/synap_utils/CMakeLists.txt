#
# NDA AND NEED-TO-KNOW REQUIRED
#
# Copyright (C) 2013-20202 Synaptics Incorporated. All rights reserved.
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

option(SYNAP_LOGS_TO_STDERR "Enable synap logging messages to stderr" OFF)

set(name synap_utils)

file(GLOB SRC
    src/file_utils.cpp
    src/string_utils.cpp
    src/zip_tool.cpp
    src/bundle_parser_zip.cpp
    src/bundle_parser.cpp
    )

if(ENABLE_FILE_BASED_BUNDLE)
    list(APPEND SRC src/bundle_parser_dir.cpp)
endif()

add_library(${name} STATIC ${SRC})

target_include_directories(${name}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${name} PRIVATE miniz nlohmann)

if(ENABLE_CXX_STD_FILESYSTEM)
    target_compile_definitions(${name} PUBLIC ENABLE_STD_FILESYSTEM)
endif()

if(SYNAP_LOGS_TO_STDERR)
    target_compile_definitions(${name} PUBLIC SYNAP_LOGS_TO_STDERR)
endif()

if(ANDROID)
    target_link_libraries(${name} PRIVATE log)
endif()

file(GLOB HEADERS inc/synap/*.hpp)
set_target_properties(${name} PROPERTIES PUBLIC_HEADER "${HEADERS}")
install(TARGETS ${name})
