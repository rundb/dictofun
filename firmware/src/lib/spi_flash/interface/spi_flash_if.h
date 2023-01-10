// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include <stdint.h>
#include <cstddef>

namespace memory
{

class SpiNorFlashIf
{
public:
    enum class Result
    {
        OK,
        ERROR_ALIGNMENT,
        ERROR_BUSY,
        ERROR_GENERAL,
        ERROR_INPUT,
        ERROR_TIMEOUT,
    };

    /// @brief Perform a synchronous read access to NOR SPI Flash memory
    /// @param address 
    /// @param data 
    /// @param size 
    /// @return OK, if operation was successfull, error code otherwise
    virtual Result read(uint32_t address, uint8_t* data, uint32_t size) = 0;

    /// @brief Perform a synchronous write access to NOR SPI Flash memory
    /// @param address in the flash memory. Has to be aligned by PAGE_SIZE
    /// @param data 
    /// @param size in bytes. Has to be aligned by PAGE_SIZE
    /// @return OK, if operation was successfull, error code otherwise
    virtual Result program(uint32_t address, const uint8_t * const data, uint32_t size) = 0;

    /// @brief Perform a synchronous erase operation in NOR SPI Flash memory
    /// @param address in the flash memory. Has to be aligned by SECTOR_SIZE
    /// @param size in bytes. Has to be aligned by SECTOR_SIZE
    /// @return OK, if operation was successfull, error code otherwise
    virtual Result erase(uint32_t address, uint32_t size) = 0;

};

}
