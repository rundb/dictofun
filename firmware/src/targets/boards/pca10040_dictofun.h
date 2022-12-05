#ifndef _DICTOFUN_PCA10040_H_
#define _DICTOFUN_PCA10040_H_

#include <nrf_spi.h>

#define BSP_BUTTON_0       25
#define LED_1              13    // BLUE
#define LED_2              14 // GREEN
#define LED_3              15 // RED

#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define LEDS_NUMBER 3
#define BUTTONS_NUMBER 1

#define LEDS_LIST { LED_1, LED_2, LED_3 }
#define BUTTONS_LIST { BSP_BUTTON_0 }

#define LEDS_ACTIVE_STATE 0
#define LEDS_INV_MASK  LEDS_MASK

#define BUTTONS_ACTIVE_STATE 0

#define LDO_EN_PIN NRF_SPI_PIN_NOT_CONNECTED
#define BUTTON_PIN NRF_SPI_PIN_NOT_CONNECTED

#define BOARD_NAME "dictofun_pca10040"

#define SPI_FLASH_MISO_PIN 12
#define SPI_FLASH_WP_PIN   10
#define SPI_FLASH_CS_PIN   8
#define SPI_FLASH_MOSI_PIN 5
#define SPI_FLASH_SCK_PIN  7
#define SPI_FLASH_RST_PIN  6

#define CONFIG_IO_PDM_CLK           NRF_SPI_PIN_NOT_CONNECTED
#define CONFIG_IO_PDM_DATA          NRF_SPI_PIN_NOT_CONNECTED

#define LOG_UART_PIN  11

#endif //_DICTOFUN_PCA10040_H_