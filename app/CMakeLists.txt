if(ENABLE_EBGRUNTIME)
    add_subdirectory(synap_cli_nb)
endif()

if(CMAKE_CROSSCOMPILING OR ENABLE_SYNAP_DEVICE_SIMULATOR)
    add_subdirectory(synap_cli_info)

    if(ENABLE_TFLITERUNTIME)
        add_subdirectory(synap_cli_tflitec)
    endif()

    add_subdirectory(synap_cli)
    add_subdirectory(synap_cli_ic)
    if(ENABLE_EBGRUNTIME)
        # FIXME: relies on Buffer ctor specific to synap_device
        add_subdirectory(synap_cli_ic2)
    endif()
    add_subdirectory(synap_cli_od)
    add_subdirectory(synap_cli_od2)
    add_subdirectory(synap_cli_ip)
    if(ENABLE_EBGRUNTIME)
        add_subdirectory(synap_init)
    endif()

    # FIXME: why disabling that ?
    if(ENABLE_ADVANCED)
        add_subdirectory(synap_cli_sr)
        add_subdirectory(synap_cli_rnn)
    endif()

endif()
