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



function(build_target TARGET_NAME)

    set(TARGET_OPTIONS
        FRAMEWORK_TOOLS     # framework/tools compilation
        EBG                 # NPU EBG runtime support
        SYNAP_KO            # NPU kernel driver
        SYNAP_TA            # NPU TA driver
        MODELS              # models compilation
        OPENCV              # OpenCV compilation
        ONNXRT              # ONNX runtime compilation
        ONNXRT_STATIC_LIB   # ONNX runtime static library
        ONNXRT_XNNPACK      # ONNX runtime XNNPACK
        ONNXRT_NNAPI        # ONNX runtime NNAPI
        TFLITE              # TFLite runtime static library
        TFLITE_SHARED       # Build fat shared library libtensorflow-lite.so
        TFLITE_NNAPI        # TFLite runtime NNAPI
        TIMVX               # VSI TIM-VX
        VX_DELEGATE         # TFLITE VX Delegate
        NCNN                # NCNN runtime
    )
    cmake_parse_arguments(PARSE_ARGV 1 BUILD
        "${TARGET_OPTIONS}"
        "" # one value keywords
        "ARGS") # mult value keywords

    message("TARGET: ${TARGET_NAME}")
    message("\tMODELS: ${BUILD_MODELS}\tFRAMEWORK_TOOLS: ${BUILD_FRAMEWORK_TOOLS}\t
        TFLITE: ${BUILD_TFLITE}\tONNXRT: ${BUILD_ONNXRT}\t
        TIMVX: ${BUILD_TIMVX} \tVX_DELEGATE: ${BUILD_VX_DELEGATE}")

    set(SYNAP_INSTALL_LIBDIR lib/${TARGET_NAME})
    set(SYNAP_INSTALL_BINDIR bin/${TARGET_NAME})

    set(SYNAP_ARGS ${BUILD_ARGS}
        #-DCMAKE_POSITION_INDEPENDENT_CODE:STRING=ON
        -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_INSTALL_LIBDIR:STRING=${SYNAP_INSTALL_LIBDIR}
        -DCMAKE_INSTALL_BINDIR:STRING=${SYNAP_INSTALL_BINDIR}
        -DCMAKE_INSTALL_INCLUDEDIR:STRING=${CMAKE_INSTALL_INCLUDEDIR}
        -DCMAKE_INSTALL_MESSAGE:STRING=LAZY
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DENABLE_CPU_PROFILING:STRING=${ENABLE_CPU_PROFILING}
        )

    if(NOT EXISTS ${OPENCV_DIR})
        set(BUILD_OPENCV OFF)
    endif()

    if(BUILD_EBG)
        list(APPEND framework-deps npu_driver-${TARGET_NAME})
        set(npu_driver_ARGS ${SYNAP_ARGS}
            -DVSSDK_DIR:PATH=${VSSDK_DIR}
            -DENABLE_NPU_DRIVER_PRIVATE:STRING=ON
            -DENABLE_SYNAP_KO:STRING=ON
            -DENABLE_SYNAP_DEVICE_SIMULATOR:STRING=${ENABLE_SYNAP_DEVICE_SIMULATOR})

        ExternalProject_Add(npu_driver-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            SOURCE_DIR ${NPU_DRIVER_DIR}
            CMAKE_CACHE_ARGS ${npu_driver_ARGS})

        if(BUILD_SYNAP_KO)
            set(synap_ko_ARGS ${SYNAP_ARGS}
                -DVSSDK_DIR:PATH=${VSSDK_DIR}
                # FIXME: Needed for FindVSSDK
                -DFRAMEWORK_DIR:PATH=${FRAMEWORK_DIR})

            ExternalProject_Add(synap_ko-${TARGET_NAME}
                BUILD_ALWAYS TRUE
                USES_TERMINAL_BUILD TRUE
                SOURCE_DIR ${NPU_DRIVER_DIR}/kernel
                CMAKE_CACHE_ARGS ${synap_ko_ARGS})
        endif()
        if(BUILD_SYNAP_TA)
            set(synap_ta_ARGS ${SYNAP_ARGS}
                -DVSSDK_DIR:PATH=${VSSDK_DIR}
                # FIXME: Needed for FindVSSDK
                -DFRAMEWORK_DIR:PATH=${FRAMEWORK_DIR})
            ExternalProject_Add(synap_ta-${TARGET_NAME}
                BUILD_ALWAYS TRUE
                USES_TERMINAL_BUILD TRUE
                SOURCE_DIR ${NPU_DRIVER_DIR}/private/ta
                CMAKE_CACHE_ARGS ${synap_ta_ARGS})
        endif()
    endif() # BUILD_EBG

    if(EXISTS ${NPU_SW_STACK_DIR})
        ExternalProject_Add(npu_sw_stack-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            SOURCE_DIR ${NPU_SW_STACK_DIR}
            CMAKE_CACHE_ARGS ${SYNAP_ARGS})
    endif()

    if(BUILD_OPENCV)
        set(opencv_args ${SYNAP_ARGS})
        include(opencv_args)
        ExternalProject_Add(opencv-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            SOURCE_DIR ${OPENCV_DIR}
            CMAKE_ARGS ${opencv_args})
    endif()

    if(BUILD_TFLITE AND EXISTS ${TFLITE_DIR}/CMakeLists.txt)
        list(APPEND framework-deps tflite-${TARGET_NAME})
        set(tflite_args ${SYNAP_ARGS})
        include(tflite_args)
        ExternalProject_Add(tflite-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_CONFIGURE TRUE
            SOURCE_DIR ${TFLITE_DIR}
            CMAKE_ARGS ${tflite_args}
            UPDATE_COMMAND ${CMAKE_COMMAND} -E touch <BINARY_DIR>/CMakeCache.txt
            BUILD_COMMAND ${CMAKE_COMMAND} --build  <BINARY_DIR> --config $<CONFIG>
            # benchmark_model is not build by default
            COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG> --target benchmark_model
            INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/${SYNAP_INSTALL_LIBDIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/${SYNAP_INSTALL_BINDIR}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${tflite_libs} ${CMAKE_INSTALL_PREFIX}/${SYNAP_INSTALL_LIBDIR}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${tflite_tools} ${CMAKE_INSTALL_PREFIX}/${SYNAP_INSTALL_BINDIR}
            # FIXME Flatbuffers compilation and installation should be done independently
            COMMAND ${CMAKE_COMMAND} -E copy_directory <BINARY_DIR>/flatbuffers/include/flatbuffers ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/flatbuffers
        )
    endif()

    if(BUILD_ONNXRT AND EXISTS ${ONNXRT_DIR}/CMakeLists.txt)
        list(APPEND framework-deps onnxrt-${TARGET_NAME})
        set(onnxrt_args ${SYNAP_ARGS})
        include(onnxrt_args)
        ExternalProject_Add(onnxrt-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_CONFIGURE TRUE
            SOURCE_DIR ${ONNXRT_DIR}
            CMAKE_ARGS ${onnxrt_args})
        if(BUILD_ONNXRT_NNAPI)
            ExternalProject_Add_Step(onnxrt-${TARGET_NAME} install_nnapi_header
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${ONNXRT_DIR}/../include/onnxruntime/core/providers/nnapi ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/onnxruntime)
        endif()
    endif()

    set(framework_ARGS ${SYNAP_ARGS}
        # only needed for private allocator
        #-DVSSDK_DIR:PATH=${VSSDK_DIR}
        -DENABLE_TEST:STRING=ON
        -DENABLE_SYNAP_DEVICE_SIMULATOR:STRING=${ENABLE_SYNAP_DEVICE_SIMULATOR}
        -DENABLE_EBGRUNTIME:STRING=${BUILD_EBG}
        -DENABLE_ONNXRUNTIME:STRING=${BUILD_ONNXRT}
        -DENABLE_TFLITERUNTIME:STRING=${BUILD_TFLITE}
        )

    ExternalProject_Add(framework-${TARGET_NAME}
        BUILD_ALWAYS TRUE
        USES_TERMINAL_BUILD true
        DEPENDS ${framework-deps}
        SOURCE_DIR ${FRAMEWORK_DIR}
        CMAKE_CACHE_ARGS ${framework_ARGS})

    # framework_tools builds opencv dependent modules if its build enable
    # otherwise only build modules without opencv
    if(BUILD_FRAMEWORK_TOOLS AND EXISTS ${FRAMEWORK_DIR}/tools)
        set(tools_ARGS ${SYNAP_ARGS}
            -DVSSDK_DIR:PATH=${VSSDK_DIR}
            -DENABLE_OPENCV:STRING=${BUILD_OPENCV}
        )

        set(framework_tools_deps framework-${TARGET_NAME})
        if(BUILD_OPENCV)
            list(APPEND framework_tools_deps opencv-${TARGET_NAME})
        endif()

        ExternalProject_Add(framework-tools-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD true
            DEPENDS ${framework_tools_deps}
            SOURCE_DIR ${FRAMEWORK_DIR}/tools
            CMAKE_CACHE_ARGS ${tools_ARGS})
    endif()

    if(ENABLE_MODELS AND BUILD_MODELS)
        set(models_ARGS
            -DVSSDK_DIR:PATH=${VSSDK_DIR}
            -DSOC:STRING=${SOC}
            -DVIP_SDK_DIR:PATH=${CMAKE_INSTALL_PREFIX}
            -DSYNAP_TOOLKIT_DIR:PATH=${FRAMEWORK_DIR}/toolkit
            -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_INSTALL_PREFIX}
            -DCMAKE_INSTALL_MESSAGE:STRING=LAZY)
        ExternalProject_Add(models
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD true
            DEPENDS vxk npu_sw_stack-${TARGET_NAME} framework-${TARGET_NAME}
            SOURCE_DIR ${MODELS_DIR}
            CMAKE_CACHE_ARGS ${models_ARGS})
    endif()

    if(BUILD_TIMVX AND EXISTS ${TIMVX_DIR}/CMakeLists.txt)
        set(timvx_args ${SYNAP_ARGS}
            -DTIM_VX_ENABLE_SYNAP:STRING=ON
            -DTIM_VX_USE_EXTERNAL_OVXLIB:STRING=ON
            -DTIM_VX_ENABLE_VIPLITE:STRING=OFF
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DSYNAP_LIBDIR:PATH=${CMAKE_INSTALL_PREFIX}/${SYNAP_INSTALL_LIBDIR}
            -DSYNAP_INCDIR:PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
        )
        ExternalProject_Add(timvx-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_CONFIGURE TRUE
            DEPENDS npu_sw_stack-${TARGET_NAME} framework-${TARGET_NAME}
            SOURCE_DIR ${TIMVX_DIR}
            CMAKE_ARGS ${timvx_args})
    endif()

    if(BUILD_VX_DELEGATE AND EXISTS ${VXDELEGATE_DIR}/CMakeLists.txt AND
       BUILD_TIMVX AND BUILD_TFLITE)
        set(vx_delegate_args ${SYNAP_ARGS}
            -DTIM_VX_LIB:PATH=${CMAKE_INSTALL_PREFIX}/${SYNAP_INSTALL_LIBDIR}/libtim-vx.so
            -DTIM_VX_INC:PATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}
            -DTFLITE_BINARY_DIR=${TFLITE_BUILD_DIR}
            -DTF_SOURCES_DIR=${TFLITE_DIR}/../..
        )
        ExternalProject_Add(vx_delegate-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_CONFIGURE TRUE
            DEPENDS timvx-${TARGET_NAME} tflite-${TARGET_NAME}
            SOURCE_DIR ${VXDELEGATE_DIR}
            CMAKE_ARGS ${vx_delegate_args})
    endif()

    if(BUILD_NCNN AND EXISTS ${NCNN_DIR}/CMakeLists.txt)
        set(ncnn_args ${SYNAP_ARGS}
            -DNCNN_SHARED_LIB=OFF
            -DNCNN_BUILD_EXAMPLES=OFF
            -DNCNN_BUILD_TESTS=OFF
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON)

        if(NOT CMAKE_CROSSCOMPILING)
            list(APPEND ncnn_args -DNCNN_OPENMP=OFF)
        endif()

        # FIXME: yocto toolchain sysroot doesn't has vulkan library
        # if(${TARGET_NAME} MATCHES "android|armv7a|aarch64")
        if(${TARGET_NAME} MATCHES "android")
            list(APPEND ncnn_args -DNCNN_VULKAN=ON -DANDROID_ARM_NEON=ON)
        endif()

        ExternalProject_Add(ncnn-${TARGET_NAME}
            BUILD_ALWAYS TRUE
            USES_TERMINAL_BUILD TRUE
            USES_TERMINAL_CONFIGURE TRUE
            SOURCE_DIR ${NCNN_DIR}
            CMAKE_ARGS ${ncnn_args})
    endif()

endfunction()
