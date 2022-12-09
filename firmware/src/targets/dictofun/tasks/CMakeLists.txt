add_library(tasks STATIC
    battery_measurement.cpp  
    task_audio.cpp  
    task_led.cpp  
    task_memory.cpp  
    task_state.cpp  
    task_cli_logger.cpp
)

target_include_directories(tasks PUBLIC ./)

target_link_libraries(tasks PUBLIC ble_fts)
target_link_libraries(tasks PUBLIC drv_pdm)
target_link_libraries(tasks PUBLIC nrf5_nrfx_saadc)
target_link_libraries(tasks PRIVATE spi_flash)

target_link_libraries(tasks PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)
