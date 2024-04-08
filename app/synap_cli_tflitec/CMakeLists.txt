set(app_name "synap_cli_tflitec")

add_executable(${app_name} main.c)

target_link_libraries(${app_name} PRIVATE tensorflow-lite)

install(TARGETS ${app_name})
