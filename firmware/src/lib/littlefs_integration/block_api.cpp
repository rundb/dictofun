// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "block_api.h"

namespace memory
{
namespace block_device
{

static memory::SpiNorFlashIf * flash_{nullptr};
static uint32_t sector_size_{0};
static uint32_t page_size_{0};
static uint32_t memory_size_{0};

void register_flash_device(memory::SpiNorFlashIf * flash, uint32_t sector_size, uint32_t page_size, uint32_t memory_size)
{
    flash_ = flash;
    sector_size_ = sector_size;
    page_size_ = page_size;
    memory_size_ = memory_size;
}

// TODO: this function should sooner or later be implemented, and file write throughput should be measured
bool is_sector_erased(lfs_block_t block)
{
    return false;
}

// TODO: this function should sooner or later be implemented, and file write throughput should be measured
void mark_sector_as_erased(lfs_block_t block)
{

}

// TODO: this function should sooner or later be implemented, and file write throughput should be measured
void mark_sector_as_needs_erasure(lfs_block_t block)
{
    
}

int read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    const auto read_result = flash_->read(block * sector_size_ + off, reinterpret_cast<uint8_t *>(buffer), size);
    if (read_result != memory::SpiNorFlashIf::Result::OK)
    {
        return -1;
    }
    return 0;
}

/// @brief Perform an LFS program operation. Only aligned offsets and sizes are supported
int program(const struct lfs_config *c, const lfs_block_t block, const lfs_off_t off, const void *buffer, const lfs_size_t size)
{
    if (size % page_size_ != 0)
    {
        return -1;   
    }

    if (0 != off % page_size_)
    {
        return -1;
    }

    for (auto page_id = 0; page_id < size / page_size_; ++page_id)
    {
        const auto address = block * sector_size_ + off + page_id * page_size_;
        const uint8_t * data_ptr = &(reinterpret_cast<const uint8_t *>(buffer))[page_id * page_size_];

        const auto program_result = flash_->program(address, data_ptr, size);
        if (program_result != memory::SpiNorFlashIf::Result::OK)
        {
            return -1;
        }
        lfs_block_t block_to_check{block};
        if ((off + page_id * page_size_) > sector_size_)
        {
            block_to_check += (off + page_id * page_size_) / sector_size_;
        }

        if (is_sector_erased(block_to_check))
        {
            mark_sector_as_needs_erasure(block_to_check);
        }
    }
    return 0;
}

int erase(const struct lfs_config *c, lfs_block_t block)
{
    const auto erase_result = flash_->erase(block * sector_size_, sector_size_);
    if (erase_result != memory::SpiNorFlashIf::Result::OK)
    {
        return -1;
    }
    return 0;
}

int sync(const struct lfs_config *c)
{
    return 0;
}

}
}
