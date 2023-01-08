add_library(task_audio_tester STATIC
    task_audio_tester.cpp
)

target_include_directories(task_audio_tester PUBLIC ./)

target_link_libraries(task_audio_tester PRIVATE)

target_link_libraries(task_audio_tester PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_audio_tester_interface INTERFACE)
target_include_directories(task_audio_tester_interface INTERFACE ./)
