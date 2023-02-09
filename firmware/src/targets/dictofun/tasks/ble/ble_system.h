// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "result.h"
#include "nrf_sdh.h"
#include "peer_manager.h"
#include "ble.h"
#include "ble_lbs.h"
#include "ble_fts.h"
#include "FreeRTOS.h"
#include "queue.h"

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

    result::Result configure(ble_lbs_led_write_handler_t led_write_handler);

    result::Result start();
    result::Result stop();
    void process();
    void connect_fts_to_target_fs();
    void register_fs_communication_queues(QueueHandle_t commands_queue, QueueHandle_t status_queue, QueueHandle_t data_queue);
private:
    result::Result init_sdh();
    result::Result init_gap();
    result::Result init_gatt();
    result::Result init_bonding();
    result::Result init_advertising();
    result::Result init_conn_params();

    static void start_advertising(void * context_ptr);

    static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
    static void pm_evt_handler(pm_evt_t const * p_evt);
    static void bonded_client_add(pm_evt_t const * p_evt);
    static void bonded_client_remove_all();
    static void on_bonded_peer_reconnection_lvl_notify(pm_evt_t const * p_evt);
};

}
