add_library(application STATIC
    assert.c
    BleServices.cpp
    BleSystem.cpp)

target_include_directories(application PUBLIC ./)

target_link_libraries(application PRIVATE ble_fts)
