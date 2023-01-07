add_library(task_system_state STATIC
    task_state.cpp
)

target_include_directories(task_system_state PUBLIC ./)

target_link_libraries(task_system_state PRIVATE
    spi_flash
)

target_link_libraries(task_system_state PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
    task_cli_logger_interface
    task_audio_interface
    task_audio_tester_interface
)
