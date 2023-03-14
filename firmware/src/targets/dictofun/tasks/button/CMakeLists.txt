add_library(task_button STATIC
    task_button.cpp
)

target_include_directories(task_button PUBLIC ./)

target_link_libraries(task_button PRIVATE 
)
target_link_libraries(task_button PUBLIC 
    application
    boards
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_button_interface INTERFACE)
target_include_directories(task_button_interface INTERFACE ./)
