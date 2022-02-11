// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "spi.h"

#include "nrf_drv_spi.h"
#include "nrf_gpio.h"

namespace spi
{

Spi* Spi::_instance{nullptr};

Spi::Spi(const uint8_t instanceIdx, const uint16_t csId)
    : _instanceIdx(instanceIdx)
    , _csId(csId)
{
    if(_instance == nullptr)
    {
        _instance = this;
    }
    else
    {
      // assert
      while(1);
    }
    if(0U == instanceIdx)
    {
        _nrfSpiInstance = NRF_DRV_SPI_INSTANCE(0);
    }
    else if(1U == _instanceIdx)
    {
        //_nrfSpiInstance = NRF_DRV_SPI_INSTANCE(1);
    }
    else if(2U == _instanceIdx)
    {
        //_nrfSpiInstance = NRF_DRV_SPI_INSTANCE(2);
    }
}

void spi_event_handler(nrf_drv_spi_evt_t const* p_event, void* p_context)
{
    Spi::getInstance().isr();
}

void Spi::init(const Configuration& configuration)
{
    nrf_drv_spi_config_t const spi_config = {.sck_pin = configuration.sckPin,
                                             .mosi_pin = configuration.mosiPin,
                                             .miso_pin = configuration.misoPin,
                                             .ss_pin = NRF_DRV_SPI_PIN_NOT_USED,
                                             .irq_priority = 3,
                                             .orc = 0x00,
                                             .frequency = configuration.baudrate,
                                             .mode = configuration.mode,
                                             .bit_order = configuration.bitOrder};
    APP_ERROR_CHECK(nrf_drv_spi_init(&_nrfSpiInstance, &spi_config, spi_event_handler, NULL));
    cleanContext();
    nrf_gpio_cfg_output(_csId);
}

/**
 * This code is only responsible for SPI transactions. It's zero copy, so SPI driver is not responsible
 * for consistency of the data if it's used during the SPI transactions
 */
Spi::Result Spi::xfer(uint8_t* txData, uint8_t* rxData, size_t size, CompletionCallback callback)
{
    if(_context.isBusy)
    {
        return Spi::Result::ERROR;
    }
    // pull CS low. It's pulled back up in the interrupt.
    nrf_gpio_pin_clear(_csId);

    if(size <= MAX_SINGLE_TRANSACTION_LENGTH)
    {
        _context.isBusy = true;
        // ugly workaround for spi_transfer, which sends an extra byte if size == 1
        const auto result = nrf_drv_spi_transfer(&_nrfSpiInstance, txData, size, rxData, (size > 1) ? size : 0 );
        if(result == NRF_SUCCESS)
        {
            _context.txBuffer = txData;
            _context.rxBuffer = rxData;
            _context.bytesLeftToSend = 0;
            _context.position = 0;
            _context.callback = callback;
            return Spi::Result::OK;
        }
    }
    else
    {
        _context.isBusy = true;
        const auto result = nrf_drv_spi_transfer(&_nrfSpiInstance,
                                                 txData,
                                                 MAX_SINGLE_TRANSACTION_LENGTH,
                                                 rxData,
                                                 MAX_SINGLE_TRANSACTION_LENGTH);
        if(result == NRF_SUCCESS)
        {
            _context.txBuffer = txData;
            _context.rxBuffer = rxData;
            _context.bytesLeftToSend = size - MAX_SINGLE_TRANSACTION_LENGTH;
            return Spi::Result::OK;
        }
    }
    nrf_gpio_pin_set(_csId);
    return Spi::Result::ERROR;
}

void Spi::isr()
{
    if(!_context.isBusy)
    {
        // looks like an error
        return;
    }
    // TODO: add error checking
    if(_context.bytesLeftToSend == 0)
    {
        nrf_gpio_pin_set(_csId);
        _context.callback(Spi::Result::OK);
        cleanContext();
    }
    else
    {
        if(_context.bytesLeftToSend > MAX_SINGLE_TRANSACTION_LENGTH)
        {
            _context.bytesLeftToSend -= MAX_SINGLE_TRANSACTION_LENGTH;
            _context.position += MAX_SINGLE_TRANSACTION_LENGTH;
            nrf_drv_spi_transfer(&_nrfSpiInstance,
                                 &_context.txBuffer[_context.position],
                                 MAX_SINGLE_TRANSACTION_LENGTH,
                                 &_context.rxBuffer[_context.position],
                                 MAX_SINGLE_TRANSACTION_LENGTH);
        }
        else
        {
            //_context.bytesLeftToSend -= MAX_SINGLE_TRANSACTION_LENGTH;
            _context.position += MAX_SINGLE_TRANSACTION_LENGTH;
            nrf_drv_spi_transfer(&_nrfSpiInstance,
                                 &_context.txBuffer[_context.position],
                                 _context.bytesLeftToSend,
                                 &_context.rxBuffer[_context.position],
                                 _context.bytesLeftToSend);
            _context.bytesLeftToSend = 0U;
        }
    }
}

void Spi::cleanContext()
{
    _context.isBusy = false;
    _context.txBuffer = nullptr;
    _context.rxBuffer = nullptr;
    _context.bytesLeftToSend = 0;
    _context.position = 0;
}

} // namespace spi