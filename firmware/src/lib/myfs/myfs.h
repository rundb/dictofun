// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include <cstdint>

using myfs_block_t = uint32_t;
using myfs_off_t = uint32_t;
using myfs_size_t = uint32_t;

// This is an attempt to introduce a filesystem that would resolve issues caused by usage of LFS.
// In particular, it should make sure that record start time is faster than linear relative to files count
// I will do my best to reuse LFS interfaces.
// Assumptions:
// - only one file can be open at a time
// - uniqueness of the file names is not guaranteed
// - file ids are always only 8 bytes
namespace filesystem
{
static constexpr uint32_t global_magic_value{0x2A7B3D1FUL};
static constexpr uint32_t file_magic_value{0xE9C864A7};
static constexpr uint32_t empty_word_value{0xFFFFFFFFUL};
static constexpr uint32_t single_file_descriptor_size_bytes{32};
static constexpr uint32_t myfs_format_marker_size{single_file_descriptor_size_bytes};
static constexpr uint32_t legacy_first_file_start_location{4096};
// TODO: derive this value from the memory size
static constexpr uint32_t first_file_start_location{8192};
static constexpr uint32_t page_size{256};


static constexpr int GENERIC_ERROR{-1};
static constexpr int INVALID_PARAMETERS{-2};
static constexpr int ERROR_FILE_NOT_FOUND{-3};
static constexpr int FS_CORRUPT{-4};
static constexpr int FS_CORRUPT_REPAIRABLE{-5};
static constexpr int NO_SPACE_LEFT{-6};
static constexpr int ALIGNMENT_ERROR{-7};
static constexpr int INTERNAL_ERROR{-8};
static constexpr int IMPLEMENTATION_ERROR{-9};
static constexpr int REMOUNT_ATTEMPTED{-10};
static constexpr int REPAIR_HAS_BEEN_PERFORMED{-16};

struct myfs_config
{
    void* context;
    int (*read)(const struct myfs_config* c,
                myfs_block_t block,
                myfs_off_t off,
                void* buffer,
                myfs_size_t size);
    int (*prog)(const struct myfs_config* c,
                myfs_block_t block,
                myfs_off_t off,
                const void* buffer,
                myfs_size_t size);
    int (*erase)(const struct myfs_config* c, myfs_block_t block);
    int (*erase_multiple)(const struct myfs_config* c, myfs_block_t block, uint32_t blocks_count);
    int (*sync)(const struct myfs_config* c);

    myfs_size_t read_size;
    myfs_size_t prog_size;
    myfs_size_t block_size;
    myfs_size_t block_count;

    void* read_buffer;
    void* prog_buffer;
};

struct myfs_t
{
    bool is_mounted{false};
    uint32_t files_count{0};
    uint32_t fs_start_address{0};
    uint32_t next_file_start_address{0};
    uint32_t next_file_descriptor_address{0};

    uint8_t* buffer_pointer{nullptr};
    uint32_t buffer_size{page_size};
    uint32_t buffer_position{0};

    bool is_file_open{false};
    bool is_full{false};
    bool is_corrupt{false};

    uint32_t current_id_search_pos{single_file_descriptor_size_bytes};

    myfs_config& config;

    myfs_t(myfs_config& cfg)
        : config(cfg)
    { }
};

/// Bytes 0..3: magic, corresponding to a created file
/// Bytes 4..7: start address
/// Bytes 8..15: file identifier/name (0 is a valid value, shall be converted to text `00`)
/// Bytes 16..19: file size (also it's a marker that file has been closed after the write)
/// Bytes 20..23: reserved for CRC
struct __attribute__((__packed__)) myfs_file_descriptor
{
    static constexpr uint32_t file_id_size{8};
    uint32_t magic;
    uint32_t start_address;
    uint8_t file_id[file_id_size];
    uint32_t file_size;
    uint8_t reserved[12];

    void size_assertion()  { static_assert(single_file_descriptor_size_bytes == sizeof(myfs_file_descriptor)); }
};

struct myfs_file_t
{
    static constexpr uint8_t id_size{myfs_file_descriptor::file_id_size};
    uint8_t flags;
    uint8_t id[id_size];
    uint32_t size;
    uint32_t read_pos;
    uint32_t start_address;
    bool is_open{false};
    bool is_write{false};
};

static constexpr uint8_t MYFS_CREATE_FLAG{1 << 0};
static constexpr uint8_t MYFS_READ_FLAG{1 << 1};

int myfs_format(myfs_t& myfs);
int myfs_mount(myfs_t& myfs);

int myfs_file_open(myfs_t& myfs, myfs_file_t& file, uint8_t* file_id, uint8_t flags);
int myfs_file_get_size(myfs_t& myfs, uint8_t* file_id);
int myfs_file_close(myfs_t& myfs, myfs_file_t& file);
int myfs_file_write(myfs_t& myfs, myfs_file_t& file, void* buffer, myfs_size_t size);
int myfs_file_read(
    myfs_t& myfs, myfs_file_t& file, void* buffer, myfs_size_t max_size, myfs_size_t& read_size);
int myfs_unmount(myfs_t& myfs);

int myfs_repair(myfs_t& myfs, myfs_file_descriptor& first_invalid_descriptor, uint32_t descriptor_address);

// "dir"-related calls
uint32_t myfs_get_files_count(myfs_t& myfs);
int myfs_rewind_dir(myfs_t& myfs);
int myfs_get_next_id(myfs_t& myfs, uint8_t* file_id);

// "stat"-related call
int myfs_get_fs_stat(myfs_t& myfs, uint32_t& files_count, uint32_t& occupied_space);
} // namespace filesystem
