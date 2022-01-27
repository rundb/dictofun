// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2021, Roman Turkin
 */

#pragma once
#include "lfs.h"

int spi_flash_block_device_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);
int spi_flash_block_device_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);
int spi_flash_block_device_erase(const struct lfs_config *c, lfs_block_t block);
int spi_flash_block_device_sync(const struct lfs_config *c);

extern const struct lfs_config lfs_configuration;
