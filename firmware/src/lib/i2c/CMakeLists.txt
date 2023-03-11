add_library(i2c_if INTERFACE)

target_include_directories(i2c_if INTERFACE ./interface)

add_library(nrf_i2c STATIC
    nrf_i2c.cpp
)

target_include_directories(nrf_i2c PUBLIC ./)
target_link_libraries(nrf_i2c PUBLIC 
    i2c_if
)
