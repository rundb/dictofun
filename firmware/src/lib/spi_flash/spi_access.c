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

#include "spi_access.h"
#include "nrf_drv_spi.h"
#include "nrf_spi_mngr.h"
#include "nrf_gpio.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "boards/boards.h"

static volatile spi_xfer_complete = true;
static const nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(0);
uint8_t tmp_rx_buf[256];
uint8_t tmp_tx_buf[256];

void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_complete = true;
}

void spi_flash_cyclic()
{
    
}

void spi_access_init()
{
    nrf_drv_spi_config_t const spi_config =
    {
        .sck_pin        = SPI_FLASH_SCK_PIN,
        .mosi_pin       = SPI_FLASH_MOSI_PIN,
        .miso_pin       = SPI_FLASH_MISO_PIN,
        .ss_pin         = SPI_FLASH_CS_PIN,
        .irq_priority   = 3,
        .orc            = 0x00,
        .frequency      = NRF_DRV_SPI_FREQ_1M,
        .mode           = NRF_DRV_SPI_MODE_3,
        .bit_order      = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
    };
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi_instance, &spi_config, spi_event_handler, NULL));
    nrf_gpio_cfg_output(SPI_FLASH_RST_PIN);
    nrf_gpio_cfg_output(SPI_FLASH_WP_PIN);

    // Set pins high, they are active-low
    nrf_gpio_pin_set(SPI_FLASH_RST_PIN);
    nrf_gpio_pin_set(SPI_FLASH_WP_PIN);

    spi_flash_reset();
    if (!spi_flash_is_present())
    {
        NRF_LOG_ERROR("SPI flash is not present");
    }

    spi_flash_trigger_read(0x00, 8);

    while (spi_flash_is_spi_bus_busy()) ;
    uint8_t tmp[8];
    spi_flash_copy_received_data(tmp, 8);
    if (tmp[0] == 0xFF && tmp[1] == 0xFF && tmp[2] == 0xFF && tmp[3] == 0xFF)
    {
        NRF_LOG_INFO("SPI flash is ready");
    }
    else
    {
        NRF_LOG_ERROR("SPI flash is not erased");
        // !!!!!!!! dangerous code.
        // it failed before but probably works because
        //  mic prepare delay adds 0.2s before first write access

        int rc = spi_flash_erase_area(0x00, 0xB0000);
        if (rc != 0)
        {
            NRF_LOG_INFO("erase failed");
        }
        while (spi_flash_is_busy());
        NRF_LOG_INFO("SPI flash has been erased");
    }
}

void spi_access_txrx(uint8_t * tx_data, uint8_t * rx_data, uint32_t size)
{
      APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi_instance, tx_data, size, rx_data, size));
}


static volatile uint32_t JEDEC_ID = 0;
int spi_flash_is_present()
{
    // read JEDEC ID.
    spi_xfer_complete = false;
    uint8_t tx_data[] = {0x9F, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t rx_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    nrf_drv_spi_transfer(&spi_instance, tx_data, 1, rx_data, 6);
    while (!spi_xfer_complete);
    JEDEC_ID = rx_data[1] | (rx_data[2] << 8) | (rx_data[3] << 16) | (rx_data[4] << 24);
    return rx_data[2] != 0 && rx_data[2] != 0xFF;
}

void spi_flash_reset()
{
    spi_xfer_complete = false;
    uint8_t tx_data[] = {0x99};
    uint8_t rx_data[] = {0xcc};
    nrf_drv_spi_transfer(&spi_instance, tx_data, 1, rx_data, 0);

    while (!spi_xfer_complete);

    for (volatile int i = 0; i < 1000000; ++i);  
}

int spi_flash_get_sr_1()
{
    spi_xfer_complete = false;
    uint8_t tx_data[] = {0x05};
    uint8_t rx_data[] = {0xcc, 0x00};
    //spi_access_txrx(tx_data, rx_data, 10);
    nrf_drv_spi_transfer(&spi_instance, tx_data, 1, rx_data, 2);
    while (!spi_xfer_complete);

    return rx_data[1];
}

int spi_flash_is_busy()
{
    return spi_flash_get_sr_1() & 0x01;
}

void spi_flash_write_enable(int new_state)
{
    spi_xfer_complete = false;
    uint8_t tx_data[] = {0x06};
    uint8_t rx_data[] = {0xcc};
    if (new_state == 0)
    { 
        // disable
      tx_data[0] = 0x04;
    }
    nrf_drv_spi_transfer(&spi_instance, tx_data, 1, rx_data, 0);
    while (!spi_xfer_complete);
}

int spi_flash_erase_chip()
{
    spi_flash_write_enable(1);
    spi_xfer_complete = false;
    uint8_t tx_data[] = {0xc7};
    uint8_t rx_data[] = {0};
    nrf_drv_spi_transfer(&spi_instance, tx_data, 1, rx_data, 0);

    while (!spi_xfer_complete);
}

int spi_flash_erase_area(uint32_t address, uint32_t size)
{
    const uint32_t ERASABLE_SIZE = 0x10000;
    if (address % ERASABLE_SIZE != 0 || size % ERASABLE_SIZE != 0)
    {
        return 1;
    }
    for (int i = 0; i < size / ERASABLE_SIZE; ++i)
    {
      spi_flash_write_enable(1);
      spi_xfer_complete = false;
      uint32_t addr = address + i * ERASABLE_SIZE;
      uint8_t tx_data[] = {0xD8,(addr >> 16) & 0xFF, (addr >> 8) & 0xFF, (addr)& 0xFF};
      uint8_t rx_data[] = {0};
      nrf_drv_spi_transfer(&spi_instance, tx_data, 4, rx_data, 0);
      while (!spi_xfer_complete);
      while (spi_flash_is_busy());
    }
    return 0;
}


int spi_flash_trigger_read(uint32_t address, uint32_t len)
{
    spi_xfer_complete = false;
    tmp_tx_buf[0] = 0x3U;
    tmp_tx_buf[1] = (address >> 16) & 0xFF;
    tmp_tx_buf[2] = (address >> 8) & 0xFF;
    tmp_tx_buf[3] = (address) & 0xFF;

    return nrf_drv_spi_transfer(&spi_instance, tmp_tx_buf, 4, tmp_rx_buf, len + 4);
}

int spi_flash_write(uint32_t address, uint32_t len, uint8_t * src)
{
    if (!spi_xfer_complete || spi_flash_is_busy())
    { 
        return 1;
    }
    spi_flash_write_enable(1);
    spi_xfer_complete = false;
    tmp_tx_buf[0] = 0x2U;
    tmp_tx_buf[1] = (address >> 16) & 0xFF;
    tmp_tx_buf[2] = (address >> 8) & 0xFF;
    tmp_tx_buf[3] = (address) & 0xFF;
    memcpy(&tmp_tx_buf[4], src, len);

    return nrf_drv_spi_transfer(&spi_instance, tmp_tx_buf, len + 4, tmp_rx_buf, 0);
}

int spi_flash_is_spi_bus_busy()
{
    return !spi_xfer_complete;
}

void spi_flash_copy_received_data(uint8_t * dst, uint32_t len)
{
    memcpy(dst, &tmp_rx_buf[4], len);
}

// Usage example:
  //bool is_spi_flash_present = spi_flash_is_present();
  //bool is_busy = false;
  //uint8_t rx_page[256] = {0};
  //if (is_spi_flash_present)
  //{
  //  spi_flash_write_enable(1);
  //  spi_flash_erase_chip();
    
  //  int busy_cycles_count = 0;
  //  do
  //  {
  //      is_busy = spi_flash_is_busy();
  //      busy_cycles_count++;
  //  } while (is_busy);

  //  spi_flash_write_enable(1);
  //  uint8_t tx_buffer[256];
  //  for (int i = 0; i < 256; ++i)
  //  {
  //    tx_buffer[i] = i;
  //  }
  //  auto res1 = spi_flash_write(0x00, 250, tx_buffer);
  //  NRFX_ASSERT(res1 == NRFX_SUCCESS);

  //  int spi_wait_count = 0;
  //  while (spi_flash_is_spi_bus_busy()) ++spi_wait_count;

  //  auto res2 = spi_flash_trigger_read(0x00, 255);
  //  NRFX_ASSERT(res2 == NRFX_SUCCESS);
  //  spi_wait_count = 0;
  //  while (spi_flash_is_spi_bus_busy()) ++spi_wait_count;

  //  spi_flash_copy_received_data(rx_page, 255);
  //  ++spi_wait_count;

  //}