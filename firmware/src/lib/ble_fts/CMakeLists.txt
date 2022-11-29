add_library(ble_fts STATIC
    ble_file_transfer_service.c
    ble_file_transfer_service.h
    ble_fts_fsm.cpp
    ble_fts_fsm.h
)

target_include_directories(ble_fts PUBLIC ./)

target_link_libraries(ble_fts PUBLIC project_config)
target_link_libraries(ble_fts PUBLIC nrf5_soc)
target_link_libraries(ble_fts PUBLIC nrf5_ble_common)