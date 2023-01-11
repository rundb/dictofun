// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "result.h"
#include <cstdint>
#include <stdint.h>
#include "ble_advertising.h"
#include "nrf_ble_qwr.h"

/// This module aggregates instantiations of all BLE services
namespace ble
{

result::Result init_services();

nrf_ble_qwr_t& get_qwr_handle();

size_t get_services_uuids(ble_uuid_t * service_uuids, size_t max_uuids);

}
