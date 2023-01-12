// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "result.h"
#include "nrf_sdh.h"
#include "peer_manager.h"
#include "ble.h"

namespace ble
{

/// This class should aggregate all BLE subsystems and provide 
/// API to control BLE functionality.
class BleSystem
{
public:
    BleSystem() {}
    BleSystem(const BleSystem&) = delete;
    BleSystem(BleSystem&&) = delete;
    BleSystem& operator=(const BleSystem&) = delete;
    BleSystem& operator=(BleSystem&&) = delete;

     ~BleSystem() = default;

    result::Result configure();

    result::Result start();
    result::Result stop();
private:
    result::Result init_sdh();
    result::Result init_gap();
    result::Result init_gatt();
    result::Result init_bonding();
    result::Result init_advertising();
    result::Result init_conn_params();

    result::Result start_advertising();

    static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
    static void pm_evt_handler(pm_evt_t const * p_evt);
    static void bonded_client_add(pm_evt_t const * p_evt);
    static void bonded_client_remove_all();
    static void on_bonded_peer_reconnection_lvl_notify(pm_evt_t const * p_evt);
};

}
