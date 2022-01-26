/*
 * Copyright (c) 2022 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "spi_flash.h"
#include "boards/boards.h"
#include "nrf_gpio.h"
#include <cstring>

namespace flash
{
volatile bool SpiFlash::_isSpiOperationPending;
SpiFlash* SpiFlash::_instance{nullptr};

SpiFlash::SpiFlash(spi::Spi& flashSpi)
    : _spi(flashSpi)
    , _context({Operation::IDLE, 0, nullptr, 0})
{
    _isSpiOperationPending = false;
    if(_instance == nullptr)
    {
        _instance = this;
    }
}

void SpiFlash::init()
{
    nrf_gpio_cfg_output(SPI_FLASH_RST_PIN);
    nrf_gpio_cfg_output(SPI_FLASH_WP_PIN);

    nrf_gpio_pin_set(SPI_FLASH_RST_PIN);
    nrf_gpio_pin_set(SPI_FLASH_WP_PIN);
}

void SpiFlash::read(uint32_t address, uint8_t* data, uint32_t size)
{
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
}

void SpiFlash::program(uint32_t address, uint8_t* data, uint32_t size)
{
    // add alignment, size checks and nullptr-check
    while(isBusy())
        ;
    writeEnable(true);
    _txBuffer[0] = 0x2;
    _txBuffer[1] = (address >> 16) & 0xFF;
    _txBuffer[2] = (address >> 8) & 0xFF;
    _txBuffer[3] = (address)&0xFF;
    memcpy(&_txBuffer[4], data, size);

    _isSpiOperationPending = true;
    _spi.xfer(_txBuffer, _rxBuffer, size + 4, spiOperationCallback);
    _context.operation = Operation::WRITE;
    _context.data = data;
    _context.address = address;
    _context.size = size;
}

void SpiFlash::eraseSector(uint32_t address)
{
    if(address % ERASABLE_SIZE != 0)
    {
        // TODO: assert
        while(1)
            ;
    }
    while(isBusy())
        ;

    writeEnable(true);
    _isSpiOperationPending = true;
    uint8_t tx_data[] = {0xD8,
                         static_cast<uint8_t>((address >> 16) & 0xFF),
                         static_cast<uint8_t>((address >> 8) & 0xFF),
                         static_cast<uint8_t>((address)&0xFF)};
    uint8_t rx_data[] = {0, 0, 0, 0};
    _spi.xfer(tx_data, rx_data, 4);
    _context.operation = Operation::ERASE;
}

void SpiFlash::erase(uint32_t address, uint32_t size)
{
    while(isBusy())
        ;

    if((address % ERASABLE_SIZE != 0) || (size % ERASABLE_SIZE != 0))
    {
        // assert
        while(1)
            ;
    }
    for(int i = 0; i < size / ERASABLE_SIZE; ++i)
    {
        uint32_t addr = address + i * ERASABLE_SIZE;
        eraseSector(addr);
        while(isBusy())
            ;
    }
    _context.operation = Operation::IDLE;
}

bool SpiFlash::isBusy()
{
    if(_isSpiOperationPending)
    {
        return true;
    }
    return (getSR1() & 0x01) > 0U;
}

void SpiFlash::reset()
{
    _isSpiOperationPending = true;
    uint8_t tx_data[] = {0x99};
    uint8_t rx_data[] = {0xcc};
    _spi.xfer(tx_data, rx_data, 1, spiOperationCallback);

    while(_isSpiOperationPending)
        ;

    for(volatile int i = 0; i < 1000000; ++i)
        ;
}

uint8_t SpiFlash::getSR1()
{
    while(_isSpiOperationPending)
        ;

    uint8_t tx_data[] = {0x05, 0x00};
    uint8_t rx_data[] = {0xcc, 0xcc, 0xcc, 0xcc};

    _isSpiOperationPending = true;
    _spi.xfer(tx_data, rx_data, 2, spiOperationCallback);

    while(_isSpiOperationPending)
        ;
    return rx_data[1];
}

void SpiFlash::readJedecId(uint8_t* id)
{
    while(isBusy())
        ;
    _isSpiOperationPending = true;
    uint8_t tx_data[] = {0x9F, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t rx_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    _spi.xfer(tx_data, rx_data, 6, spiOperationCallback);
    while(_isSpiOperationPending)
        ;
    memcpy(id, &rx_data[1], 4);
}

void SpiFlash::writeEnable(bool shouldEnable)
{
    while(isBusy())
        ;

    _isSpiOperationPending = true;

    uint8_t tx_data[] = {0x06};
    uint8_t rx_data[] = {0xcc};
    if(!shouldEnable)
    {
        // disable
        tx_data[0] = 0x04;
    }
    _spi.xfer(tx_data, rx_data, 1, spiOperationCallback);
    while(_isSpiOperationPending)
        ;
}

void SpiFlash::eraseChip()
{
    writeEnable(true);
    const auto sr1 = getSR1();
    while(isBusy())
        ;
    _isSpiOperationPending = true;

    uint8_t tx_data[] = {0xC7};
    uint8_t rx_data[] = {0x0};

    _spi.xfer(tx_data, rx_data, 1, spiOperationCallback);
    while(_isSpiOperationPending)
        ;
}

void SpiFlash::spiOperationCallback(spi::Spi::Result result)
{
    _isSpiOperationPending = false;
    getInstance().completionCallback();
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
