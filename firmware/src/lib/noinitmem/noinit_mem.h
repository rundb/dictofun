// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include <cstdint>

namespace noinit
{

enum ResetReason: std::uint32_t 
{
    UNKNOWN,
    FILE_SYSTEM_ERROR_DURING_RECORD = 0x8E5A301FUL,
    FILE_SYSTEM_ERROR_DURING_BLE = 0x4B2F1D9CUL,
    ISSUE_3 = 0xA7C681F5UL,
    ISSUE_4 = 0x13D9BDA8UL,
};

static constexpr uint32_t VALID_MAGIC{0x8F1A2B3C};

// This class contains memory variables that are preserved after a restart of an MCU. 
#pragma pack(1)
struct NoInitMemory
{
    static void load();
    static void store();

    static NoInitMemory& instance();

    uint32_t _magic;
    ResetReason reset_reason;
    uint32_t reset_count;

    // This is an initial implementation variable, counting the amount of times system has booted in the current power cycle.
    uint32_t boot_count;

    uint32_t _crc;
private:
    void reset();
};
#pragma pack(0)

}
