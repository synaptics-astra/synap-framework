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

function(parse_config cfg_file key_prefix debug)

    file(STRINGS ${cfg_file} cfg REGEX "^[ ]*[^#].+$")
    if(debug)
#        string(REPLACE ";"  "\n" str "${cfg}")
#        message("VSSDK_BUILD_CONFIG ${str}")
    endif()

    foreach(item IN LISTS cfg)
        string(FIND ${item} "=" pos)
        string(SUBSTRING ${item} 0 ${pos} key)
        math(EXPR pos2 "${pos} + 1")
        string(SUBSTRING ${item} ${pos2} -1 val)
        set(key "${key_prefix}_${key}")
        if(val STREQUAL "y")
            set("${key}" ON CACHE INTERNAL "" FORCE)
        else()
            string(REGEX REPLACE "\"([^\"]+)\"" "\\1" val ${val})
            set("${key}" ${val} CACHE INTERNAL "" FORCE)
        endif()
        if(debug)
            message("${key}\t\t${val}")
        endif()
    endforeach()
endfunction()
