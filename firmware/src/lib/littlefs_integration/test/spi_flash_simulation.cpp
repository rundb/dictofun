#include "spi_flash_simulation_api.h"
#include <cstring>

namespace memory 
{
namespace block_device 
{
namespace sim 
{

InRamFlash::InRamFlash(size_t sector_size, size_t page_size, size_t total_size)
: sector_size_{sector_size}
, page_size_{page_size}
, total_size_{total_size}
{
    memory_ = new uint8_t[total_size_];
    for (auto i = 0; i < total_size_; ++i)
    {
        memory_[i] = 0xFF;
    }
}

InRamFlash::~InRamFlash()
{
    delete [] memory_;
}

SpiNorFlashIf::Result InRamFlash::read(uint32_t address, uint8_t* data, uint32_t size)
{
    memcpy(data, &memory_[address], size);
    return SpiNorFlashIf::Result::OK;
}

SpiNorFlashIf::Result InRamFlash::program(uint32_t address, const uint8_t * const data, uint32_t size)
{
    if (address % page_size_ != 0 || size % page_size_ != 0)
    {
        return SpiNorFlashIf::Result::ERROR_ALIGNMENT;
    }

    memcpy(&memory_[address], data, size);
    return SpiNorFlashIf::Result::OK;
}

SpiNorFlashIf::Result InRamFlash::erase(uint32_t address, uint32_t size)
{
    if (address % sector_size_ != 0 || size % sector_size_ != 0)
    {
        return SpiNorFlashIf::Result::ERROR_ALIGNMENT;
    }
    for (auto i = address; i < address + size; ++i)
    {
        memory_[i] = 0xFF;
    }
    return SpiNorFlashIf::Result::OK;
}


}
}
}
