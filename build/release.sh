#!/bin/bash

source build/header.rc
source build/chipset.rc
source build/utils.rc

source build/chipparam.rc ${syna_chip_name_set[0]}

oe_dir=${CONFIG_SYNA_SDK_PATH}/toolchain/oe/linux-x64/gcc-11.3.0-poky
cmake_args=""

BUILD_DIR=${CONFIG_SYNA_SDK_BUILD_PATH}/SYNAP
if [[ -d ${BUILD_DIR} ]];then
    # keep release build clean anytime
    rm -rf ${BUILD_DIR}
fi
mkdir -p ${BUILD_DIR}


# $1:android_armv7a, $2:android_aarch64, $3:linux_x86_host, $4:linux_baseline, $5:oe_armv7a, $6:oe_aarch64
function execute_build()
{
    cmake_args+=" -DVSSDK_DIR=${CONFIG_SYNA_SDK_PATH}
                -DCMAKE_BUILD_TYPE=Release
                -DENABLE_SYNAP_DEVICE_SIMULATOR=OFF
                -DENABLE_ANDROID_VSSDK=${1}
                -DENABLE_ANDROID64_NDK_VSSDK=${2}
                -DENABLE_LINUX_HOST_VSSDK=${3}
                -DENABLE_LINUX_BASELINE=${4}
                -DENABLE_LINUX_YOCTO_ARMv7a_VSSDK=${5}
                -DENABLE_LINUX_YOCTO_AARCH64_VSSDK=${6}
                "

    pushd ${BUILD_DIR}

    cmake -G Ninja ${CONFIG_SYNA_SDK_PATH}/synap/framework/build $cmake_args
    ninja -j 16

    popd
}

for i in ${!syna_chip_rev_set[*]}
do
    toolchain_array=()
    count=$(get_target_count)
    for ((index = 0; index != count; index++));
    do
        target_list=$(get_target_list_by_index $index)
        IFS=',' read cur_runtime cur_sysroot cur_toolchain cur_toolchain_path <<< "$target_list"

        # only build for different toolchain
        if [[ " ${toolchain_array[@]} " =~ " ${cur_toolchain} " ]]; then
            continue
        fi

        toolchain_array+=($cur_toolchain)
        cur_sysroot_out=$(get_sysroot_out_path $cur_sysroot)

        if [[ ${cur_sysroot_out} == *"rdk"* ]];then
            echo "sw_stack not compile rdk"
            continue
        fi

        echo "Build NPU ${cur_runtime} with ${cur_toolchain} sysroot:${cur_sysroot_out}"

        if [[ ${cur_toolchain} == *"arm-linux-gnueabihf"* ]]; then
            linux_baseline="ON"
        elif [[ ${cur_toolchain} == *"arm-linux-androideabi"* ]]; then
            android_vssdk="ON"
            linux_x86_host="ON"
        elif [[ ${cur_toolchain} == *"aarch64-linux-android"* ]]; then
            android64_ndk="ON"
        elif [[ ${cur_toolchain} == *"arm-pokymllib32-linux-gnueabi"* ]]; then
            oe32="ON"
        elif [[ ${cur_toolchain} == *"aarch64-pokymllib32-linux"* ]]; then
            oe64="ON"
        else
            echo "current toolchain ${cur_toolchain} DOES NOT build"
            continue
        fi
    done
done

echo "CONFIG_SYNA_SDK_PATH : ${CONFIG_SYNA_SDK_PATH}"
echo "CONFIG_SYNA_SDK_BUILD_PATH : ${CONFIG_SYNA_SDK_BUILD_PATH}"

# cp synap/release all pre-installed files to build/install if exists
synap_rel_dir=${CONFIG_SYNA_SDK_PATH}/synap/release
synap_rel_inc=${synap_rel_dir}/include
synap_rel_lib=${synap_rel_dir}/lib
synap_rel_bin=${synap_rel_dir}/bin

synap_build_install_dir=${CONFIG_SYNA_SDK_BUILD_PATH}/SYNAP/install
mkdir -p ${synap_build_install_dir}

if [[ -d ${synap_rel_inc} ]];then
  cp -rf ${synap_rel_inc} ${synap_build_install_dir}
fi

if [[ -d ${synap_rel_bin} ]];then
  cp -rf ${synap_rel_bin} ${synap_build_install_dir}
fi

if [[ -d ${synap_rel_lib} ]];then
  rsync -az ${synap_rel_lib} ${synap_build_install_dir}
fi

if [[ ${linux_x86_host} == *"ON"* ]];then
    cmake_args+=" -DENABLE_VXK=ON -DSOC=VS640A0;VS680A0 "
fi

if [[ ${android_vssdk} == *"ON"* ]];then
    cmake_args+=" -DNDK_DIR=${CONFIG_SYNA_SDK_PATH}/toolchain/clang/linux-x86/ndk "
fi

execute_build ${android_vssdk} ${android64_ndk} ${linux_x86_host} ${linux_baseline} "OFF" "OFF"

unset cmake_args
cmake_args+=" -DENABLE_VXK=OFF "

# yocto 32/64bit source env file confuse system to find toolchain,
# so build them separately in case impact others
if [[ ${oe32} == *"ON"* ]];then
    oe32_environment_file=environment-setup-armv7at2hf-neon-vfpv4-pokymllib32-linux-gnueabi
    source ${oe_dir}/${oe32_environment_file}
    execute_build "OFF" "OFF" "OFF" "OFF" ${oe32} "OFF"
fi

if [[ ${oe64} == *"ON"* ]];then
    oe64_environment_file=environment-setup-armv8a-poky-linux
    source ${oe_dir}/${oe64_environment_file}
    execute_build "OFF" "OFF" "OFF" "OFF" "OFF" ${oe64}
fi

# copy compiled lib/bin/header and related source code
rel_dir=${CONFIG_SYNA_SDK_PATH}/synap/release
syna_rel_dir=${CONFIG_SYNA_SDK_REL_PATH}/synap

# copy install/* to vssdk/synap/release
# in order to put all pre-installed and compiled files together in release
if [[ -d ${BUILD_DIR}/install ]];then
    cp -adprf ${BUILD_DIR}/install/* ${rel_dir}
fi

# copy synap/release to syna-release/synap
mkdir -p ${syna_rel_dir}/release
rsync -az --exclude={.git,.gitignore,.gitattributes} ${rel_dir}/ ${syna_rel_dir}/release/

# copy framework some files to syna-release/synap
framework_dir=${CONFIG_SYNA_SDK_PATH}/synap/framework
syna_rel_framework_dir=${syna_rel_dir}/framework
mkdir -p ${syna_rel_framework_dir}
rsync -az --exclude={.git,.gitignore,.gitattributes,doc,dockerfiles,private,scripts,toolkit,tools,CHANGELOG} \
    ${framework_dir}/ ${syna_rel_framework_dir}/

# cope kernel related
driver_dir=${CONFIG_SYNA_SDK_PATH}/synap/vsi_npu_driver
syna_rel_driver_dir=${CONFIG_SYNA_SDK_REL_PATH}/synap/vsi_npu_driver
mkdir -p ${syna_rel_driver_dir}
rsync -az --exclude={.git,.gitignore,.gitattributes,private,scripts,kernel/tests} ${driver_dir}/ ${syna_rel_driver_dir}/
