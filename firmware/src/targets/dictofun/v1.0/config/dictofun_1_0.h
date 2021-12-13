#ifndef _DICTOFUN_1_0_H_
#define _DICTOFUN_1_0_H_

#define BSP_BUTTON_0       25
#define LED_1              13 // BLUE
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

#define LDO_EN_PIN 11
#define BUTTON_PIN 25

#endif //_DICTOFUN_1_0_H_