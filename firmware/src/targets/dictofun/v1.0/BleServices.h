#pragma once

#include <stdint.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include "ble_file_transfer_service.h"

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
    nrf_ble_qwr_t * getQwrHandle();
    //uint16_t getLbsUUID();
    size_t setAdvUuids(ble_uuid_t * uuids, size_t max_uuids);

    static BleServices& getInstance() {return *_instance;}

    void handleFtsData(ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length);
private:
    static BleServices * _instance;
};
}
