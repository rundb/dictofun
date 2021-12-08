#pragma once

#include <stdint.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>

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
};
}
