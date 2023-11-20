// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "spi.h"
#include "spi_flash_if.h"
#include <functional>
#include <stdint.h>

namespace flash
{

class SpiFlash : public memory::SpiNorFlashIf
{
public:
    using DelayFunction = std::function<void(uint32_t)>;
    using TickFunction = std::function<uint32_t(void)>;
    using Result = memory::SpiNorFlashIf::Result;
    explicit SpiFlash(spi::Spi& flashSpi, DelayFunction delay_function, TickFunction tick_function);

    SpiFlash() = delete;
    SpiFlash(const SpiFlash&) = delete;
    SpiFlash(SpiFlash&&) = delete;
    SpiFlash& operator=(const SpiFlash&) = delete;
    SpiFlash& operator=(SpiFlash&&) = delete;

    ~SpiFlash() = default;

    void init();

    // Synchronous subset
    // ID has to be 8 bytes long
    void readJedecId(uint8_t* id);
    void reset();

    // Interface implementation
    SpiNorFlashIf::Result read(uint32_t address, uint8_t* data, uint32_t size) override;
    SpiNorFlashIf::Result
    program(uint32_t address, const uint8_t* const data, uint32_t size) override;
    SpiNorFlashIf::Result erase(uint32_t address, uint32_t size) override;

    // These 2 calls are asynchronous
    Result eraseSector(uint32_t address);
    void eraseChip();
    SpiFlash::Result erase64KBlock(const uint32_t address);

    bool isBusy();

    uint8_t getSR1();

    static inline SpiFlash& getInstance()
    {
        return *_instance;
    }

private:
    spi::Spi& _spi;
    DelayFunction _delay;
    TickFunction _get_ticks;
    static SpiFlash* _instance;
    static const size_t MAX_TRANSACTION_SIZE = 265;
    uint8_t _txBuffer[MAX_TRANSACTION_SIZE];
    uint8_t _rxBuffer[MAX_TRANSACTION_SIZE];
    static const uint32_t SECTOR_SIZE = 0x1000;
    static const uint32_t B64K_SIZE = 0x10000;
    static const uint32_t PAGE_SIZE = 0x100;
    uint32_t _last_transaction_tick{0};

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
        uint8_t* data;
        size_t size;
    };

    static constexpr uint32_t short_delay_duration_ms{1};
    static constexpr uint32_t long_delay_duration_ms{200};
    static constexpr uint32_t max_wait_time_ms{5};
    static constexpr uint32_t max_read_transaction_time_ms{5};
    static constexpr uint32_t max_program_transaction_time_ms{10};
    static constexpr uint32_t max_erase_duration_time_ms{2000};

    Context _context;
    void writeEnable(bool shouldEnable);
};

} // namespace flash