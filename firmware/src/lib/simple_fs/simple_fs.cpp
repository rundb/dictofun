// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "simple_fs.h"
#include <cstring>

namespace filesystem
{

struct FilesystemState
{
    bool is_initialized;
};

FilesystemState _state{
    .is_initialized = false,
};

static const uint32_t MAGIC_FILE_OPEN = 0xBABABEBE;
static const uint32_t MAGIC_FILE_CLOSED = 0xC0C0DADA;
static const uint32_t FILE_VALID_MAGIC = 0x92F61455UL;

static const uint32_t MAGIC_FILE_WRITTEN_VALID_MASK = 0xFFFF0000;
static const uint32_t MAGIC_FILE_WRITTEN_VALID_VALUE = 0xDEFFFFFF;

static const uint32_t MAGIC_FILE_WRITTEN_INVALID_MASK = 0xFFFF0000;
static const uint32_t MAGIC_FILE_WRITTEN_INVALID_VALUE = 0xDEADFFFF;

static const uint32_t CHECKSUM_MASK = 0x0000FFFF;

SpiFlashConfiguration _spi_flash_configuration;

result::Result init(const SpiFlashConfiguration& spi_flash_configuration)
{
    _state.is_initialized = true;
    // todo: check how copy constructor would work here
    _spi_flash_configuration = spi_flash_configuration;
    // currently sizes != 256 are not implemented

    return result::Result::OK;
}

static const size_t HEADER_SIZE = 4U;
static const size_t NEXT_FILE_POINTER_OFFSET = 0;
static const size_t FILE_SIZE_POINTER_OFFSET = 1;
static const size_t FILE_STATE_FLAGS_OFFSET = 3;
void read_next_file_header(const uint32_t address, uint32_t* header)
{
    auto& config = _spi_flash_configuration;
    result::Result res;
    do
    {
        res = config.read(
            address, reinterpret_cast<uint8_t*>(header), HEADER_SIZE * sizeof(uint32_t));
    } while(res != result::Result::OK);
}

// make sure to fill in corresponding fields in file.rom before calling this checksum
uint16_t get_file_checksum(File& file)
{
    const auto w0 = static_cast<uint16_t>(file.rom.next_file_start & 0xFFFF);
    const auto w1 = static_cast<uint16_t>((file.rom.next_file_start & 0xFFFF0000) >> 16);
    const auto w2 = static_cast<uint16_t>(file.rom.size & 0xFFFF);
    const auto w3 = static_cast<uint16_t>((file.rom.size & 0xFFFF0000) >> 16);
    const auto w4 = static_cast<uint16_t>(file.rom.magic & 0xFFFF);
    const auto w5 = static_cast<uint16_t>((file.rom.magic & 0xFFFF0000) >> 16);
    return w0 ^ w1 ^ w2 ^ w3 ^ w4 ^ w5;
}

bool is_file_open(File& file)
{
    return file.ram.runtime_magic == MAGIC_FILE_OPEN;
}

result::Result open(File& file, FileMode mode)
{
    if(is_file_open(file))
    {
        // file is already open
        return result::Result::ERROR_GENERAL;
    }
    if(mode == FileMode::WRONLY)
    {
        file.ram.runtime_magic = MAGIC_FILE_OPEN;
        bool is_last_file_found{false};
        uint32_t current_file_address{0x00};
        uint32_t header[HEADER_SIZE]{0, 0, 0, 0};
        while(!is_last_file_found)
        {
            read_next_file_header(current_file_address, header);
            const auto next_address = header[NEXT_FILE_POINTER_OFFSET];
            if(next_address < 0x01000000)
            {
                current_file_address = next_address;
                continue;
            }
            is_last_file_found = true;
            file.ram.current_file_start_address = current_file_address;
            file.ram.current_operation_address =
                current_file_address + _spi_flash_configuration.page_size;
            file.ram.size = 0;
            file.ram.is_write = true;
        }
        return result::Result::OK;
    }
    else if(mode == FileMode::RDONLY)
    {
        file.ram.runtime_magic = MAGIC_FILE_OPEN;
        bool is_first_valid_file_found{false};
        uint32_t current_file_address{0x00};
        uint32_t header[HEADER_SIZE]{0, 0, 0, 0};

        while(!is_first_valid_file_found)
        {
            read_next_file_header(current_file_address, header);
            const auto next_address = header[NEXT_FILE_POINTER_OFFSET];
            const auto file_state_flags = header[FILE_STATE_FLAGS_OFFSET];
            // in this case first file valid for read is found
            if((file_state_flags & MAGIC_FILE_WRITTEN_VALID_MASK) ==
               (MAGIC_FILE_WRITTEN_VALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK))
            {
                //TODO: clarify action
            }
            else if((file_state_flags & MAGIC_FILE_WRITTEN_VALID_MASK) ==
                    (MAGIC_FILE_WRITTEN_INVALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK))
            {
                current_file_address = next_address;
                continue;
            }
            else
            {
                // some malfunction, or reached end of files
                return result::Result::ERROR_GENERAL;
            }
            is_first_valid_file_found = true;
            // TODO: add validity check for the header
            file.ram.current_file_start_address = current_file_address;
            file.ram.current_operation_address =
                current_file_address + _spi_flash_configuration.page_size;
            file.ram.size = header[FILE_SIZE_POINTER_OFFSET];
            file.ram.is_write = false;
            file.rom.size = file.ram.size;
            file.rom.next_file_start = header[NEXT_FILE_POINTER_OFFSET];
        }

        return result::Result::OK;
    }

    return result::Result::ERROR_NOT_IMPLEMENTED;
}

FilesCount get_files_count()
{
    FilesCount result{0, 0};
    bool is_last_file_found{false};
    uint32_t current_file_address{0x00};
    uint32_t header[HEADER_SIZE]{0, 0, 0, 0};
    while(!is_last_file_found)
    {
        if(current_file_address > 0x00100000)
        {
            is_last_file_found = true;
            continue;
        }

        read_next_file_header(current_file_address, header);
        const auto next_address = header[NEXT_FILE_POINTER_OFFSET];
        const auto file_state_flags = header[FILE_STATE_FLAGS_OFFSET];
        current_file_address = next_address;

        // 0xDEFF0000
        constexpr auto FILE_VALID_MASK_VALUE =
            MAGIC_FILE_WRITTEN_VALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK;

        // 0xDEAD0000
        constexpr auto FILE_INVALID_MASK_VALUE =
            MAGIC_FILE_WRITTEN_INVALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK;

        const auto file_state_masked_value = file_state_flags & MAGIC_FILE_WRITTEN_VALID_MASK;
        if(file_state_masked_value == FILE_VALID_MASK_VALUE)
        {
            result.valid++;
        }
        else if(file_state_masked_value == FILE_INVALID_MASK_VALUE)
        {
            result.invalid++;
        }
        else
        {
            return result;
        }
    }

    return result;
}

size_t get_occupied_memory_size()
{
    size_t result{0};
    bool is_last_file_found{false};
    uint32_t current_file_address{0x00};
    uint32_t header[HEADER_SIZE]{0, 0, 0, 0};
    while(!is_last_file_found)
    {
        if(current_file_address > 0x00100000)
        {
            is_last_file_found = true;
            continue;
        }

        read_next_file_header(current_file_address, header);
        const auto next_address = header[NEXT_FILE_POINTER_OFFSET];
        const auto file_state_flags = header[FILE_STATE_FLAGS_OFFSET];
        const auto file_size = header[FILE_SIZE_POINTER_OFFSET];

        // 0xDEFF0000
        constexpr auto FILE_VALID_MASK_VALUE =
            MAGIC_FILE_WRITTEN_VALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK;

        // 0xDEAD0000
        constexpr auto FILE_INVALID_MASK_VALUE =
            MAGIC_FILE_WRITTEN_INVALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK;

        const auto file_state_masked_value = file_state_flags & MAGIC_FILE_WRITTEN_VALID_MASK;
        if((file_state_masked_value != FILE_VALID_MASK_VALUE) &&
           (file_state_masked_value != FILE_INVALID_MASK_VALUE))
        {
            auto next_file_start_pointer = current_file_address + file_size;
            if(next_file_start_pointer % _spi_flash_configuration.sector_size != 0)
            {
                next_file_start_pointer =
                    ((next_file_start_pointer / _spi_flash_configuration.sector_size) + 1) *
                    _spi_flash_configuration.sector_size;
            }
            return next_file_start_pointer;
        }
        result = current_file_address;
        current_file_address = next_address;
    }

    return result;
}

result::Result write(File& file, uint8_t* const data, const size_t size)
{
    if(!is_file_open(file) || !file.ram.is_write)
    {
        return result::Result::ERROR_GENERAL;
    }
    // todo: implement sequential write of >1 pages

    const auto res = _spi_flash_configuration.write(file.ram.current_operation_address, data, size);
    if(res != result::Result::OK)
    {
        return res;
    }
    file.ram.current_operation_address += size;
    file.ram.size += size;

    return result::Result::OK;
}

result::Result read(File& file, uint8_t* data, const size_t size, size_t& read_size)
{
    if(!is_file_open(file) || file.ram.is_write)
    {
        return result::Result::ERROR_GENERAL;
    }

    auto real_read_size = size;
    if(file.ram.size < size)
    {
        real_read_size = file.ram.size;
    }

    const auto res =
        _spi_flash_configuration.read(file.ram.current_operation_address, data, real_read_size);
    if(res != result::Result::OK)
    {
        return res;
    }
    read_size = real_read_size;
    file.ram.size -= real_read_size;
    file.ram.current_operation_address += real_read_size;
    return result::Result::OK;
}

result::Result close(File& file)
{
    if(!is_file_open(file))
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    auto& config = _spi_flash_configuration;
    if(file.ram.is_write)
    {
        // finalize file write
        file.rom.next_file_start =
            file.ram.current_file_start_address + config.page_size + file.ram.size;
        // Now make sure that next file starts at the boundary of a sector
        if(file.rom.next_file_start % config.sector_size != 0)
        {
            file.rom.next_file_start =
                ((file.rom.next_file_start / config.sector_size) + 1) * config.sector_size;
        }
        file.rom.size = file.ram.size;
        file.rom.magic = FILE_VALID_MAGIC;
        const auto checksum = get_file_checksum(file);
        file.rom.file_state_flags =
            (MAGIC_FILE_WRITTEN_VALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK) | checksum;

        constexpr auto FILE_ROM_SIZE = sizeof(File::RomDescriptor);
        uint8_t file_rom_packed[FILE_ROM_SIZE];
        memcpy(file_rom_packed, &file.rom, FILE_ROM_SIZE);

        const auto res =
            config.write(file.ram.current_file_start_address, file_rom_packed, FILE_ROM_SIZE);
        if(res != result::Result::OK)
        {
            return res;
        }
        // invalidate file variable
        file.ram.runtime_magic = 0;
        return result::Result::OK;
    }
    else
    {
        // finalize file read
        file.rom.file_state_flags &= ~(MAGIC_FILE_WRITTEN_VALID_MASK);
        file.rom.file_state_flags |=
            (MAGIC_FILE_WRITTEN_INVALID_VALUE & MAGIC_FILE_WRITTEN_VALID_MASK);

        constexpr auto FILE_ROM_SIZE = sizeof(File::RomDescriptor);
        uint8_t file_rom_packed[FILE_ROM_SIZE];
        memcpy(file_rom_packed, &file.rom, FILE_ROM_SIZE);

        const auto res =
            config.write(file.ram.current_file_start_address, file_rom_packed, FILE_ROM_SIZE);
        if(res != result::Result::OK)
        {
            return res;
        }
        file.ram.runtime_magic = 0;
        return result::Result::OK;
    }
    // unreachable
    return result::Result::ERROR_INVALID_PARAMETER;
}

} // namespace filesystem
