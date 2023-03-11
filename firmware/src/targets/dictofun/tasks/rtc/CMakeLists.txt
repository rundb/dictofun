add_library(task_rtc STATIC
    task_rtc.cpp
)

target_include_directories(task_rtc PUBLIC ./)

target_link_libraries(task_rtc PRIVATE
    nrf5_log
)

target_link_libraries(task_rtc PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_rtc_interface INTERFACE)
target_include_directories(task_rtc_interface INTERFACE ./)
