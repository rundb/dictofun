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
#include <ble/peer_manager/peer_manager.h>
#include "BleServices.h"
#include "lfs.h"

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
    void start(lfs_t * fs, lfs_file_t * file);
    void cyclic();

    static inline BleSystem& getInstance() {return *_instance; }
    bool isActive() { return _isActive;}
    inline BleServices& getServices() { return _bleServices; }

    void bleEventHandler(ble_evt_t const * p_ble_evt, void * p_context);

    uint16_t& getConnectionHandle() const { return getInstance().m_conn_handle; }

    static const uint8_t APP_BLE_OBSERVER_PRIO = 3;
    static const uint8_t APP_BLE_CONN_CFG_TAG = 1;
private:
    static BleSystem * _instance;
    lfs_t * _fs{nullptr};
    lfs_file_t * _record_file{nullptr};
    BleServices _bleServices;
    bool _isActive{false};
    void initBleStack();
    void startAdvertising();
    void initGapParams();
    void initGatt();
    void initConnParameters();

    // Bonding-related methods
    void initBonding();
    static void pm_evt_handler(pm_evt_t const * p_evt);
    static void bonded_client_add(pm_evt_t const * p_evt);
    static void bonded_client_remove_all(void);
    static void on_bonded_peer_reconnection_lvl_notify(pm_evt_t const * p_evt);

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

    static const uint8_t SEC_PARAM_BOND = 1;
    static const uint8_t SEC_PARAM_MITM = 0;
    static const uint8_t SEC_PARAM_LESC = 0;
    static const uint8_t SEC_PARAM_KEYPRESS = 0;
    static const uint8_t SEC_PARAM_IO_CAPABILITIES = BLE_GAP_IO_CAPS_NONE;
    static const uint8_t SEC_PARAM_OOB = 0;
    static const uint8_t SEC_PARAM_MIN_KEY_SIZE = 7;
    static const uint8_t SEC_PARAM_MAX_KEY_SIZE = 16;
};

}

#endif 
