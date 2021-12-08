#pragma once

#include <stdint.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include <ble_fts/ble_file_transfer_service.h>

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

    class FtsState
    {
    public:
        enum command_id
        {
            CMD_EMPTY = 0,
            CMD_GET_FILE,
            CMD_GET_FILE_INFO,
        };


    private:
        command_id _cmd{CMD_EMPTY};
        uint32_t _file_size{0};
        const uint16_t _ble_its_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;
        bool _is_file_transmission_done{false};
        uint32_t _rec_start_ts{0};

    };

    FtsState _ftsState;
};
}
