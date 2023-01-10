add_library(task_led STATIC
    task_led.cpp
)

target_include_directories(task_led PUBLIC ./)

target_link_libraries(task_led PRIVATE
    nrf5_app_timer_v2
)

target_link_libraries(task_led PUBLIC 
    application
)
