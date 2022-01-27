#pragma once

/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include "ble_file_transfer_service.h"
#include "lfs.h"

namespace ble
{
enum BleCommands
{
    CMD_EMPTY,
    CMD_GET_FILE,
    CMD_GET_FILE_INFO,
};

/**
 * This class contains instances of BLE services used by app
 */
class BleServices
{
public:
    BleServices();
    void init();
    void start(lfs_t * fs, lfs_file_t * file);
    void cyclic();

    nrf_ble_qwr_t * getQwrHandle();
    //uint16_t getLbsUUID();
    size_t setAdvUuids(ble_uuid_t * uuids, size_t max_uuids);

    static BleServices& getInstance() {return *_instance;}

    void handleFtsData(ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length);

    void setFileSizeForTransfer(const size_t size) { _file_size = size; }

    bool isFileTransmissionComplete() { return _is_file_transmission_done; }

private:
    static BleServices * _instance;
    lfs_t * _fs{nullptr};
    lfs_file_t * _file{nullptr};
    BleCommands _ble_cmd;
    uint32_t _read_pointer{0};
    static const uint32_t SPI_READ_SIZE = 128;
    size_t _file_size{0UL};
    bool _is_file_transmission_started{false};
    bool _is_file_transmission_done{false};

    uint32_t send_data(const uint8_t *data, uint32_t data_size);
    static const uint16_t BLE_ITS_MAX_DATA_LEN = BLE_GATT_ATT_MTU_DEFAULT - 3;
};
}
