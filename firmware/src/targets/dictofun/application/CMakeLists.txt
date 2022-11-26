add_library(application STATIC
    assert.c
    BleServices.cpp
    BleSystem.cpp)

target_include_directories(application PUBLIC ./)
