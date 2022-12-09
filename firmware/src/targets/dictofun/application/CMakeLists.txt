add_library(application STATIC
    assert.c
    BleServices.cpp
    BleSystem.cpp)

target_include_directories(application PUBLIC ./)

target_link_libraries(application PUBLIC tasks)

target_link_libraries(application PRIVATE ble_fts)
target_link_libraries(application PUBLIC nrf5_ble_qwr)
target_link_libraries(application PUBLIC nrf5_ble_srv_lbs)
target_link_libraries(application PUBLIC nrf5_ble_srv_bas)
target_link_libraries(application PUBLIC nrf5_dfu_ble)
target_link_libraries(application PRIVATE nrf5_ble_advertising)
target_link_libraries(application PUBLIC nrf5_ble_srv_dfu)
target_link_libraries(application PUBLIC nrf5_ble_gatt)

add_subdirectory(audio)
