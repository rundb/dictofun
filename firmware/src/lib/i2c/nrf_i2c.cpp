// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "nrf_i2c.h"

namespace i2c
{

NrfI2c::NrfI2c(const Config& config){}

i2c::Result NrfI2c::init()
{
    return Result::ERROR_NOT_IMPLEMENTED;
}

i2c::Result NrfI2c::read(const uint8_t address, uint8_t * data, const uint8_t size)
{
    return Result::ERROR_NOT_IMPLEMENTED;
}

i2c::Result NrfI2c::write(const uint8_t address, const uint8_t * const data, const uint8_t size)
{
    return Result::ERROR_NOT_IMPLEMENTED;
}

i2c::Result NrfI2c::write_read(const uint8_t address, const uint8_t * const tx_data, const uint8_t tx_size, uint8_t * rx_data, const uint8_t rx_size)
{
    return Result::ERROR_NOT_IMPLEMENTED;
}

}