// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "block_api_myfs.h"
#include "myfs.h"

namespace memory
{
namespace block_device
{

static memory::SpiNorFlashIf* flash_{nullptr};
static uint32_t sector_size_{0};
static uint32_t page_size_{0};
static uint32_t memory_size_{0};
static uint32_t erasure_bits_sector_start_{0};
constexpr uint32_t max_supported_memory_size{16 * 1024 * 1024};
constexpr uint32_t min_supported_block_size{4096};


void myfs_register_flash_device(memory::SpiNorFlashIf* flash,
                                uint32_t sector_size,
                                uint32_t page_size,
                                uint32_t memory_size)
{
    flash_ = flash;
    sector_size_ = sector_size;
    page_size_ = page_size;
    memory_size_ = memory_size;
}


int myfs_read(const struct ::filesystem::myfs_config* c,
              myfs_block_t block,
              myfs_off_t off,
              void* buffer,
              myfs_size_t size)
{
    const auto read_result =
        flash_->read(block * sector_size_ + off, reinterpret_cast<uint8_t*>(buffer), size);
    if(read_result != memory::SpiNorFlashIf::Result::OK)
    {
        return -1;
    }
    return 0;
}

int myfs_program(const struct ::filesystem::myfs_config* c,
                 const myfs_block_t block,
                 const myfs_off_t off,
                 const void* buffer,
                 const myfs_size_t size)
{
    if(size % page_size_ != 0)
    {
        return -1;
    }

    if(0 != off % page_size_)
    {
        return -1;
    }

    for(auto page_id = 0; page_id < size / page_size_; ++page_id)
    {
        const auto address = block * sector_size_ + off + page_id * page_size_;
        const uint8_t* data_ptr = &(reinterpret_cast<const uint8_t*>(buffer))[page_id * page_size_];

        const auto program_result = flash_->program(address, data_ptr, size);
        if(program_result != memory::SpiNorFlashIf::Result::OK)
        {
            return -1;
        }
    }
    return 0;
}

int myfs_erase(const struct ::filesystem::myfs_config* c, myfs_block_t block)
{
    const auto erase_result = flash_->erase(block * sector_size_, sector_size_);
    if(erase_result != memory::SpiNorFlashIf::Result::OK)
    {
        return -1;
    }

    return 0;
}

int myfs_erase_multiple(const struct ::filesystem::myfs_config* c, myfs_block_t block, uint32_t blocks_count)
{

    const auto erase_result = flash_->erase(block * sector_size_, sector_size_ * blocks_count);
    if (erase_result != memory::SpiNorFlashIf::Result::OK)
    {
        return -1;
    }

    return 0;
}

int myfs_sync(const struct ::filesystem::myfs_config* c)
{
    return 0;
}

} // namespace block_device
} // namespace memory
