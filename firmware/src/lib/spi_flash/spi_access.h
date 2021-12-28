#pragma once

/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  //#ifdef __cplusplus

void spi_access_init();

void spi_access_txrx(uint8_t * tx_data, uint8_t * rx_data, uint32_t size);

void spi_flash_cyclic();

void spi_flash_reset();
int spi_flash_is_present();
int spi_flash_get_sr_1();

int spi_flash_erase_chip();
int spi_flash_erase_area(uint32_t address, uint32_t start);
int spi_flash_trigger_read(uint32_t address, uint32_t len);
int spi_flash_write(uint32_t address, uint32_t len, uint8_t * src);
void spi_flash_write_enable(int new_state);

int spi_flash_is_busy();
void spi_flash_copy_received_data(uint8_t * dst, uint32_t len);
int spi_flash_is_spi_bus_busy();

#ifdef __cplusplus
}
#endif //#ifdef __cplusplus
