add_library(myfs STATIC
    myfs.cpp
    block_api_myfs.cpp
)

target_include_directories(myfs PUBLIC ./
)

target_link_libraries(myfs PUBLIC
    spi_flash_interface
)

target_link_libraries(myfs PRIVATE
    nrf5_log
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
)
