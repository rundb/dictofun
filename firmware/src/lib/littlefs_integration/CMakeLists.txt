add_library(littlefs_integration STATIC
    block_api.cpp
)

target_include_directories(littlefs_integration PUBLIC ./)

target_link_libraries(littlefs_integration PUBLIC
    spi_flash_interface
    littlefs
    nrf5_log
)
