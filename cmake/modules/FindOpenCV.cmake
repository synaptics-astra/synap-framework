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

message("OpenCV:\t\tenabled")

# Note: declararation order of library dependency is important for gcc, please don´t change them.
# For example libopencv_core.a depends on zlib so z must be declared after.

add_library(opencv_core INTERFACE)
add_dependencies(opencv_core opencv)
target_include_directories(opencv_core INTERFACE ${SYNAP_SYSROOT_INCLUDEDIR})
target_link_libraries(opencv_core INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_core.a
    z
)

if(ANDROID)
    target_link_libraries(opencv_core INTERFACE z)
else()
    target_link_libraries(opencv_core INTERFACE z pthread dl)
endif()

add_library(opencv_imgproc INTERFACE)
target_link_libraries(opencv_imgproc INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_imgproc.a
    opencv_core
)
add_library(opencv_calib3d INTERFACE)
target_link_libraries(opencv_calib3d INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_calib3d.a
    opencv_imgproc opencv_flann
)
add_library(opencv_features2d INTERFACE)
target_link_libraries(opencv_features2d INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_features2d.a
    opencv_flann
)
add_library(opencv_flann INTERFACE)
target_link_libraries(opencv_flann INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_flann.a
    opencv_core
)
add_library(opencv_imgcodecs INTERFACE)
target_link_libraries(opencv_imgcodecs INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_imgcodecs.a
    opencv_imgproc
)

add_library(opencv_objdetect INTERFACE)
target_link_libraries(opencv_objdetect INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_objdetect.a
    opencv_imgproc opencv_calib3d
    #opencv_dnn
)
add_library(opencv_photo INTERFACE)
target_link_libraries(opencv_photo INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_photo.a
    opencv_imgproc
)
add_library(opencv_stitching INTERFACE)
target_link_libraries(opencv_stitching INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_stitching.a
    opencv_imgproc opencv_features2d opencv_calib3d opencv_flann
)
add_library(opencv_video INTERFACE)
target_link_libraries(opencv_video INTERFACE
    ${SYNAP_SYSROOT_LIBDIR}/libopencv_video.a
    opencv_imgproc
    opencv_calib3d
    #opencv_dnn
)

if(ENABLE_OPENCV_JPEG)
    message("OpenCV JPEG:\tenabled")
    set(libjpeg ${SYNAP_SYSROOT_LIBDIR}/liblibjpeg-turbo.a)
    target_link_libraries(opencv_imgcodecs INTERFACE ${libjpeg})
endif()

if(ENABLE_OPENCV_PNG)
    message("OpenCV PNG:\tenabled")
    set(libpng ${SYNAP_SYSROOT_LIBDIR}/liblibpng.a)
    target_link_libraries(opencv_imgcodecs INTERFACE ${libpng})
endif()
