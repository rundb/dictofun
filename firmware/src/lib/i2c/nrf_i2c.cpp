// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "nrf_i2c.h"
#include "nrf_drv_twi.h"

//nrf_drv_twi_t twi_0 = NRF_DRV_TWI_INSTANCE(0);
nrf_drv_twi_t twi_1 = NRF_DRV_TWI_INSTANCE(1);

namespace i2c
{

NrfI2c * NrfI2c::_instances[max_i2c_instances] = {nullptr, nullptr };

NrfI2c::NrfI2c(const Config& config)
: _config{config}
, _twi_instance(config.idx == 1 ? twi_1 : _twi_dummy)
{
    _instances[config.idx] = this;
    _irq_context.id = config.idx;
}

nrf_drv_twi_frequency_t NrfI2c::get_frequency_config(const Config::Baudrate baudrate) const
{
    return (baudrate == Config::Baudrate::_400K) ? NRF_DRV_TWI_FREQ_400K : NRF_DRV_TWI_FREQ_100K;
}

void twi_event_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    const auto id = reinterpret_cast<NrfI2c::IrqContext *>(p_context)->id;
    NrfI2c::instance(id).irq_handler(p_event);
}

void NrfI2c::irq_handler(nrf_drv_twi_evt_t const * p_event)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
        {
            if (_callback != nullptr)
            {
                _callback(TransactionResult::COMPLETE);
            }
            break;
        }
        case NRF_DRV_TWI_EVT_ADDRESS_NACK:
        {
            if (_callback != nullptr)
            {
                _callback(TransactionResult::ERROR_NACK);
            }
            break;
        }
        case NRF_DRV_TWI_EVT_DATA_NACK:
        {
            if (_callback != nullptr)
            {
                _callback(TransactionResult::ERROR_NACK);
            }
            break;
        }
    }
}

i2c::Result NrfI2c::init()
{
    nrf_drv_twi_config_t twi_config{
        _config.clk_pin,
        _config.data_pin,
        get_frequency_config(_config.baudrate),
        1,
        false,
        false
    };
    const auto init_result = nrf_drv_twi_init(&_twi_instance,
                         &twi_config,
                         twi_event_handler,
                         &_irq_context);
    if (NRFX_SUCCESS != init_result)
    {
        return Result::ERROR_GENERAL;
    }  
    nrf_drv_twi_enable(&_twi_instance);

    return Result::OK;
}

i2c::Result NrfI2c::read(const uint8_t address, uint8_t * data, const uint8_t size)
{
    const auto rx_result = nrf_drv_twi_rx(
        &_twi_instance,
        address,
        data, 
        size
    );
    if (NRFX_SUCCESS != rx_result)
    {
        return Result::ERROR_GENERAL;
    }

    return Result::OK;
}

i2c::Result NrfI2c::write(const uint8_t address, const uint8_t * const data, const uint8_t size)
{
    const auto tx_result = nrf_drv_twi_tx(
        &_twi_instance,
        address,
        data, 
        size,
        false
    );
    if (NRFX_SUCCESS != tx_result)
    {
        return Result::ERROR_GENERAL;
    }
    return Result::OK;
}

i2c::Result NrfI2c::write_read(const uint8_t address, const uint8_t * const tx_data, const uint8_t tx_size, uint8_t * rx_data, const uint8_t rx_size)
{
    return Result::ERROR_NOT_IMPLEMENTED;
}

}
