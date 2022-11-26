add_library(ble_fts STATIC
    ble_file_transfer_service.c
    ble_file_transfer_service.h
    ble_fts_fsm.cpp
    ble_fts_fsm.h
)

target_include_directories(ble_fts PUBLIC ./)
