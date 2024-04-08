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
#

add_executable(zip_tool_test zip_tool/main.cpp)

target_link_libraries(zip_tool_test synap_utils synap_app)

set(source ${CMAKE_CURRENT_LIST_DIR}/zip_tool/data)

add_test(NAME ZipTool-test-cleanup COMMAND rm -rf z0 z1 z9 zfile zmem test*.zip)

add_test(NAME zip-data.0 COMMAND zip -r -0 ${CMAKE_CURRENT_BINARY_DIR}/test0.zip . WORKING_DIRECTORY ${source})
add_test(NAME zip-data.1 COMMAND zip -r -1 ${CMAKE_CURRENT_BINARY_DIR}/test1.zip . WORKING_DIRECTORY ${source})
add_test(NAME zip-data.9 COMMAND zip -r -9 ${CMAKE_CURRENT_BINARY_DIR}/test9.zip . WORKING_DIRECTORY ${source})
set_tests_properties(zip-data.0 PROPERTIES DEPENDS "ZipTool-test-cleanup")
set_tests_properties(zip-data.1 PROPERTIES DEPENDS "ZipTool-test-cleanup")
set_tests_properties(zip-data.9 PROPERTIES DEPENDS "ZipTool-test-cleanup")

add_test(NAME ZipTool-extract-all.0 COMMAND zip_tool_test -m 0 -f test0.zip -o z0)
add_test(NAME ZipTool-extract-all-check.0 COMMAND diff -r ${source} z0)
set_tests_properties(ZipTool-extract-all.0 PROPERTIES DEPENDS "zip-data.0")
set_tests_properties(ZipTool-extract-all-check.0 PROPERTIES DEPENDS "ZipTool-extract-all.0")

add_test(NAME ZipTool-extract-all.1 COMMAND zip_tool_test -m 0 -f test1.zip -o z1)
add_test(NAME ZipTool-extract-all-check.1 COMMAND diff -r ${source} z1)
set_tests_properties(ZipTool-extract-all.1 PROPERTIES DEPENDS "zip-data.1")
set_tests_properties(ZipTool-extract-all-check.1 PROPERTIES DEPENDS "ZipTool-extract-all.1")

add_test(NAME ZipTool-extract-all.9 COMMAND zip_tool_test -m 0 -f test9.zip -o z9)
add_test(NAME ZipTool-extract-all-check.9 COMMAND diff -r ${source} z9)
set_tests_properties(ZipTool-extract-all.9 PROPERTIES DEPENDS "zip-data.9")
set_tests_properties(ZipTool-extract-all-check.9 PROPERTIES DEPENDS "ZipTool-extract-all.9")

add_test(NAME ZipTool-extract-mem-to-file COMMAND zip_tool_test -m 1 -f test0.zip -o zfile)
add_test(NAME ZipTool-extract-mem-to-file-check COMMAND diff -r ${source} zfile)
set_tests_properties(ZipTool-extract-mem-to-file PROPERTIES DEPENDS "zip-data.0")
set_tests_properties(ZipTool-extract-mem-to-file-check PROPERTIES DEPENDS "ZipTool-extract-mem-to-file")

# warning: ZipTool-extract-mem-to-mem completely ignore empty directories so this test can only work when
# there is no empty directory for folder ${source}
add_test(NAME ZipTool-extract-mem-to-mem COMMAND zip_tool_test -m 2 -f test0.zip -o zmem)
add_test(NAME ZipTool-extract-mem-to-mem-check COMMAND diff -r ${source} zmem)
set_tests_properties(ZipTool-extract-mem-to-mem PROPERTIES DEPENDS "zip-data.0")
set_tests_properties(ZipTool-extract-mem-to-mem-check PROPERTIES DEPENDS "ZipTool-extract-mem-to-mem")
