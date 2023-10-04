add_library(task_battery STATIC
    task_battery.cpp
)

target_include_directories(task_battery PUBLIC ./)

target_link_libraries(task_battery PRIVATE
    adc
    battery
    nrf5_log
)

target_link_libraries(task_battery PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_battery_interface INTERFACE)
target_include_directories(task_battery_interface INTERFACE ./)
