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
static constexpr uint32_t first_file_start_location{4096};
static constexpr uint32_t page_size{256};

struct myfs_t
{
    bool is_mounted{false};
    uint32_t files_count{0};
    uint32_t fs_start_address{0};
    uint32_t next_file_start_address{0};
    uint32_t next_file_descriptor_address{0};

    uint8_t * buffer_pointer{nullptr};
    uint32_t buffer_size{page_size};
    uint32_t buffer_position{0};

    bool is_file_open{false};

    uint32_t current_id_search_pos{0};
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


static constexpr uint8_t MYFS_CREATE_FLAG{1<<0};
static constexpr uint8_t MYFS_READ_FLAG{1<<1};

struct myfs_config {
    void *context;
    int (*read)(const struct myfs_config *c, myfs_block_t block, myfs_off_t off, void *buffer, myfs_size_t size);
    int (*prog)(const struct myfs_config *c, myfs_block_t block, myfs_off_t off, const void *buffer, myfs_size_t size);
    int (*erase)(const struct myfs_config *c, myfs_block_t block);
    int (*sync)(const struct myfs_config *c);

    myfs_size_t read_size;
    myfs_size_t prog_size;
    myfs_size_t block_size;
    myfs_size_t block_count;
    int32_t block_cycles;

    myfs_size_t cache_size;
    myfs_size_t lookahead_size;

    void *read_buffer;
    void *prog_buffer;
    void *lookahead_buffer;

    myfs_size_t name_max;
    myfs_size_t file_max;
    myfs_size_t attr_max;
    myfs_size_t metadata_max;
};

int myfs_format(myfs_t *myfs, const myfs_config *config);
int myfs_mount(myfs_t *myfs, const myfs_config *config);

int myfs_file_open(myfs_t *myfs, const myfs_config& config, myfs_file_t& file, uint8_t * file_id, uint8_t flags);
int myfs_file_get_size(myfs_t& myfs, const myfs_config& config, uint8_t * file_id);
int myfs_file_close(myfs_t *myfs, const myfs_config& config, myfs_file_t& file);
int myfs_file_write(myfs_t *myfs, const myfs_config& config, myfs_file_t& file, void *buffer, myfs_size_t size);
int myfs_file_read(myfs_t *myfs, const myfs_config& config, myfs_file_t& file, void *buffer, myfs_size_t max_size, myfs_size_t& read_size);
int myfs_unmount(myfs_t *myfs, const myfs_config& config);

// "dir"-related calls
int myfs_get_files_count(myfs_t& myfs);
int myfs_rewind_dir(myfs_t& myfs);
int myfs_get_next_id(myfs_t& myfs, const myfs_config& config, uint8_t * file_id);

}
