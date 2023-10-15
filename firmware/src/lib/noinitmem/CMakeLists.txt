add_library(noninitmem STATIC
    noinit_mem.cpp
)

target_include_directories(noninitmem PUBLIC ./)

target_link_libraries(noninitmem PRIVATE
    nrf5_crc32
    nrf5_log
)
