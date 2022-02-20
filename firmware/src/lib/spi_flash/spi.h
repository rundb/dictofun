// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include <cstddef>
#include <stdint.h>
#include "nrf_drv_spi.h"

namespace spi
{

/**
 * This is a wrapper around Nordic SPI functions.
 * Its' purpose is to overcome limitations of the normal SPI driver 
 * (in particular, max transfer size of 128 bytes).
 */
class Spi
{
public:

    Spi(uint8_t instanceIdx, uint16_t csId);

    struct Configuration
    {
        nrf_drv_spi_frequency_t baudrate;
        nrf_drv_spi_mode_t mode;
        nrf_drv_spi_bit_order_t bitOrder;
        uint8_t sckPin;
        uint8_t mosiPin;
        uint8_t misoPin;
    };
    void init(const Configuration& configuration);

    enum class Result
    {
        OK,
        ERROR
    };
    using CompletionCallback = void(*)(Result);
    Result xfer(uint8_t* txData, uint8_t* rxData, size_t size);
    Result xfer(uint8_t* txData, uint8_t* rxData, size_t size, CompletionCallback callback);

    static inline Spi& getInstance() {return *_instance;}

    void isr();
    nrf_drv_spi_t _nrfSpiInstance;
private:
    // TODO: extend to several instances, if needed
    static Spi * _instance;

    uint8_t _instanceIdx;
    uint16_t _csId;

    struct Context
    {
        bool isBusy;
        uint8_t * txBuffer;
        uint8_t * rxBuffer;
        size_t bytesLeftToSend;
        size_t position;
        CompletionCallback callback;
    };

    volatile Context _context;

    void cleanContext();

    static const size_t MAX_SINGLE_TRANSACTION_LENGTH = 128U;
    static void emptyCallback(const Result) {}
};

} // namespace spi