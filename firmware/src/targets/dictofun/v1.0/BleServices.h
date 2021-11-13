#pragma once

#include <stdint.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include "ble_file_transfer_service.h"

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
    BleCommands _ble_cmd;
    uint32_t _read_pointer{0};
    static const uint32_t SPI_READ_SIZE = 128;
    size_t _file_size{0UL};
    bool _is_file_transmission_done{false};

    uint32_t send_data(const uint8_t *data, uint32_t data_size);
    static const uint16_t BLE_ITS_MAX_DATA_LEN = BLE_GATT_ATT_MTU_DEFAULT - 3;
};
}
