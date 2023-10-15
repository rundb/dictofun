// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "crc32.h"
#include "noinit_mem.h"
#include "nrf_log.h"

namespace noinit
{

static NoInitMemory _noinit_memory __attribute__((section(".noinit")));;

static uint32_t get_noinit_memory_crc() {
    return crc32_compute(
        reinterpret_cast<const uint8_t*>(&_noinit_memory), sizeof(NoInitMemory) - sizeof(uint32_t), 0);
}

void NoInitMemory::load()
{
    const auto crc_calculated = get_noinit_memory_crc();
    auto& nim = instance();
    if (nim._magic != VALID_MAGIC && nim._crc != crc_calculated)
    {
        nim.reset();
    }
    else 
    {
        NRF_LOG_INFO("noinit mem: ok, boot = %d, reset = %d", nim.boot_count, nim.reset_count);
        nim.boot_count++;
        store();
    }
}

void NoInitMemory::reset()
{
    NRF_LOG_INFO("noinit mem: resetting")
    _magic = VALID_MAGIC;
    reset_count = 0;
    reset_reason = ResetReason::UNKNOWN;
    boot_count = 0;
    _crc = get_noinit_memory_crc();
}

void NoInitMemory::store()
{
    auto& nim = instance();
    if (nim._magic != VALID_MAGIC)
    {
        nim.reset();
    }
    nim._crc = get_noinit_memory_crc();
}

NoInitMemory& NoInitMemory::instance()
{
    return _noinit_memory;
}


}
