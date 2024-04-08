#!/bin/bash

source build/header.rc
source build/install.rc
source build/module/toolchain/${CONFIG_TOOLCHAIN_APPLICATION}.rc

#############
# Functions #
#############


compile_and_install() {
  # Config, build and install

  args=""

  if [ "is${CONFIG_ANDROID_OS}" = "isy" ]; then
    args="${args} -DCMAKE_TOOLCHAIN_FILE=${topdir}/external/opencv/Toolchain-syna-android.cmake"
    args="${args} -DSYSROOT=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/build_sysroot"
    args="${args} -DTARGET=${CONFIG_TARGET} -DANDROID=ON"
  else
    args="${args} -DCMAKE_TOOLCHAIN_FILE=${topdir}/external/opencv/Toolchain-syna-linux.cmake"
  fi

  cmake ${opt_synap_tools_src} \
   ${args} \
   -DVSSDK_DIR=${topdir} \
   -DSYNAP_BUILD_DIR=${CONFIG_SYNA_SDK_BUILD_PATH}/SYNAP \
   -DCMAKE_INSTALL_PREFIX=${outdir_synap_tools_intermediate}/install

  make -j ${CONFIG_CPU_NUMBER}
  make install

  INSTALL_D ${outdir_synap_tools_intermediate}/install/bin  ${opt_workdir_build_sysroot}/bin
}

#############
# Submodule #
#############

### Common settings ###
outdir_synap_tools_intermediate=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/synap_tools
mkdir -p ${outdir_synap_tools_intermediate}
opt_workdir_build_sysroot=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/build_sysroot
opt_workdir_runtime_sysroot=${CONFIG_SYNA_SDK_OUT_TARGET_PATH}/system
opt_synap_tools_src=${topdir}/synap/framework/tools

echo "Build and install synap_tools clean:${clean}"

pushd ${outdir_synap_tools_intermediate}

if [ ${clean} -eq 0 ];then
    compile_and_install &
    wait $!
else
    set -x
    rm -rf ${outdir_synap_tools_intermediate}
fi
popd

### Clean up ###
unset -f compile_and_install
