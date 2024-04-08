cmake_minimum_required(VERSION 3.18.1)

project("synap_jni")

# use different paths in install and source
if(EXISTS ${CMAKE_SOURCE_DIR}/../../../../synap_device/CMakeLists.txt)
    set(SYNAP_DEVICE_DIR ${CMAKE_SOURCE_DIR}/../../../../synap_device/)
else()
    set(SYNAP_DEVICE_DIR ${CMAKE_SOURCE_DIR}/../../../../../external/vsi_npu_driver/synap/user/)
endif()

add_subdirectory(${SYNAP_DEVICE_DIR} synap_device)

add_library(synap_jni SHARED synap_jni.c)

find_library(log-lib log )

target_link_libraries( synap_jni PUBLIC ${log-lib} PRIVATE synap_device )