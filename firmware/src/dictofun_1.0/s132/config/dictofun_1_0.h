#ifndef _DICTOFUN_1_0_H_
#define _DICTOFUN_1_0_H_

#define BSP_BUTTON_0       25
//#define BSP_BOARD_LED_0    13
#define LED_1              13
//#define BSP_BOARD_LED_1    14
#define LED_2              14
//#define BSP_BOARD_LED_2    15
#define LED_3              15

#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define LEDS_NUMBER 3
#define BUTTONS_NUMBER 1

//#define LEDS_LIST { BSP_BOARD_LED_0, BSP_BOARD_LED_1, BSP_BOARD_LED_2 }
#define LEDS_LIST { LED_1, LED_2, LED_3 }
#define BUTTONS_LIST { BSP_BUTTON_0 }

#define LEDS_ACTIVE_STATE 0
#define LEDS_INV_MASK  LEDS_MASK

#define BUTTONS_ACTIVE_STATE 0

#endif //_DICTOFUN_1_0_H_