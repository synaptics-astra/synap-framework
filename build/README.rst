SyNAP development
=================

Configuration and options
-------------------------

$ cmake /path/to/framework/build -G Ninja -DVSSDK_DIR=/path/to/vssdk/repo -DNDK_DIR=/path/to/android-ndk-rXXX

NDK_DIR can also be set using an environment variable:

$ NDK_DIR=/path/to/android-ndk-rXXX cmake /path/to/framework/build -G Ninja -DVSSDK_DIR=/path/to/vssdk/repo

.. note::

    When building with Docker inside the ``synap_builder`` container, the NDK_DIR environment variable
    is already pointing to the correct NDK directory, and the generator is set to default to Ninja.
    So the command line is simply:

    $ cmake /path/to/framework/build -DVSSDK_DIR=/path/to/vssdk/repo


By default the build will do nothing, you can use the cmake command to enable or disable the build
of specific components, for example:

    $ cmake /path/to/framework/build -DENABLE_ANDROID_NDK=ON -DENABLE_MODELS=ON -DENABLE_VXK=ON -DENABLE_DOC=OFF

    Enabled components:

      [x] vx kernels (-DENABLE_VXK)
      [x] models (-DENABLE_MODELS)
      [ ] doc (-DENABLE_DOC)
      [ ] Linux Baseline (-DENABLE_LINUX_BASELINE)
      [ ] Linux Host (-DENABLE_LINUX_HOST)
      [x] Android NDK (-DENABLE_ANDROID_NDK)
      [ ] Android 64bit NDK (-DENABLE_ANDROID64_NDK)
      [ ] Linux Yocto (-DENABLE_LINUX_YOCTO)

Compilation and installation
----------------------------

$ ninja

This will compile the components specified in the cmake command line.


Compilation and installation of individual targets
--------------------------------------------------

$ ninja <project>-<platform>

Available projects:
- vxk: platform independent VX kernels compilation for VS640 and VS680
- models: platform independent model compilation for VS640 and VS680
- doc: platform independent RST documentation generation
- framework: SyNAP framework, optional dependencies (tflite, xnnpack, onnxruntime)
- framework-tools: SyNAP framework, optional dependencies (opencv, timvx)
- npu_driver: userspace runtime (synap_device, ebg_utils)
- npu_sw_stack: ovxlib
- synap_ko: SyNAP kernel module (/dev/synap device driver)
- synap_ta: SyNAP firmware (Trusted Application)
- onnxrt: Microsft ONNXruntime
- tflite: Google TensorFlow Lite runtime
- opencv: OpenCV

Available platforms:
- x86_64-linux-gcc: host Linux system with gcc (tested with Ubuntu > 22.04)
- armv7a-android-ndk-api31: Android 32bit target with NDK API 31 (S)
- aarch64-android-ndk-api33: Android 64bit target with NDK API 33 (T)
- android-vssdk: Android VSSDK target, will follow VSSDK build profile (32 or 64bit, API level, VS640 or VS680)
- linux-yocto: Linux Yocto target
- linux-baseline: Linux Baseline target

The target compilations are defined in the function build_target which has a number of options
documented directly in the CMakeLists.txt. You can also modify some of the options in the
CMakeLists.txt, for example:

    build_target(armv7a-android-ndk-api31
        EBG TFLITE FRAMEWORK_TOOLS OPENCV SYNAP_KO SYNAP_TA ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
        ARGS -DCMAKE_TOOLCHAIN_FILE:STRING=${NDK_DIR}/build/cmake/android.toolchain.cmake ${NDK_ARGS}
            -DANDROID_PLATFORM:STRING=android-31
            -DANDROID_ABI:STRING=armeabi-v7a
            -DANDROID_STL:STRING=c++_static
            # Don't set that as this will hide linking issue and will require runtime debugging
            #-DANDROID_ALLOW_UNDEFINED_SYMBOLS:STRING=true
    )

This means that the above target will pass the following option to the various subproject:
EBG TFLITE FRAMEWORK_TOOLS OPENCV SYNAP_KO SYNAP_TA ONNXRT ONNXRT_XNNPACK ONNXRT_NNAPI
