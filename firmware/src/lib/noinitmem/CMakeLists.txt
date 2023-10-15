add_library(noinitmem STATIC
    noinit_mem.cpp
)

target_include_directories(noinitmem PUBLIC ./)

target_link_libraries(noinitmem PRIVATE
    nrf5_crc32
    nrf5_log
)
