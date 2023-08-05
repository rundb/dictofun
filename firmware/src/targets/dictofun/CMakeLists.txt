add_executable(dictofun_main 
    main.cpp
    freertos_memory_allocation.c
    app_error_handler.cpp
)

add_subdirectory(tasks)
add_subdirectory(config)
add_subdirectory(application)
add_subdirectory(version)

nrf5_target(dictofun_main)

target_link_libraries(dictofun_main PRIVATE 
    task_audio
    task_audio_tester
    task_cli_logger
    task_memory
    task_system_state
    task_led
    task_ble
    task_rtc
    task_button
    spi_flash
    littlefs
    boards
    nrf5_app_error
    nrf5_drv_clock
    nrf5_nrfx_clock
    nrf5_app_timer_v2
    nrf5_log
    nrf5_log_default_backends
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
    dummy_module
    application_audio
    microphone_pdm
    # TODO: fix following dependencies (spread them across corresponding modules)
    nrf5_sdh
    nrf5_app_util_platform
    nrf5_memobj
    nrf5_atomic
    nrf5_ringbuf
    nrf5_log_backend_uart
    nrf5_log_backend_serial
    nrf5_section
    nrf5_strerror
    nrf5_balloc
    nrf5_nrfx_uarte
    nrf5_drv_uart
    nrf5_nrfx_uart
    nrf5_ext_fprintf
    nrf5_nrfx_prs
    nrf5_pwr_mgmt
    nrf5_sortlist
    nrf5_hardfault
    nrf5_hardfault_handler
)

add_custom_command(TARGET dictofun_main
    PRE_BUILD
    COMMAND python3 ARGS ../../../../../firmware/src/targets/dictofun/version/version.py
)

add_custom_command(TARGET dictofun_main
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex dictofun_main ${PROJECT_NAME}.hex)
