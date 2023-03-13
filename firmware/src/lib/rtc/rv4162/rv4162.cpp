// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "rv4162.h"
#include <functional>

#include "nrf_log.h"

namespace rtc
{

static volatile i2c::TransactionResult last_transaction_result{i2c::TransactionResult::NONE};
void i2c_completion_callback(i2c::TransactionResult result)
{
    last_transaction_result = result;
}

uint8_t bcd_to_dec(const uint8_t bcd, const uint8_t bits_count)
{
    uint8_t low_nibble{bcd & 0xFU};
    uint8_t high_nibble{(bcd & 0xF0U) >> 4};
    return low_nibble + high_nibble * 10;
}

uint8_t dec_to_bcd(const uint8_t dec)
{
    uint8_t low_nibble{static_cast<uint8_t>((dec % 10) & 0x0FU)};
    uint8_t high_nibble{static_cast<uint8_t>(dec / 10)};
    return low_nibble | (high_nibble << 4);
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
    if (i2c::Result::OK != i2c_txrx_result)
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
    datetime.second = bcd_to_dec(rx_data[1], 7);
    datetime.minute = bcd_to_dec(rx_data[2], 7);
    datetime.hour = bcd_to_dec(rx_data[3], 6);
    datetime.day = bcd_to_dec(rx_data[5], 6);
    datetime.month = bcd_to_dec(rx_data[6], 5);
    datetime.year = bcd_to_dec(rx_data[7], 8);

    return result::Result::OK;
}

result::Result Rv4162::set_date_time(const DateTime& datetime)
{
    static constexpr uint8_t tx_data_size{8};
    uint8_t tx_data[tx_data_size]{
        1U,
        dec_to_bcd(datetime.second),
        dec_to_bcd(datetime.minute),
        dec_to_bcd(datetime.hour),
        dec_to_bcd(datetime.weekday),
        dec_to_bcd(datetime.day),
        dec_to_bcd(datetime.month),
        dec_to_bcd(datetime.year)
    };
    last_transaction_result = i2c::TransactionResult::NONE;
    const auto tx_result = _i2c.write(_i2c_address, tx_data, sizeof(tx_data));

    static constexpr uint32_t timeout_value{1000000};
    uint32_t timeout{timeout_value};
    while (i2c::TransactionResult::NONE == last_transaction_result && ((timeout--) > 0));
    if (last_transaction_result != i2c::TransactionResult::COMPLETE)
    {
        return result::Result::ERROR_GENERAL;
    }
    return (tx_result == i2c::Result::OK) ? result::Result::OK : result::Result::ERROR_GENERAL;
}

}
