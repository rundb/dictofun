add_library(task_ble STATIC
    task_ble.cpp
    ble_system.cpp
    ble_services.cpp
    nrf_error_decoder.cpp
    ble_fts_glue_test.cpp
    ble_fts_glue_target.cpp
)

target_include_directories(task_ble PUBLIC ./)

target_link_libraries(task_ble PRIVATE 
    nrf5_sdh
    nrf5_app_error
    nrf5_log
    nrf5_ble_qwr
    nrf5_ble_gatt
    nrf5_ble_common
    nrf5_ble_peer_manager_handler
    nrf5_ble_peer_manager
    nrf5_fds
    nrf5_ble_id_manager
    nrf5_ble_security_manager
    nrf5_ble_security_dispatcher
    nrf5_ble_gatt_cache_manager
    nrf5_ble_peer_data_storage
    nrf5_ble_peer_database
    nrf5_fstorage
    nrf5_fstorage_sd
    nrf5_atfifo
    nrf5_ble_peer_id
    nrf5_ble_pm_buffer
    nrf5_atflags
    nrf5_ble_advertising
    nrf5_ble_srv_lbs
    boards
    ble_fts
)
target_link_libraries(task_ble PUBLIC 
    nrf5_freertos
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
    nrf5_freertos_portable_memmang_1
    common
)

add_library(task_ble_interface INTERFACE)
target_include_directories(task_ble_interface INTERFACE ./)
