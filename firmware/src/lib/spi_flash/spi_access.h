#ifndef _SPI_ACCESS_H_
#define _SPI_ACCESS_H_

#include <stdint.h>
#include "spiflash.h"

#ifdef __cplusplus
extern "C" {
#endif  //#ifdef __cplusplus

void spi_access_init();

void spi_access_txrx(uint8_t * tx_data, uint8_t * rx_data, uint32_t size);

void spi_flash_cyclic();

extern spiflash_hal_t spi_flash_hal;

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


#endif //_SPI_ACCESS_H_