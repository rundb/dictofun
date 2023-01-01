add_library(tasks STATIC
    battery_measurement.cpp
)

target_include_directories(tasks PUBLIC ./)

target_include_directories(tasks PRIVATE cli)

target_link_libraries(tasks PUBLIC ble_fts)
target_link_libraries(tasks PUBLIC drv_pdm)
target_link_libraries(tasks PUBLIC nrf5_nrfx_saadc)
target_link_libraries(tasks PRIVATE 
    spi_flash
)

target_link_libraries(tasks PUBLIC 
    application
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
    nrf5_cli
    nrf5_cli_uart
    nrf5_queue
)

add_subdirectory(audio)
add_subdirectory(cli)
add_subdirectory(memory)
add_subdirectory(systemstate)
add_subdirectory(led)