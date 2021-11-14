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

namespace ble
{

class BleSystem
{
public:
    BleSystem()
    {
        _instance = this;
    }

    void init();
    void cyclic();


    static inline BleSystem& getInstance() {return *_instance; }

    void bleEventHandler(ble_evt_t const * p_ble_evt, void * p_context);
private:
    static BleSystem * _instance;
    void bleStackInit();
    void startAdvertising();
    void gapParamsInit();

    uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
    uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

    static const uint8_t APP_BLE_OBSERVER_PRIO = 3;
    static const uint8_t APP_BLE_CONN_CFG_TAG = 1;

    static const uint32_t MIN_CONN_INTERVAL = MSEC_TO_UNITS(100, UNIT_1_25_MS);        /**< Minimum acceptable connection interval (0.5 seconds). */
    static const uint32_t MAX_CONN_INTERVAL = MSEC_TO_UNITS(200, UNIT_1_25_MS);        /**< Maximum acceptable connection interval (1 second). */
    static const uint32_t SLAVE_LATENCY = 0;                                       /**< Slave latency. */
    static const uint32_t CONN_SUP_TIMEOUT = MSEC_TO_UNITS(4000, UNIT_10_MS);         /**< Connection supervisory time-out (4 seconds). */

    static const char DEVICE_NAME[];                         /**< Name of device. Will be included in the advertising data. */
};

}

#endif 
