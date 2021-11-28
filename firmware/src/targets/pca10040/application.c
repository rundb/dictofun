#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "boards.h"
#include "app_timer.h"
#include "app_button.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "spi_access.h"
#include "drv_audio.h"

#include "BleSystem.h"

#define ADVERTISING_LED                 BSP_BOARD_LED_0                         /**< Is on when device is advertising. */
#define CONNECTED_LED                   BSP_BOARD_LED_1                         /**< Is on when device has connected. */
#define LEDBUTTON_LED                   BSP_BOARD_LED_2                         /**< LED to be toggled with the help of the LED Button Service. */
#define LEDBUTTON_BUTTON                BSP_BUTTON_0                            /**< Button that will trigger the notification event with the LED Button Service */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                     /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define LDO_EN_PIN  11
#define BUTTON_PIN  25

enum APP_SM_STATES
{
  APP_SM_REC_INIT = 10,
  APP_SM_RECORDING,
  APP_SM_CONN,
  APP_SM_XFER,

  APP_SM_FINALISE = 50,
  APP_SM_SHUTDOWN,

};

#define SPI_XFER_DATA_STEP 0x100
static enum APP_SM_STATES app_sm_state = APP_SM_REC_INIT;
drv_audio_frame_t * pending_frame = NULL;
size_t write_pointer = 0;

static void leds_init(void)
{
    bsp_board_init(BSP_INIT_LEDS);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
APP_TIMER_DEF(timestamp_timer);
void timestamp_timer_timeout_handler(void * p_context) {}
static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    app_timer_create(&timestamp_timer, APP_TIMER_MODE_REPEATED, timestamp_timer_timeout_handler);
    err_code = app_timer_start(&timestamp_timer, 32768, NULL);
    APP_ERROR_CHECK(err_code);
}

// static void led_write_handler(uint16_t conn_handle, ble_lbs_t * p_lbs, uint8_t led_state)
// {
//     if (led_state)
//     {
//         bsp_board_led_on(LEDBUTTON_LED);
//         NRF_LOG_INFO("Received LED ON!");
//     }
//     else
//     {
//         bsp_board_led_off(LEDBUTTON_LED);
//         NRF_LOG_INFO("Received LED OFF!");
//     }
// }
static bool rec_button_released()
{
 // auto tmp = nrf_gpio_pin_input_get(BUTTON_PIN);
  return 0;//(0 == tmp);
}

static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

static void audio_frame_handle()
{
  if(pending_frame) 
  {
    const int data_size = pending_frame->buffer_occupied;
    NRFX_ASSERT(data_size < SPI_XFER_DATA_STEP);
    NRFX_ASSERT(app_sm_state == APP_SM_RECORDING);
      //  nrf_gpio_pin_set(17);
    spi_flash_write(write_pointer, data_size, pending_frame->buffer);
       //     nrf_gpio_pin_clear(17);
    write_pointer += SPI_XFER_DATA_STEP;

    pending_frame->buffer_free_flag = true;
    pending_frame->buffer_occupied = 0;
    pending_frame = NULL;
  }
}
static void audio_frame_cb(drv_audio_frame_t * frame)
{
  NRFX_ASSERT(NULL == pending_frame);
  pending_frame = frame;
}

static uint32_t read_pointer = 0;
#define SPI_READ_SIZE 128

void application_init()
{
    // Initialize.
    leds_init();
    timers_init();
    power_management_init();

}

void application_cyclic()
{
}

