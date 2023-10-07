add_library(adc_if INTERFACE)
target_include_directories(adc_if INTERFACE ./interface)

add_library(adc STATIC
    adc.cpp
)

target_include_directories(adc PUBLIC ./)

target_link_libraries(adc PUBLIC 
    adc_if
    common
)

target_link_libraries(adc PRIVATE
    nrf5_nrfx_saadc
)

