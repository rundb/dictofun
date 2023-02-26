add_library(ble_fts OBJECT
    ble_fts.cpp
)

target_include_directories(ble_fts PUBLIC ./)

target_link_libraries(ble_fts PUBLIC
    common
    nrf5_ble_link_ctx_manager
)

target_link_libraries(ble_fts PRIVATE
    nrf5_ble_common
    nrf5_crc32
)
