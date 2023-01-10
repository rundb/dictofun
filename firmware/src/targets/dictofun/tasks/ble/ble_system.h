// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "result.h"

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
};

}