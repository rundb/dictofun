add_library(task_ble STATIC
    task_ble.cpp
)

target_include_directories(task_ble PUBLIC ./)

target_link_libraries(task_ble PRIVATE 
)
target_link_libraries(task_ble PUBLIC 
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_ble_interface INTERFACE)
target_include_directories(task_ble_interface INTERFACE ./)
