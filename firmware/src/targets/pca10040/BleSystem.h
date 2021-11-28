#pragma once

#ifdef __cplusplus
extern "C"
{
#endif 
void application_init();
void application_cyclic();
#ifdef __cplusplus
}

#include <ble_types.h>
#include <ble.h>
#include <libraries/util/app_util.h>
#include <libraries/timer/app_timer.h>
#include <ble/nrf_ble_gatt/nrf_ble_gatt.h>
#include "BleServices.h"

namespace ble
{

class BleSystem
{
public:
    BleSystem()
    : _bleServices()
    {
        _instance = this;
    }

    void init();
    void cyclic();


    static inline BleSystem& getInstance() {return *_instance; }
    inline BleServices& getServices() { return _bleServices; }

    void bleEventHandler(ble_evt_t const * p_ble_evt, void * p_context);

    uint16_t& getConnectionHandle() const { return getInstance().m_conn_handle; }

    static const uint8_t APP_BLE_OBSERVER_PRIO = 3;
    static const uint8_t APP_BLE_CONN_CFG_TAG = 1;
private:
    static BleSystem * _instance;
    BleServices _bleServices;
    void initBleStack();
    void startAdvertising();
    void initGapParams();
    void initGatt();
    void initConnParameters();

    uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
    //uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
    //NRF_BLE_GATT_DEF(m_gatt);
    nrf_ble_qwr_t * _qwr_default_handle{nullptr};

    static const uint32_t MIN_CONN_INTERVAL = MSEC_TO_UNITS(100, UNIT_1_25_MS);
    static const uint32_t MAX_CONN_INTERVAL = MSEC_TO_UNITS(200, UNIT_1_25_MS);
    static const uint32_t SLAVE_LATENCY = 0; 
    static const uint32_t CONN_SUP_TIMEOUT = MSEC_TO_UNITS(4000, UNIT_10_MS);

    static const char DEVICE_NAME[];

    static const uint32_t FIRST_CONN_PARAMS_UPDATE_DELAY  = APP_TIMER_TICKS(20000);
    static const uint32_t NEXT_CONN_PARAMS_UPDATE_DELAY = APP_TIMER_TICKS(5000);
    static const uint32_t MAX_CONN_PARAMS_UPDATE_COUNT = 3;
};

}

#endif 
