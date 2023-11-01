add_library(myfs STATIC
    myfs.cpp
    block_api_myfs.cpp
)

target_include_directories(myfs PUBLIC ./)

target_link_libraries(myfs PUBLIC
    spi_flash_interface
)
