add_library(spi_flash STATIC EXCLUDE_FROM_ALL
    spi_flash.cpp
    spi.cpp
)

target_include_directories(spi_flash PUBLIC ./)

target_link_libraries(spi_flash PUBLIC 
    common
    spi_flash_interface
    nrf5_drv_spi
)

target_link_libraries(spi_flash PRIVATE 
    boards
    nrf5_app_timer_v2
    nrf5_nrfx_spim
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)

add_subdirectory(interface)
