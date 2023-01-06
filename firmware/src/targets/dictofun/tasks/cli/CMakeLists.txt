add_library(task_cli_logger STATIC
    task_cli_logger.cpp
    cli_commands.c
)

target_include_directories(task_cli_logger PUBLIC ./)

target_link_libraries(task_cli_logger PRIVATE 
    nrf5_cli
    nrf5_cli_uart
    nrf5_queue
)
target_link_libraries(task_cli_logger PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_cli_logger_interface INTERFACE)
target_include_directories(task_cli_logger_interface INTERFACE ./)
