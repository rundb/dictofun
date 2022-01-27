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

#pragma once

#include "spi.h"
#include <stdint.h>

namespace flash
{

class SpiFlash
{
public:
    SpiFlash(spi::Spi& flashSpi);

    void init();

    // Synchronous subset
    // ID has to be 8 bytes long
    void readJedecId(uint8_t* id);
    void reset();

    enum class Result
    {
        OK,
        ALIGNMENT_ERROR,
        ETC_ERROR,
    };

    // Asynchronous subset
    Result read(uint32_t address, uint8_t* data, uint32_t size);
    Result program(uint32_t address, const uint8_t * const data, uint32_t size);
    Result erase(uint32_t address, uint32_t size);

    // These 2 calls are asynchronous
    Result eraseSector(uint32_t address);
    void eraseChip();

    bool isBusy();

    uint8_t getSR1();

    static inline SpiFlash& getInstance() { return *_instance; }
private:
    spi::Spi& _spi;
    static SpiFlash * _instance;
    static const size_t MAX_TRANSACTION_SIZE = 265;
    uint8_t _txBuffer[MAX_TRANSACTION_SIZE];
    uint8_t _rxBuffer[MAX_TRANSACTION_SIZE];
    static const uint32_t SECTOR_SIZE = 0x1000;
    static const uint32_t PAGE_SIZE = 0x100;

    static void spiOperationCallback(spi::Spi::Result result);
    static volatile bool _isSpiOperationPending;
    void completionCallback();

    enum class Operation
    {
        IDLE,
        READ,
        WRITE,
        ERASE
    };
    struct Context
    {
        Operation operation;
        uint32_t address;
        uint8_t * data;
        size_t size;
    };

    Context _context;
    void writeEnable(bool shouldEnable);
};

} // namespace flash