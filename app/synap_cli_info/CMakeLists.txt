add_executable(synap_cli_info main.cpp)

if(NOT CMAKE_CROSSCOMPILING)
  set(PYBIND11_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../pybind11)
  if(EXISTS ${PYBIND11_DIR}/CMakeLists.txt)
    add_subdirectory(${PYBIND11_DIR} pybind11)
    target_link_libraries(synap_cli_info PRIVATE pybind11::embed)
    target_compile_definitions(synap_cli_info PRIVATE SYNAP_PYBIND_ENABLE=1)
  endif()
endif()

target_link_libraries(synap_cli_info PRIVATE
  synap_app synap_utils
  synapnb
  nlohmann
)

if(ENABLE_EBGRUNTIME)
    target_link_libraries(synap_cli_info PRIVATE ebg_utils)
endif()

install(TARGETS synap_cli_info)
