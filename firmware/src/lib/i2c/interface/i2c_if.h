// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include <stdint.h>
#include <functional>

namespace i2c
{

enum class Result: uint8_t
{
    OK,
    ERROR_NACK,
    ERROR_BUS_ERROR,
    ERROR_BUSY,
    ERROR_NOT_IMPLEMENTED,
    ERROR_GENERAL,
};

enum class TransactionResult
{
    COMPLETE,
    ERROR_NACK,
    ERROR_GENERAL,
    NONE,
};

class I2cIf 
{
public:
    virtual Result read(uint8_t address, uint8_t * data, uint8_t size) = 0;

    virtual Result write(uint8_t address, const uint8_t * data, uint8_t size) = 0;
    virtual Result write_read(uint8_t address, const uint8_t * const tx_data, uint8_t tx_size, uint8_t * rx_data, uint8_t rx_size) = 0;

    using CompletionCallback = std::function<void(TransactionResult)>;
    virtual void register_completion_callback(CompletionCallback callback) = 0;

};

}
