add_library(task_led STATIC
    task_led.cpp
)

target_include_directories(task_led PUBLIC ./)

target_link_libraries(task_led PRIVATE
    nrf5_app_timer_v2
    nrf5_nrfx_pwm
    nrf5_drv_pwm
)

target_link_libraries(task_led PUBLIC 
    boards
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_led_interface INTERFACE)

target_include_directories(task_led_interface INTERFACE ./)
