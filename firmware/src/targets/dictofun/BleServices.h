// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include <stdint.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include "ble_file_transfer_service.h"
#include "ble_fts_fsm.h"
#include "simple_fs.h"

namespace ble
{

/**
 * This class contains instances of BLE services used by app
 */
class BleServices
{
public:
    BleServices();
    void init();
    void start();
    void cyclic();

    nrf_ble_qwr_t * getQwrHandle();

    size_t setAdvUuids(ble_uuid_t * uuids, size_t max_uuids);

    static BleServices& getInstance() {return *_instance;}

    void handleFtsData(ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length);

    void setFileSizeForTransfer(const size_t size) { _file_size = size; }

    bool isFileTransmissionComplete() { return _fsm.is_transmission_complete(); }
    
    static void disconnect(uint16_t conn_handle, void* p_context);
private:
    static BleServices * _instance;
    filesystem::File _file;
    filesystem::FilesCount _files_count{0,0};
    BleCommands _ble_cmd;
    uint32_t _read_pointer{0};

    size_t _file_size{0UL};
    bool _is_file_transmission_started{false};
    bool _is_file_transmission_done{false};

    ble::fts::FtsStateMachine _fsm;


    uint32_t send_data(const uint8_t *data, uint32_t data_size);
    
};
}
