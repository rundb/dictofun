// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "lfs.h"

// LittleFS integration process suggests implementation of methods that are used to access the flash memory.
namespace memory 
{
namespace blockdevice 
{
namespace sim 
{

int read(
    const struct lfs_config *c, 
    lfs_block_t block,
    lfs_off_t off, 
    void *buffer, 
    lfs_size_t size
);

int program(
    const struct lfs_config *c, 
    lfs_block_t block,
    lfs_off_t off, 
    const void *buffer, 
    lfs_size_t size);

int erase(const struct lfs_config *c, lfs_block_t block);
int sync(const struct lfs_config *c);

}
}
}