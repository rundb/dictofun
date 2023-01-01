add_library(task_memory STATIC
    task_memory.cpp
)

target_include_directories(task_memory PUBLIC ./)

target_link_libraries(task_memory PRIVATE
    spi_flash
)

target_link_libraries(task_memory PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)
