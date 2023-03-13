add_library(rv4162 STATIC
    rv4162.cpp
)

target_include_directories(rv4162 PUBLIC ./)

target_link_libraries(rv4162 PUBLIC 
    i2c_if
    common
    nrf5_log
)

target_link_libraries(rv4162 PRIVATE
)
