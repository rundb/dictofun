// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "block_api.h"

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
constexpr uint32_t sectors_erasure_max_bitmap_size{max_supported_memory_size / 8 /
                                                   min_supported_block_size};
static uint8_t sectors_erasure_bitmap[sectors_erasure_max_bitmap_size];
static uint32_t sectors_erasure_actual_bitmap_size_{0};

void register_flash_device(memory::SpiNorFlashIf* flash,
                           uint32_t sector_size,
                           uint32_t page_size,
                           uint32_t memory_size)
{
    flash_ = flash;
    sector_size_ = sector_size;
    page_size_ = page_size;
    memory_size_ = memory_size;
    erasure_bits_sector_start_ = memory_size_ - sector_size_;
    sectors_erasure_actual_bitmap_size_ = (memory_size_ / 8 / sector_size_) + 1;

    for(auto i = 0; i < sectors_erasure_actual_bitmap_size_ / page_size_; ++i)
    {
        const auto page_address = erasure_bits_sector_start_ + i * page_size_;
        flash_->read(page_address, &sectors_erasure_bitmap[i * page_size_], page_size_);
    }
}

bool is_sector_erased(lfs_block_t block)
{
    const auto byte_offset{block / 8};
    const auto bit_offset{block % 8};

    return (sectors_erasure_bitmap[byte_offset] & (1 << bit_offset)) > 0;
}

void mark_sector_as_erased(lfs_block_t block)
{
    const auto byte_offset{block / 8};
    const auto bit_offset{block % 8};

    sectors_erasure_bitmap[byte_offset] |= (1 << bit_offset);

    // erase the bitmap sector
    const auto erase_result = flash_->erase(erasure_bits_sector_start_, sector_size_);
    if(memory::SpiNorFlashIf::Result::OK != erase_result)
    {
        return;
    }
    for(auto i = 0; i < sectors_erasure_actual_bitmap_size_ / page_size_; ++i)
    {
        const auto page_address = erasure_bits_sector_start_ + i * page_size_;
        flash_->program(page_address, sectors_erasure_bitmap, page_size_);
    }
}

void mark_sector_as_needs_erasure(lfs_block_t block)
{
    const auto byte_offset{block / 8};
    const auto bit_offset{block % 8};

    sectors_erasure_bitmap[byte_offset] &= ~(1 << bit_offset);

    // let byte_offset be 5, then it is in page (5 / 256) * 256 = 0
    const auto byte_address = erasure_bits_sector_start_ + byte_offset;
    const auto result = flash_->program(byte_address, &sectors_erasure_bitmap[byte_offset], 1);
    if(memory::SpiNorFlashIf::Result::OK != result)
    {
        // TODO: define appropriate actions here
    }
    uint8_t readback_data{0};
    const auto readres = flash_->read(byte_address, &readback_data, 1);
    if(memory::SpiNorFlashIf::Result::OK != readres ||
       readback_data != sectors_erasure_bitmap[byte_offset])
    {
        // TODO: define appropriate actions here
    }
}

int read(
    const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    const auto read_result =
        flash_->read(block * sector_size_ + off, reinterpret_cast<uint8_t*>(buffer), size);
    if(read_result != memory::SpiNorFlashIf::Result::OK)
    {
        return -1;
    }
    return 0;
}

/// @brief Perform an LFS program operation. Only aligned offsets and sizes are supported
int program(const struct lfs_config* c,
            const lfs_block_t block,
            const lfs_off_t off,
            const void* buffer,
            const lfs_size_t size)
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
        lfs_block_t block_to_check{block};
        if((off + page_id * page_size_) > sector_size_)
        {
            block_to_check += (off + page_id * page_size_) / sector_size_;
        }

        if(is_sector_erased(block_to_check))
        {
            mark_sector_as_needs_erasure(block_to_check);
        }
    }
    return 0;
}

int erase(const struct lfs_config* c, lfs_block_t block)
{
    if(is_sector_erased(block))
    {
        return 0;
    }
    const auto erase_result = flash_->erase(block * sector_size_, sector_size_);
    if(erase_result != memory::SpiNorFlashIf::Result::OK)
    {
        return -1;
    }
    mark_sector_as_erased(block);

    return 0;
}

int sync(const struct lfs_config* c)
{
    return 0;
}

} // namespace block_device
} // namespace memory
