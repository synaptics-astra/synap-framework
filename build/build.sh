#!/bin/bash

source build/header.rc

[ $clean -eq 1 ] && exit 0
android_vssdk="OFF"
android64_ndk="OFF"
linux_baseline="OFF"
oe32="OFF"
oe64="OFF"
cmake_args=""
oe_dir=${CONFIG_SYNA_SDK_PATH}/toolchain/oe/linux-x64/gcc-11.3.0-poky

# build_sysroot for different modules dpendency
build_sysroot=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_SYNA_SDK_OUT_SYSYROOT}

# rootfs for sdk lib and finally in image
rootfs=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/${CONFIG_SYNA_SDK_OUT_ROOTFS}
rootfs_install=${rootfs}/usr

if [ "is${CONFIG_RUNTIME_ANDROID}" = "isy" ]; then
    if [[ ${CONFIG_TOOLCHAIN_APPLICATION_PATH} == *"aarch64"* ]];then
        aarch="aarch64"
        android64_ndk="ON"
    else
        aarch="armv7a"
        android_vssdk="ON"
    fi

    bin_install_dir=${rootfs}/home/galois/bin
    models_install_dir=${rootfs}/home/galois/firmware/models
    vxk_install_dir=${rootfs}/home/galois/firmware/vxk

    target_name=${aarch}-android
    cmake_args+=" -DNDK_DIR=${CONFIG_SYNA_SDK_PATH}/toolchain/clang/linux-x86/ndk"

elif [ "is${CONFIG_RUNTIME_LINUX_BASELINE_BUILDROOT}" = "isy" ]; then
    linux_baseline="ON"
    bin_install_dir=${rootfs}/opt/syna/bin
    models_install_dir=${rootfs}/opt/syna/fw/models
    vxk_install_dir=${rootfs}/opt/syna/fw/vxk

    target_name="linux-gcc-arm"

elif [ "is${CONFIG_RUNTIME_OE}" = "isy" ]; then
    oe32="ON"
    bin_install_dir=${rootfs}/opt/syna/bin
    models_install_dir=${rootfs}/opt/syna/fw/models
    vxk_install_dir=${rootfs}/opt/syna/fw/vxk

    target_name="armv7a-oe"
    environment_file=environment-setup-armv7at2hf-neon-vfpv4-pokymllib32-linux-gnueabi
    source ${oe_dir}/${environment_file}

elif [ "is${CONFIG_RUNTIME_OE64}" = "isy" ]; then
    oe64="ON"
    bin_install_dir=${rootfs}/opt/syna/bin
    models_install_dir=${rootfs}/opt/syna/fw/models
    vxk_install_dir=${rootfs}/opt/syna/fw/vxk

    target_name="aarch64-oe"
    environment_file=environment-setup-armv8a-poky-linux
    source ${oe_dir}/${environment_file}
else
    echo "error! unsupported target!"
    exit 1
fi

if [ "is${CONFIG_BERLIN_VS640_A0}" = "isy" ]; then
    chip_c="platypus"
    chip_alias="vs640"
elif [ "is${CONFIG_BERLIN_VS680_A0}" = "isy" ]; then
    chip_c="dolphin"
    chip_alias="vs680"
else
    echo "error! unsupported chip!"
    exit 1
fi


# cp synap/release all pre-built files to build install if exist
synap_rel_dir=${CONFIG_SYNA_SDK_PATH}/synap/release
synap_rel_inc=${synap_rel_dir}/include
synap_rel_lib=${synap_rel_dir}/lib/${target_name}
synap_rel_bin=${synap_rel_dir}/bin/${target_name}

BUILD_DIR=${CONFIG_SYNA_SDK_BUILD_PATH}/SYNAP
mkdir -p ${BUILD_DIR}

synap_build_install_dir=${CONFIG_SYNA_SDK_BUILD_PATH}/SYNAP/install
synap_build_install_lib_dir=${synap_build_install_dir}/lib/${target_name}
synap_build_install_bin_dir=${synap_build_install_dir}/bin/${target_name}
mkdir -p ${synap_build_install_dir}
mkdir -p ${synap_build_install_lib_dir}
mkdir -p ${synap_build_install_bin_dir}

if [[ -d ${synap_rel_inc} ]];then
  cp -rf ${synap_rel_inc} ${synap_build_install_dir}
fi

if [[ -d ${synap_rel_bin} ]]; then
  cp ${synap_rel_bin}/* ${synap_build_install_bin_dir}
fi

if [[ -d ${synap_rel_lib} ]];then
  rsync -az ${synap_rel_lib}/* ${synap_build_install_lib_dir}
fi


# begin to compile framework

cmake_args+=" -DVSSDK_DIR=${CONFIG_SYNA_SDK_PATH}
            -DCMAKE_BUILD_TYPE=Release
            -DENABLE_SYNAP_DEVICE_SIMULATOR=OFF
            -DENABLE_ANDROID_VSSDK=${android_vssdk}
            -DENABLE_ANDROID64_NDK_VSSDK=${android64_ndk}
            -DENABLE_LINUX_BASELINE=${linux_baseline}
            -DENABLE_LINUX_YOCTO_ARMv7a_VSSDK=${oe32}
            -DENABLE_LINUX_YOCTO_AARCH64_VSSDK=${oe64}
            "

pushd ${BUILD_DIR}
cmake -G Ninja ${CONFIG_SYNA_SDK_PATH}/synap/framework/build $cmake_args
ninja -j 16
popd


# cp compiled lib/inc/bin to build_sysroot for modules build dependency and gen IMAGE
build_sysroot_usr_lib=${build_sysroot}/usr/lib
build_sysroot_inc=${build_sysroot}/include
rootfs_lib=${rootfs_install}/lib

mkdir -p ${build_sysroot_usr_lib}
mkdir -p ${build_sysroot_inc}
mkdir -p ${rootfs_lib}
mkdir -p ${bin_install_dir}

if [[ -d ${synap_build_install_dir}/include ]];then
  cp -rf ${synap_build_install_dir}/include/* ${build_sysroot_inc}
fi

if [[ -d ${synap_build_install_lib_dir} ]];then
  rsync -az ${synap_build_install_lib_dir}/* ${build_sysroot_usr_lib}
  rsync -az ${synap_build_install_lib_dir}/* ${rootfs_lib}
fi

if [[ -d ${synap_build_install_bin_dir} ]]; then
  cp ${synap_build_install_bin_dir}/* ${bin_install_dir}
fi

#install models
if [ -d "${synap_rel_dir}/models/${chip_c}" ]; then
    mkdir -p ${models_install_dir}
    cp -adprf ${synap_rel_dir}/models/${chip_c}/* ${models_install_dir}/.
    # copy secure models
    cp -adprf ${synap_rel_dir}/models/${chip_alias}/* ${models_install_dir}/.
fi

#install vxk
if [ -d "${synap_rel_dir}/vxk/${chip_c}" ]; then
    mkdir -p ${vxk_install_dir}
    cp -adprf ${synap_rel_dir}/vxk/${chip_c}/* ${vxk_install_dir}/.
fi
