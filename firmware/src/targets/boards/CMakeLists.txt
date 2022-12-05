add_library(boards STATIC
    boards.c
    boards.h
)

target_include_directories(boards PUBLIC ./)

target_link_libraries(boards INTERFACE nrf5_nrfx_hal)
target_link_libraries(boards PUBLIC nrf5_nrfx_hal)
target_link_libraries(boards PUBLIC nrf5_mdk)
target_link_libraries(boards PUBLIC nrf5_assert)
