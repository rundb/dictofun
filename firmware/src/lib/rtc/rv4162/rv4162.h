// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "i2c_if.h"
#include "result.h"

namespace rtc
{

class Rv4162
{
public:
    explicit Rv4162(i2c::I2cIf& i2c)
    : _i2c{i2c}
    {

    }
    Rv4162(const Rv4162&) = delete;
    Rv4162(Rv4162&&) = delete;
    Rv4162& operator = (const Rv4162&) = delete;
    Rv4162& operator = (Rv4162&&) = delete;
    ~Rv4162() = default;

    struct DateTime 
    {
        uint8_t year;
        uint8_t month;
        uint8_t day;
        uint8_t weekday;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    result::Result init();

    result::Result get_date_time(DateTime& datetime);
    result::Result set_date_time(const DateTime& datetime);

private:
    i2c::I2cIf& _i2c;
    static constexpr uint8_t _i2c_address{0x68};
};


}
