// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

/**
 * This file represents common interface for filesystem access required by dictofun.
 * It is targeted to be implemented over an SPI Flash memory, and has a few specific
 * requirements.
 * 
 * Requirements are the following. FS should:
 * - be implemented over relatively small SPI NOR Flash chips (up to 16 Mb);
 * - be able to read and write limited amount of files (<= 20);
 * - support exactly one open file at a time;
 * - support fast file opening for write (under 100 ms);
 * - support fast write operations (< 8ms per page, so erase should not happen during the write)
 * 
 * - file closing, index updates, erase operations can be slow;
 * - possibly support redundancy and wear levelling (f.e. ECC for headers);
 * 
 * FS should not (or doesn't have to):
 * - support directories
 * - support editing the files
 */

#include "result.h"
#include "stdint.h"
#include <cstddef>

namespace filesystem
{
/**
 * Forward declaration of a structure representing the file system. 
 */
struct Filesystem;

/**
 * Every file in Flash memory should start from a single page with following structure:
 * - bits 0-31: pointer to the next file in the FS. It is not equal the size of the file, see the \notes
 * - bits 32-63: size of the data inside the file
 * - bits 64-95: FS-specific magic value
 * - bits 96-127: flags that define status of the file. File can have 3 valid states:
 *               -- not written yet, should be 0xFFFFFFFF
 *               -- written, but not ready to be deleted yet: should be 0xDEFF_<CHKS>
 *               -- written, can be deleted: should be 0xDEAD_<CHKS>
 *               <CHKS> here is a redundancy/correction code for first 12 bytes of the file
 * - bytes 33-256: stay empty (== 0xFF)
 *
 * \notes Data is stored in chunks proportional to page size. When file is being closed, we want to assure
 * that we can safely and easily remove that file without buffering it to the RAM. So the pointer to the next 
 * file has to point to a start of a sector.
 */
struct File
{

    // these fields shall be written into header of the file
    struct RomDescriptor
    {
        // start of the file. Data starts from the next page (so with an offset of 256)
        uint32_t next_file_start;
        // size of the data written in the file ()
        uint32_t size;
        // FS-specific magic (defined in the implementation)
        uint32_t magic;
        // Corresponds to state of the file on ROM + contains CRC/ECC of first 12 bytes
        uint32_t file_state_flags;
    };

    RomDescriptor rom;

    // These fields are relevant only when the file is open
    struct RamDescriptor
    {
        // these fields are used during the file operations
        bool is_write;
        // this field is used to detect uninitialized file struct usage
        uint32_t runtime_magic;
        uint32_t current_file_start_address;
        uint32_t current_operation_address;
        // this variable grows in case of write and decreases in case of read operation
        uint32_t size;
    };

    RamDescriptor ram;
};

enum class FileMode
{
    RDONLY,
    WRONLY
};

// todo: define signatures for methods necessary for SPI Flash memory
using ReadFunction = result::Result (*)(const uint32_t address, uint8_t* data, const size_t size);
using WriteFunction = result::Result (*)(const uint32_t address,
                                         const uint8_t* const data,
                                         const size_t size);
using EraseFunction = result::Result (*)(const uint32_t address, const size_t size);
struct SpiFlashConfiguration
{
    ReadFunction read;
    WriteFunction write;
    EraseFunction erase;
    // page_size - minimal programmable block
    size_t page_size;
    // sector_size - minimal erasable block
    size_t sector_size;
};

/**
 * These methods don't belong to any of the classes.
 * a) this way we can hide implementation completely in the .cpp file
 * b) this way it's closer to existing FS implementations 
 */
result::Result init(const SpiFlashConfiguration& spiFlashConfiguration);

/**
 * \param flags - flags from POSIX FS (basically read and write)
 */
result::Result open(File& file, FileMode mode);

/**
 * 
 */
result::Result close(File& file);

/**
 * Write <size> bytes to the end of the file <file> open for write
 */
result::Result write(File& file, uint8_t* const data, const size_t size);

/**
 * Attempt to read <size> bytes from the file. Write data to <data> memory and read the actual size 
 *  that has been read into variable read_size
 */
result::Result read(File& file, uint8_t* data, const size_t size, size_t& read_size);

struct FilesCount
{
    size_t valid;
    size_t invalid;
};
/**
 * Iterate through the file system and count valid and invalid files
 */
FilesCount get_files_count();

/**
 * Calculate how much memory is taken
 */
size_t get_occupied_memory_size();

} // namespace filesystem