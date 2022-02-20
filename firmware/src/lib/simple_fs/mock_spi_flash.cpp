// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#include "simple_fs.h"
#include <cstring>
#include <iostream>

using namespace std;

namespace test
{

// Simulating a standard 128Mbit memory (16 Mb)
constexpr size_t MOCK_MEMORY_SIZE = 16 * 1024 * 1024;
constexpr size_t PAGE_SIZE = 256;
constexpr size_t SECTOR_SIZE = 4096;
static uint8_t _memory_mock[MOCK_MEMORY_SIZE];
void initFsMock()
{
    for(size_t i = 0; i < MOCK_MEMORY_SIZE; ++i)
    {
        _memory_mock[i] = 0xFF;
    }
}

result::Result spiMockRead(const uint32_t address, uint8_t* data, const size_t size)
{
    if(address >= MOCK_MEMORY_SIZE)
        return result::Result::ERROR_INVALID_PARAMETER;
    memcpy(data, &_memory_mock[address], size);
    return result::Result::OK;
}
result::Result spiMockWrite(const uint32_t address, const uint8_t* const data, const size_t size)
{
    if(address >= MOCK_MEMORY_SIZE || (address % PAGE_SIZE != 0))
        return result::Result::ERROR_INVALID_PARAMETER;
    memcpy(&_memory_mock[address], data, size);
    return result::Result::OK;
}

result::Result spiMockErase(const uint32_t address, const size_t size)
{
    if(address % SECTOR_SIZE != 0 || size % SECTOR_SIZE != 0)
        return result::Result::ERROR_INVALID_PARAMETER;
    for(uint32_t pos = address; pos < address + size; ++pos)
    {
        _memory_mock[pos] = 0xFF;
    }
    return result::Result::OK;
}

void print_memory(size_t offset, size_t size)
{
    if(offset > sizeof(_memory_mock))
        return;
    size_t printed = 0;
    cout << offset << "\t";
    while(printed < size)
    {
        cout << hex << (int)_memory_mock[offset + printed] << " ";
        printed++;
        if((offset + printed) % 16 == 0)
            cout << endl << hex << (offset + printed) << "\t";
    }
}

filesystem::SpiFlashConfiguration test_spi_flash_config = {
    spiMockRead, spiMockWrite, spiMockErase, PAGE_SIZE, SECTOR_SIZE};

} // namespace test
