# add_library(spi_flash STATIC EXCLUDE_FROM_ALL
#     spi_flash.cpp
#     spi.cpp
# )

# target_include_directories(spi_flash PUBLIC ./)

# target_link_libraries(spi_flash PUBLIC 
#     common
#     spi_flash_interface
#     nrf5_drv_spi
# )

# target_link_libraries(spi_flash PRIVATE 
#     boards
#     nrf5_app_timer_v2
#     nrf5_nrfx_spim
# )

# add_subdirectory(interface)


add_library(i2c_if INTERFACE)

target_include_directories(i2c_if INTERFACE ./interface)

add_library(nrf_i2c STATIC
    nrf_i2c.cpp
)

target_include_directories(nrf_i2c PUBLIC ./)
target_link_libraries(nrf_i2c PUBLIC 
    i2c_if
)