add_library(battery_if INTERFACE)
target_include_directories(battery_if INTERFACE ./)

target_link_libraries(battery_if INTERFACE 
    adc_if
)

add_library(battery STATIC
    battery.cpp
)

target_include_directories(battery PUBLIC ./)

target_link_libraries(battery PUBLIC 
    adc_if
    common
)
