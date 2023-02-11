add_library(task_memory STATIC
    task_memory.cpp
    littlefs_access.cpp
    memtest.cpp
)

target_include_directories(task_memory PUBLIC ./)

target_link_libraries(task_memory PRIVATE
    spi_flash
    task_ble_interface
    ble_fts
)

target_link_libraries(task_memory PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
    littlefs
    littlefs_integration
)

add_library(task_memory_interface INTERFACE)
target_include_directories(task_memory_interface INTERFACE ./)
