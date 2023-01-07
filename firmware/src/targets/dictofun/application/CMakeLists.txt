add_library(application STATIC
    assert.c
    BleServices.cpp
    BleSystem.cpp)

target_include_directories(application PUBLIC ./)

target_link_libraries(application PUBLIC tasks)

target_link_libraries(application PRIVATE 
    ble_fts
    nrf5_ble_advertising
)
target_link_libraries(application PUBLIC 
    nrf5_ble_qwr
    boards
    nrf5_ble_srv_lbs
    nrf5_ble_srv_bas
    nrf5_dfu_ble
    nrf5_ble_srv_dfu
    nrf5_ble_gatt
)

add_subdirectory(audio)
