add_library(task_wdt STATIC
    task_wdt.cpp
)

target_include_directories(task_wdt PUBLIC ./)

target_link_libraries(task_wdt PRIVATE
    nrf5_log
    nrf5_drv_wdt
    nrf5_nrfx_wdt
)

target_link_libraries(task_wdt PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)
