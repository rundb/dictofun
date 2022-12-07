add_library(boards OBJECT
    boards.c
)

target_include_directories(boards PUBLIC ./)

target_link_libraries(boards INTERFACE nrf5_nrfx_hal)
target_link_libraries(boards PUBLIC nrf5_nrfx_hal)
target_link_libraries(boards PUBLIC nrf5_mdk)
target_link_libraries(boards PUBLIC nrf5_assert)
