add_library(spi_flash STATIC
    spi_flash.cpp
    spi.cpp
    block_device_api.cpp
)

target_include_directories(spi_flash PUBLIC ./)

target_link_libraries(spi_flash PUBLIC common)
target_link_libraries(spi_flash PRIVATE boards
    simple_fs
)

target_link_libraries(spi_flash PUBLIC nrf5_drv_spi)

target_link_libraries(spi_flash PRIVATE 
    nrf5_app_timer
    nrf5_nrfx_spim
    )
