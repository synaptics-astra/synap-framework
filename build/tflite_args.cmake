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

if(NOT EXISTS ${TFLITE_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "Cannot find ${TFLITE_DIR}/CMakeLists.txt")
endif()

# we use our own fork of xnnpack to patch the issue https://github.com/google/XNNPACK/issues/4775 on aarch32
list(APPEND tflite_args
    -DFETCHCONTENT_SOURCE_DIR_XNNPACK=${TFLITE_DIR}/../../../xnnpack/
)

list(APPEND tflite_args
    -DTFLITE_ENABLE_INSTALL:STRING=OFF
    -DTFLITE_BUILD_SHARED_LIB_FAT=${BUILD_TFLITE_SHARED}
    # This is enabled by default on Android only, check if needed for Yocto.
    #-DTFLITE_ENABLE_RUY:STRING=ON
    -DTFLITE_ENABLE_NNAPI:STRING=${BUILD_TFLITE_NNAPI}
    -DTFLITE_ENABLE_GPU:STRING=ON
    -DTFLITE_ENABLE_XNNPACK:STRING=ON
    -DXNNPACK_ENABLE_ARM_BF16:STRING=OFF
)

# FIXME: OE 32bit toolchain need change OECORE_TARGET_ARCH=arm to armv7
# to fix cpuinfo CMAKE_SYSTEM_PROCESSOR cannot find issue
if(${TARGET_NAME} MATCHES "armv7a-oe")
    list(APPEND tflite_args
        # oe toolchain 32bit build issue like https://github.com/google/XNNPACK/issues/1465
        # -DXNNPACK_ENABLE_ARM_I8MM=OFF cannot fix, so disable xnnpack
        # as yocto is 64bit system, in the end only 64bit lib will be used
        # we build 32bit just because sirius still need 32bit lib
        -DTFLITE_ENABLE_XNNPACK:STRING=OFF
    )
endif()


set(TFLITE_BUILD_DIR ${CMAKE_BINARY_DIR}/tflite-${TARGET_NAME}-prefix/src/tflite-${TARGET_NAME}-build)

set(tflite_tools ${TFLITE_BUILD_DIR}/tools/benchmark/benchmark_model)

if (BUILD_TFLITE_SHARED)
    set(tflite_libs ${TFLITE_BUILD_DIR}/libtensorflow-lite.so)
    return()
endif()

set(tflite_libs
    ${TFLITE_BUILD_DIR}/libtensorflow-lite.a

    ${TFLITE_BUILD_DIR}/_deps/xnnpack-build/libXNNPACK.a
    ${TFLITE_BUILD_DIR}/_deps/farmhash-build/libfarmhash.a
    ${TFLITE_BUILD_DIR}/_deps/flatbuffers-build/libflatbuffers.a
    ${TFLITE_BUILD_DIR}/_deps/fft2d-build/libfft2d_fftsg2d.a
    ${TFLITE_BUILD_DIR}/_deps/fft2d-build/libfft2d_fftsg.a
    ${TFLITE_BUILD_DIR}/_deps/gemmlowp-build/libeight_bit_int_gemm.a
    ${TFLITE_BUILD_DIR}/_deps/cpuinfo-build/libcpuinfo_internals.a
    ${TFLITE_BUILD_DIR}/_deps/cpuinfo-build/libcpuinfo.a
    ${TFLITE_BUILD_DIR}/pthreadpool/libpthreadpool.a
)

list(APPEND tflite_libs
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_log_severity.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_throw_delegate.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_scoped_set_env.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_strerror.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/hash/libabsl_hash.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/hash/libabsl_city.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/hash/libabsl_low_level_hash.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/synchronization/libabsl_graphcycles_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/container/libabsl_raw_hash_set.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/container/libabsl_hashtablez_sampler.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/status/libabsl_status.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/status/libabsl_statusor.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/time/libabsl_time.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/time/libabsl_time_zone.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/time/libabsl_civil_time.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/crc/libabsl_crc_cord_state.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/crc/libabsl_crc_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/crc/libabsl_crc32c.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_cordz_functions.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_cordz_info.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_strings.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_cord_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_str_format_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_cordz_sample_token.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_cordz_handle.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_cord.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/debugging/libabsl_stacktrace.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/debugging/libabsl_symbolize.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/synchronization/libabsl_synchronization.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_spinlock_wait.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/numeric/libabsl_int128.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_base.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_malloc_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/base/libabsl_raw_logging_internal.a
)

list(APPEND tflite_libs
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_ctx.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_context.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_cpuinfo.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_prepacked_cache.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_thread_pool.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_wait.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_allocator.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_system_aligned_alloc.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_context_get_ctx.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_denormal.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_tune.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_blocking_counter.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_apply_multiplier.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_frontend.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_prepare_packed_matrices.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_trmul.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_block_map.a
)

if(BUILD_TFLITE_ARM)
    list(APPEND tflite_libs
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_pack_arm.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_kernel_arm.a)
endif()


if(BUILD_TFLITE_X86)
    list(APPEND tflite_libs
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_kernel_avx.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_pack_avx.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_have_built_path_for_avx.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_kernel_avx512.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_pack_avx512.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_have_built_path_for_avx512.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_kernel_avx2_fma.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_pack_avx2_fma.a
        ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/libruy_have_built_path_for_avx2_fma.a)
endif()

set(tflite_libs_unused
    ${TFLITE_BUILD_DIR}/_deps/fft2d-build/libfft2d_alloc.a
    ${TFLITE_BUILD_DIR}/_deps/fft2d-build/libfft2d_fftsg3d.a
    ${TFLITE_BUILD_DIR}/_deps/fft2d-build/libfft2d_shrtdct.a
    ${TFLITE_BUILD_DIR}/_deps/fft2d-build/libfft2d_fft4f2d.a

    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/profiler/libruy_profiler_instrumentation.a
    ${TFLITE_BUILD_DIR}/_deps/ruy-build/ruy/profiler/libruy_profiler_profiler.a

    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/types/libabsl_bad_variant_access.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/types/libabsl_bad_optional_access.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/types/libabsl_bad_any_cast_impl.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_config.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_commandlineflag_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_program_name.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_private_handle_accessor.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_reflection.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_marshalling.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_parse.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_usage_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_usage.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/flags/libabsl_flags_commandlineflag.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/crc/libabsl_crc_cpu_detect.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_randen_slow.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_randen_hwaes_impl.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_distribution_test_util.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_seed_material.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_randen.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_randen_hwaes.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_seed_sequences.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_platform.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_seed_gen_exception.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_distributions.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/random/libabsl_random_internal_pool_urbg.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/strings/libabsl_strings_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/profiling/libabsl_exponential_biased.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/profiling/libabsl_periodic_sampler.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/debugging/libabsl_leak_check.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/debugging/libabsl_demangle_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/debugging/libabsl_failure_signal_handler.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/debugging/libabsl_examine_stack.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/debugging/libabsl_debugging_internal.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_entry.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_die_if_null.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_format.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_check_op.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_globals.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_sink.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_flags.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_globals.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_conditions.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_message.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_proto.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_nullguard.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_internal_log_sink_set.a
    ${TFLITE_BUILD_DIR}/_deps/abseil-cpp-build/absl/log/libabsl_log_initialize.a
)
