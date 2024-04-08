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

set(SYNAP_USED_ONNXRT_SHARED_LIB ON)

add_library(onnxruntime INTERFACE)
target_include_directories(onnxruntime INTERFACE ${SYNAP_SYSROOT_INCLUDEDIR}/onnxruntime)

if(SYNAP_USED_ONNXRT_SHARED_LIB)
    target_link_libraries(onnxruntime INTERFACE ${SYNAP_SYSROOT_LIBDIR}/libonnxruntime.so)
else()

    # Warning not tested yet and will conflict with tflite

    target_link_libraries(onnxruntime INTERFACE
        libonnx.a
        libonnx_proto.a
        libprotobuf.a
        libabsl_base.a
        libabsl_log_severity.a
        libabsl_malloc_internal.a
        libabsl_raw_logging_internal.a
        libabsl_spinlock_wait.a
        libabsl_throw_delegate.a
        libabsl_city.a
        libabsl_hash.a
        libabsl_low_level_hash.a
        libabsl_raw_hash_set.a
        libabsl_hashtablez_sampler.a
        libre2.a
        libpthreadpool.a
        libXNNPACK.a
        libcpuinfo.a
        libclog.a
        libnsync_cpp.a

        libonnxruntime_flatbuffers.a
        libonnxruntime_common.a
        libonnxruntime_mlas.a
        libonnxruntime_graph.a
        libonnxruntime_framework.a
        libonnxruntime_util.a
        libonnxruntime_providers.a
        libonnxruntime_providers_xnnpack.a
        libonnxruntime_optimizer.a
        libonnxruntime_session.a
        )

        if(ANDROID)
            target_link_libraries(onnxruntime INTERFACE
                libonnxruntime_providers_nnapi.a
            )
        endif()
endif()

