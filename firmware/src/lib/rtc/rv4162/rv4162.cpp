// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "rv4162.h"
#include <functional>

namespace rtc
{

static volatile i2c::TransactionResult last_transaction_result{i2c::TransactionResult::NONE};
void i2c_completion_callback(i2c::TransactionResult result)
{
    last_transaction_result = result;
}

result::Result Rv4162::init()
{
    _i2c.register_completion_callback(i2c_completion_callback);
    uint8_t test_data[1] {0};
    last_transaction_result = i2c::TransactionResult::NONE;
    const auto i2c_tx_result = _i2c.read(_i2c_address, test_data, sizeof(test_data));
    if (i2c_tx_result != i2c::Result::OK)
    {
        return result::Result::ERROR_GENERAL;
    }
    static constexpr uint32_t timeout_value{1000000};
    uint32_t timeout{timeout_value};
    while (i2c::TransactionResult::NONE == last_transaction_result && ((timeout--) > 0));
    if (last_transaction_result != i2c::TransactionResult::COMPLETE)
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

result::Result Rv4162::get_date_time(DateTime& datetime)
{
    uint8_t tx_data[1] = {0x0};
    uint8_t rx_data[8];

    last_transaction_result = i2c::TransactionResult::NONE;
    const auto i2c_txrx_result = _i2c.write_read(_i2c_address, tx_data, sizeof(tx_data), rx_data, sizeof(rx_data));
    static constexpr uint32_t timeout_value{1000000};
    uint32_t timeout{timeout_value};
    while (i2c::TransactionResult::NONE == last_transaction_result && ((timeout--) > 0));
    if (last_transaction_result != i2c::TransactionResult::COMPLETE)
    {
        return result::Result::ERROR_GENERAL;
    }
    datetime.second = rx_data[1];
    datetime.minute = rx_data[2];
    datetime.hour = rx_data[3];
    datetime.day = rx_data[4];
    datetime.month = rx_data[5];
    datetime.year = rx_data[6];

    return result::Result::OK;
}

}