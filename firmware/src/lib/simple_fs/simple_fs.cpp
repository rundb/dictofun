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

static File * current_file{nullptr};

SpiFlashConfiguration _spi_flash_configuration;

result::Result init(const SpiFlashConfiguration& spi_flash_configuration)
{

    _state.is_initialized = true;
    // todo: check how copy constructor would work here
    _spi_flash_configuration = spi_flash_configuration;
    // currently sizes != 256 are not implemented

    current_file = nullptr;

    return result::Result::OK;
}

void deinit()
{
    _state.is_initialized = false;
    current_file = nullptr;
}

constexpr size_t MAX_FILES_COUNT{50U};

static const size_t HEADER_SIZE = 4U;
static const size_t NEXT_FILE_POINTER_OFFSET = 0;
static const size_t FILE_SIZE_POINTER_OFFSET = 1;
static const size_t FILE_MAGIC_OFFSET = 2;
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

FileState get_file_state(const uint32_t state_flags)
{
    if (state_flags == FILE_OPEN_FOR_WRITE) return FILE_OPEN_FOR_WRITE;
    if (state_flags == FILE_CLOSED_AFTER_WRITE) return FILE_CLOSED_AFTER_WRITE;
    if (state_flags == FILE_INVALIDATED) return FILE_INVALIDATED;
    return FILE_ERROR_STATE;
}

bool is_file_open()
{
    return nullptr != current_file;
}

result::Result create(File& file)
{
    if (is_file_open())
    {
        return result::Result::ERROR_BUSY;
    }
    // Iterate through the files and find either first empty header or first 
    // file that has been open for write and not closed.
    bool is_last_file_found{false};
    uint32_t current_file_address{0x00};
    uint32_t header[HEADER_SIZE]{0, 0, 0, 0};

    uint32_t iterations_counter{0UL};
    
    while (!is_last_file_found && iterations_counter < MAX_FILES_COUNT)
    {
        ++iterations_counter;
        read_next_file_header(current_file_address, header);
        const auto next_address = header[NEXT_FILE_POINTER_OFFSET];
        const auto magic = header[FILE_MAGIC_OFFSET];
        const auto file_status = header[FILE_STATE_FLAGS_OFFSET];
        if (magic == FILE_VALID_MAGIC)
        {
            // At this point we are confident that discovered header is valid
            // and previous file has been completely written. 
            current_file_address = next_address;
            continue;
        }
        const auto file_state = get_file_state(file_status);
        if (file_state == FILE_OPEN_FOR_WRITE)
        {
            return result::Result::ERROR_GENERAL;
        }
        // At this point magic is not present and we are sure that previously
        // open file has been properly written. So here should be the empty area.
        is_last_file_found = true;

        file.ram.current_file_start_address = current_file_address;
        file.ram.current_operation_address =
            current_file_address + _spi_flash_configuration.page_size;
        file.ram.size = 0;
        file.ram.is_write = true;
    }
    if (iterations_counter == MAX_FILES_COUNT)
    {
        return result::Result::ERROR_GENERAL;
    }
    current_file = &file;
    // TODO: mark file as open for write. 
    return result::Result::OK;
}

result::Result open(File& file)
{
    if (is_file_open())
    {
        return result::Result::ERROR_BUSY;
    }

    bool is_first_valid_file_found{false};
    uint32_t current_file_address{0x00};
    uint32_t header[HEADER_SIZE]{0, 0, 0, 0};
    uint32_t iterations_counter{0UL};

    while(!is_first_valid_file_found && iterations_counter < MAX_FILES_COUNT)
    {
        ++iterations_counter;
        read_next_file_header(current_file_address, header);
        const auto next_address = header[NEXT_FILE_POINTER_OFFSET];
        const auto magic = header[FILE_MAGIC_OFFSET];
        const auto file_state_flags = header[FILE_STATE_FLAGS_OFFSET];

        if (magic != FILE_VALID_MAGIC)
        {
            // At this point magic should have been written, so something is wrong
            return result::Result::ERROR_GENERAL;
        }
        const auto file_state = get_file_state(file_state_flags);
        if (file_state == FILE_CLOSED_AFTER_WRITE)
        {
            // Find has been found. 
            is_first_valid_file_found = true;

            file.ram.current_file_start_address = current_file_address;
            file.ram.current_operation_address =
                current_file_address + _spi_flash_configuration.page_size;
            file.ram.size = header[FILE_SIZE_POINTER_OFFSET];
            file.ram.is_write = false;
            file.rom.size = file.ram.size;
            file.rom.next_file_start = header[NEXT_FILE_POINTER_OFFSET];
        }
        else if (file_state != FILE_INVALIDATED)
        {
            // This can happen if a) file was not properly closed b) file was not properly invalidated.
            // Both cases are nothing good.
            return result::Result::ERROR_GENERAL;
        }
        current_file_address = next_address;
    }
    if (iterations_counter == MAX_FILES_COUNT)
    {
        return result::Result::ERROR_GENERAL;
    }

    file.ram.runtime_magic = MAGIC_FILE_OPEN;
    current_file = &file;

    return result::Result::OK;
}

result::Result get_files_count(FilesCount& files_count)
{
    files_count.invalid = 0;
    files_count.valid = 0;

    bool is_last_file_found{false};
    uint32_t current_file_address{0x00};
    uint32_t header[HEADER_SIZE]{0, 0, 0, 0};
    uint32_t iterations_counter{0UL};
    
    while(!is_last_file_found && iterations_counter < MAX_FILES_COUNT)
    {
        ++iterations_counter;
        if(current_file_address > 0x00100000)
        {
            is_last_file_found = true;
            continue;
        }

        read_next_file_header(current_file_address, header);
        const auto next_address = header[NEXT_FILE_POINTER_OFFSET];
        const auto magic = header[FILE_MAGIC_OFFSET];
        const auto file_state_flags = header[FILE_STATE_FLAGS_OFFSET];
        current_file_address = next_address;

        if (magic != FILE_VALID_MAGIC)
        {
            is_last_file_found = true;
            continue;
        }

        const auto file_state = get_file_state(file_state_flags);
        
        if(file_state == FILE_CLOSED_AFTER_WRITE)
        {
            files_count.valid++;
        }
        else if(FILE_INVALIDATED == file_state)
        {
            files_count.invalid++;
        }
        else
        {
            return result::Result::ERROR_GENERAL;
        }
    }
    if (iterations_counter == MAX_FILES_COUNT)
    {
        return result::Result::ERROR_GENERAL;
    }

    return result::Result::OK;
}

size_t get_occupied_memory_size()
{
    size_t result{0};
    bool is_last_file_found{false};
    uint32_t current_file_address{0x00};
    uint32_t header[HEADER_SIZE]{0, 0, 0, 0};
    uint32_t iterations_counter{0UL};
    while(!is_last_file_found && iterations_counter < MAX_FILES_COUNT)
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
        const auto magic = header[FILE_MAGIC_OFFSET];

        const auto file_state = get_file_state(file_state_flags);

        if(magic != FILE_VALID_MAGIC)
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
    if (iterations_counter == MAX_FILES_COUNT)
    {
        return 0xFFFFFFFFUL;
    }

    return result;
}

result::Result write(File& file, uint8_t* const data, const size_t size)
{
    if(!is_file_open() || !file.ram.is_write)
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
    if(!is_file_open() || file.ram.is_write)
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
    if ((!is_file_open()) || 
       (&file != current_file))
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    if (file.ram.is_write)
    {
        auto& config = _spi_flash_configuration;

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
        file.rom.file_state_flags = FILE_CLOSED_AFTER_WRITE;

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
        current_file = nullptr;
        return result::Result::OK;
    }
    else
    {
        // invalidate file variable
        file.ram.runtime_magic = 0;
        current_file = nullptr;
        return result::Result::OK;
    }
}

result::Result invalidate(File& file)
{
    if((file.ram.is_write) || 
       (!is_file_open()) || 
       (&file != current_file))
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    // so the only thing to be done is to invalidate the status flag
    auto& config = _spi_flash_configuration;
    // finalize file write

    uint32_t header[HEADER_SIZE]{0, 0, 0, 0};
    read_next_file_header(file.ram.current_file_start_address, header);
    header[FILE_STATE_FLAGS_OFFSET] = FILE_INVALIDATED;
    constexpr auto FILE_ROM_SIZE = sizeof(File::RomDescriptor);

    const auto res =
        config.write(file.ram.current_file_start_address, reinterpret_cast<uint8_t *>(header), FILE_ROM_SIZE);
    if(res != result::Result::OK)
    {
        return res;
    }
    // invalidate file variable
    file.ram.runtime_magic = 0;
    current_file = nullptr;
    return result::Result::OK;
}

} // namespace filesystem
