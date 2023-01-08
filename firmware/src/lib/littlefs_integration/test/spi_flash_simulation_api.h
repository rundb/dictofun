// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "spi_flash_if.h"

// LittleFS integration process suggests implementation of methods that are used to access the flash memory.
namespace memory 
{
namespace block_device 
{
namespace sim 
{

class InRamFlash : public SpiNorFlashIf
{
public:
    InRamFlash(size_t sector_size, size_t page_size, size_t total_size);
    ~InRamFlash();

    Result read(uint32_t address, uint8_t* data, uint32_t size) override;
    Result program(uint32_t address, const uint8_t * const data, uint32_t size) override;
    Result erase(uint32_t address, uint32_t size) override;
private:
    size_t sector_size_;
    size_t page_size_;
    size_t total_size_;
    uint8_t * memory_{nullptr};
};

}
}
}