// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "ble_advertising.h"
#include "ble_fts.h"
#include "nrf_ble_qwr.h"
#include "queue.h"
#include "result.h"
#include "time.h"
#include <cstdint>
#include <stdint.h>

/// This module aggregates instantiations of all BLE services
namespace ble
{

result::Result init_services();

nrf_ble_qwr_t* get_qwr_handle();

size_t get_services_uuids(ble_uuid_t* service_uuids, size_t max_uuids);

void services_process();
void set_fts_fs_handler(ble::fts::FileSystemInterface& fs_if);
namespace services
{
void register_fs_communication_queues(QueueHandle_t commands_queue,
                                      QueueHandle_t status_queue,
                                      QueueHandle_t data_queue);
void register_keepalive_queue(QueueHandle_t keepalive_queue);
void register_fts_to_state_status_queue(QueueHandle_t status_queue);
bool is_fts_active();
void start_db_discovery(uint16_t conn_handle);
void request_current_time();

bool is_current_time_update_pending();
result::Result get_current_time_update(time::DateTime& datetime);

void set_bas_battery_level(uint8_t batt_level, const bool update_bas);

void restart();
void register_disconnect_delegate(std::function<void()> disconnect_delegate);

} // namespace services
} // namespace ble
