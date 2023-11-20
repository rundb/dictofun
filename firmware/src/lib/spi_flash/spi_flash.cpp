// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "spi_flash.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include <cstring>

namespace flash
{

static const uint32_t MAX_SPI_WAIT_TIMEOUT = 100000;

volatile bool SpiFlash::_isSpiOperationPending;
SpiFlash* SpiFlash::_instance{nullptr};

SpiFlash::SpiFlash(spi::Spi& flashSpi, DelayFunction delay_function, TickFunction tick_function)
    : _spi(flashSpi)
    , _delay(delay_function)
    , _get_ticks(tick_function)
    , _context({Operation::IDLE, 0, nullptr, 0})
{
    _isSpiOperationPending = false;
    if(_instance == nullptr)
    {
        _instance = this;
    }
    else
    {
        NRF_LOG_ERROR("spi: more than one instantiation in the system");
        while(1)
        {
            _delay(100);
        }
    }
}

void SpiFlash::init()
{
    nrf_gpio_cfg_output(SPI_FLASH_RST_PIN);
    nrf_gpio_cfg_output(SPI_FLASH_WP_PIN);

    nrf_gpio_pin_set(SPI_FLASH_RST_PIN);
    nrf_gpio_pin_set(SPI_FLASH_WP_PIN);
}

SpiFlash::Result SpiFlash::read(uint32_t address, uint8_t* data, uint32_t size)
{
    if(data == nullptr)
    {
        return Result::ERROR_INPUT;
    }

    if(size > MAX_TRANSACTION_SIZE)
    {
        return Result::ERROR_INPUT;
    }

    // TODO: consider reducing this delay to minimum (using more detailed ticks' source)
    if(_get_ticks() - _last_transaction_tick == 0)
    {
        _delay(1);
    }

    // 1. fill in transaction header
    _txBuffer[0] = 0x3U;
    _txBuffer[1] = (address >> 16) & 0xFF;
    _txBuffer[2] = (address >> 8) & 0xFF;
    _txBuffer[3] = (address)&0xFF;

    // 2. start the transaction
    _spi.xfer(_txBuffer, _rxBuffer, size + 4, spiOperationCallback);
    _isSpiOperationPending = true;

    _context.operation = Operation::READ;
    _context.data = data;
    _context.address = address;
    _context.size = size;

    uint32_t timeout{max_read_transaction_time_ms};
    while(_isSpiOperationPending && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(0 == timeout)
    {
        // TODO: define appropriate actions for this case
        // Apparently it's a critical error. Context is unknown at this point...
        NRF_LOG_ERROR("read: timeout error");
        return Result::ERROR_TIMEOUT;
    }

    _last_transaction_tick = _get_ticks();
    return Result::OK;
}

// TODO: might have to implement programming of several pages
SpiFlash::Result SpiFlash::program(uint32_t address, const uint8_t* const data, uint32_t size)
{

    if(data == nullptr)
    {
        return Result::ERROR_INPUT;
    }
    if(size > PAGE_SIZE)
    {
        return Result::ERROR_ALIGNMENT;
    }
    uint32_t timeout{max_wait_time_ms};
    while(isBusy() && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(timeout == 0)
    {
        return Result::ERROR_TIMEOUT;
    }

    if(_get_ticks() - _last_transaction_tick == 0)
    {
        _delay(1);
    }

    writeEnable(true);
    _txBuffer[0] = 0x2;
    _txBuffer[1] = (address >> 16) & 0xFF;
    _txBuffer[2] = (address >> 8) & 0xFF;
    _txBuffer[3] = (address)&0xFF;
    memcpy(&_txBuffer[4], data, size);

    _isSpiOperationPending = true;
    _spi.xfer(_txBuffer, _rxBuffer, size + 4, spiOperationCallback);
    _context.operation = Operation::WRITE;
    _context.data = (uint8_t*)data;
    _context.address = address;
    _context.size = size;

    timeout = max_program_transaction_time_ms;
    while(_isSpiOperationPending && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(0 == timeout)
    {
        // TODO: define appropriate actions for this case
    }

    _last_transaction_tick = _get_ticks();
    return Result::OK;
}

SpiFlash::Result SpiFlash::eraseSector(const uint32_t address)
{
    if(address % SECTOR_SIZE != 0)
    {
        return Result::ERROR_ALIGNMENT;
    }

    uint32_t timeout{max_wait_time_ms};
    while(isBusy() && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(timeout == 0)
    {
        return Result::ERROR_TIMEOUT;
    }

    if(_get_ticks() - _last_transaction_tick == 0)
    {
        _delay(1);
    }

    writeEnable(true);
    _isSpiOperationPending = true;
    _txBuffer[0] = 0x20;
    _txBuffer[1] = static_cast<uint8_t>((address >> 16) & 0xFF);
    _txBuffer[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
    _txBuffer[3] = static_cast<uint8_t>((address)&0xFF);

    _spi.xfer(_txBuffer, _rxBuffer, 4, spiOperationCallback);
    _context.operation = Operation::ERASE;
    _last_transaction_tick = _get_ticks();
    return Result::OK;
}

SpiFlash::Result SpiFlash::erase64KBlock(const uint32_t address)
{
    if (address % B64K_SIZE != 0)
    {
        return Result::ERROR_ALIGNMENT;
    }
    uint32_t timeout{max_wait_time_ms};
    while(isBusy() && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(timeout == 0)
    {
        return Result::ERROR_TIMEOUT;
    }

    if(_get_ticks() - _last_transaction_tick == 0)
    {
        _delay(1);
    }

    writeEnable(true);
    _isSpiOperationPending = true;
    _txBuffer[0] = 0xD8;
    _txBuffer[1] = static_cast<uint8_t>((address >> 16) & 0xFF);
    _txBuffer[2] = static_cast<uint8_t>((address >> 8) & 0xFF);
    _txBuffer[3] = static_cast<uint8_t>((address)&0xFF);

    _spi.xfer(_txBuffer, _rxBuffer, 4, spiOperationCallback);
    _context.operation = Operation::ERASE;
    _last_transaction_tick = _get_ticks();
    return Result::OK;
}

SpiFlash::Result SpiFlash::erase(const uint32_t address, const uint32_t size)
{

    if((address % SECTOR_SIZE != 0) || (size % SECTOR_SIZE != 0))
    {
        return Result::ERROR_ALIGNMENT;
    }

    uint32_t timeout{max_wait_time_ms};
    while(isBusy() && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(timeout == 0)
    {
        return Result::ERROR_TIMEOUT;
    }

    if((address % SECTOR_SIZE != 0) || (size % SECTOR_SIZE != 0))
    {
        return Result::ERROR_ALIGNMENT;
    }
    volatile int32_t position = static_cast<int32_t>(address);
    volatile int32_t leftover = static_cast<int32_t>(size);
    while (leftover > 0)
    {
        Result res{Result::ERROR_GENERAL};
        int32_t current_erase_size{0};
        if (leftover >= static_cast<int32_t>(B64K_SIZE) && ((position % B64K_SIZE) == 0))
        {
            res = erase64KBlock(position);
            current_erase_size = B64K_SIZE;
        }
        else 
        {
            res = eraseSector(position);
            current_erase_size = SECTOR_SIZE;
        }
        NRF_LOG_INFO("erasing %d@%x, %d left, res=%d", current_erase_size, position, leftover);
        if (res != Result::OK)
        {
            _context.operation = Operation::IDLE;
            return res;
        }
        timeout = max_erase_duration_time_ms;
        while(isBusy() && timeout > 0)
        {
            _delay(short_delay_duration_ms);
            --timeout;
        }
        if(timeout == 0)
        {
            return Result::ERROR_TIMEOUT;
        }
        leftover = leftover - current_erase_size;
        position += current_erase_size;
    }

    _context.operation = Operation::IDLE;
    return Result::OK;
}

bool SpiFlash::isBusy()
{
    if(_isSpiOperationPending)
    {
        return true;
    }
    auto sr1 = getSR1();
    auto result = (sr1 & 0x01) > 0U;
    return result;
}

void SpiFlash::reset()
{
    _isSpiOperationPending = true;
    uint8_t tx_data[] = {0x99};
    uint8_t rx_data[] = {0x00};
    _spi.xfer(tx_data, rx_data, 1, spiOperationCallback);

    volatile uint32_t timeout = MAX_SPI_WAIT_TIMEOUT;
    while(_isSpiOperationPending && ((timeout--) > 0))
    { }

    for(volatile int i = 0; i < 1000000; ++i)
        ;
}

uint8_t SpiFlash::getSR1()
{
    volatile uint32_t timeout = MAX_SPI_WAIT_TIMEOUT;
    while(_isSpiOperationPending && ((timeout--) > 0))
        ;

    uint8_t tx_data[] = {0x05, 0x00};
    uint8_t rx_data[] = {0xcc, 0xcc, 0x00, 0x00};

    _isSpiOperationPending = true;
    _spi.xfer(tx_data, rx_data, 2, spiOperationCallback);

    timeout = MAX_SPI_WAIT_TIMEOUT;
    while(_isSpiOperationPending && ((timeout--) > 0))
        ;
    return rx_data[1];
}

void SpiFlash::readJedecId(uint8_t* id)
{
    uint32_t timeout{max_wait_time_ms};
    while(isBusy() && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(timeout == 0)
    {
        return;
    }

    _isSpiOperationPending = true;
    uint8_t tx_data[] = {0x9F, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t rx_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    _spi.xfer(tx_data, rx_data, 6, spiOperationCallback);

    timeout = max_wait_time_ms;
    while(_isSpiOperationPending && ((timeout--) > 0))
    {
        _delay(short_delay_duration_ms);
    }
    memcpy(id, &rx_data[1], 4);
}

// TODO: add return type
void SpiFlash::writeEnable(bool shouldEnable)
{
    uint32_t timeout{max_wait_time_ms};
    while(isBusy() && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(timeout == 0)
    {
        return;
    }

    _isSpiOperationPending = true;

    uint8_t tx_data[] = {0x06};
    uint8_t rx_data[] = {0xcc};
    if(!shouldEnable)
    {
        // disable
        tx_data[0] = 0x04;
    }
    _spi.xfer(tx_data, rx_data, 1, spiOperationCallback);
    timeout = MAX_SPI_WAIT_TIMEOUT;
    while(_isSpiOperationPending && ((timeout--) > 0))
        ;
}

// TODO: add return type
void SpiFlash::eraseChip()
{
    writeEnable(true);

    uint32_t timeout{long_delay_duration_ms};
    while(isBusy() && timeout > 0)
    {
        _delay(short_delay_duration_ms);
        --timeout;
    }
    if(timeout == 0)
    {
        return;
    }

    _isSpiOperationPending = true;

    uint8_t tx_data[] = {0xC7};
    uint8_t rx_data[] = {0x0};

    _spi.xfer(tx_data, rx_data, 1, spiOperationCallback);
    timeout = MAX_SPI_WAIT_TIMEOUT;
    while(_isSpiOperationPending && ((timeout--) > 0))
        ;
}

void SpiFlash::spiOperationCallback(spi::Spi::Result result)
{
    getInstance().completionCallback();
    _isSpiOperationPending = false;
}

void SpiFlash::completionCallback()
{
    switch(_context.operation)
    {
    case Operation::READ: {
        memcpy(_context.data, &_rxBuffer[4], _context.size);
        _context.operation = Operation::IDLE;
        break;
    }
    default: {
        break;
    }
    }
}

} // namespace flash
