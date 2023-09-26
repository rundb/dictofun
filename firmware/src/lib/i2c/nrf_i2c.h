// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "i2c_if.h"
#include "nrf_drv_twi.h"

#include <functional>

namespace i2c
{

class NrfI2c : public I2cIf
{
public:
    struct Config
    {
        uint8_t idx;
        uint8_t clk_pin;
        uint8_t data_pin;
        enum class Baudrate
        {
            _100K,
            _400K,
        } baudrate;
    };
    NrfI2c(const Config& config);
    NrfI2c(const NrfI2c&) = delete;
    NrfI2c(NrfI2c&&) = delete;
    NrfI2c& operator=(const NrfI2c&) = delete;
    NrfI2c& operator=(NrfI2c&&) = delete;
    ~NrfI2c() = default;

    i2c::Result init();

    i2c::Result read(uint8_t address, uint8_t* data, uint8_t size) override;

    i2c::Result write(uint8_t address, const uint8_t* const data, uint8_t size) override;

    i2c::Result write_read(uint8_t address,
                           const uint8_t* const tx_data,
                           uint8_t tx_size,
                           uint8_t* rx_data,
                           uint8_t rx_size) override;
    void register_completion_callback(CompletionCallback callback) override
    {
        _callback = callback;
    }

private:
    static constexpr uint32_t max_i2c_instances{2};
    static NrfI2c* _instances[max_i2c_instances];
    static NrfI2c& instance(const uint8_t idx)
    {
        return *_instances[idx];
    }
    const Config& _config;
    nrf_drv_twi_t& _twi_instance;
    nrf_drv_twi_t _twi_dummy;

    nrf_drv_twi_frequency_t get_frequency_config(Config::Baudrate baudrate) const;

    friend void twi_event_handler(nrf_drv_twi_evt_t const* p_event, void* p_context);
    void irq_handler(nrf_drv_twi_evt_t const* p_event);

    struct IrqContext
    {
        uint8_t id;
    };
    IrqContext _irq_context;
    CompletionCallback _callback{nullptr};

    struct TransactionContext
    {
        enum class Type
        {
            NONE,
            WRITE_READ,
            READ,
            WRITE
        } type{Type::NONE};
        uint8_t address;
        uint8_t* rx_data;
        uint8_t rx_data_size;
    } _transaction_context;
};

} // namespace i2c
