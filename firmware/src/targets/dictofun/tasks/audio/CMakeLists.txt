add_library(task_audio STATIC
    task_audio.cpp
)

target_include_directories(task_audio PUBLIC ./)

target_link_libraries(task_audio PRIVATE 
    application_audio
    microphone_pdm
)
target_link_libraries(task_audio PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_library(task_audio_interface INTERFACE)
target_include_directories(task_audio_interface INTERFACE ./)
