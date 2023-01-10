add_library(task_ble STATIC
    task_ble.cpp
    ble_system.cpp
)

target_include_directories(task_ble PUBLIC ./)

target_link_libraries(task_ble PRIVATE 
    nrf5_sdh
    nrf5_app_error
    nrf5_log
    nrf5_ble_qwr
)
target_link_libraries(task_ble PUBLIC 
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
    common
)

add_library(task_ble_interface INTERFACE)
target_include_directories(task_ble_interface INTERFACE ./)
