add_executable(dictofun_main main.cpp)

add_subdirectory(tasks)
add_subdirectory(config)
add_subdirectory(application)

nrf5_target(dictofun_main)

target_link_libraries(dictofun_main PRIVATE 
    tasks
    spi_flash
    boards
    nrf5_app_error
    nrf5_drv_clock
    nrf5_nrfx_clock
    nrf5_log
    nrf5_log_default_backends
    # TODO: fix following dependencies
    nrf5_sdh
    nrf5_app_util_platform
    nrf5_memobj
    nrf5_atomic
    nrf5_ringbuf
    nrf5_log_backend_uart
    nrf5_log_backend_serial
    nrf5_section
    nrf5_strerror
    nrf5_balloc
    nrf5_nrfx_uarte
    nrf5_drv_uart
    nrf5_nrfx_uart
    nrf5_ext_fprintf
    nrf5_nrfx_prs
    nrf5_pwr_mgmt
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
)
