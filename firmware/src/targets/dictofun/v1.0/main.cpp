#include "BleSystem.h"
#include <boards/boards.h>
#include <libraries/log/nrf_log_ctrl.h>
#include <libraries/log/nrf_log.h>
#include <libraries/log/nrf_log_default_backends.h>
#include <spi_access.h>
#include <nrf_gpio.h>

#include <tasks/task_audio.h>
#include <tasks/task_state.h>
#include <tasks/task_led.h>

static void log_init();
static void idle_state_handle();
static void timers_init();

ble::BleSystem bleSystem{};

int main()
{
    nrf_gpio_cfg_output(LDO_EN_PIN);
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_pin_set(LDO_EN_PIN);

    log_init();
    bsp_board_init(BSP_INIT_LEDS);
    timers_init();
    spi_access_init();
    bleSystem.init();
    audio_init();

    led::task_led_init();
    
    for (;;)
    {
        bleSystem.cyclic();
        audio_frame_handle();
        spi_flash_cyclic();
        application::application_cyclic();
        led::task_led_cyclic();
        idle_state_handle();
    }
}

// Application helper functions

uint32_t get_timestamp()
{
    return app_timer_cnt_get();
}

static void log_init()
{
    ret_code_t err_code = NRF_LOG_INIT(get_timestamp);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void idle_state_handle()
{
    NRF_LOG_PROCESS();
}

uint32_t get_timestamp_delta(uint32_t base)
{
  uint32_t now = app_timer_cnt_get();

  if(base > now)
    return 0;
  else
    return  now - base;
}

// Expanding APP_TIMER_DEF macro, as it's not compatible with C++
NRF_LOG_INSTANCE_REGISTER(APP_TIMER_LOG_NAME, timestamp_timer,
                            APP_TIMER_CONFIG_INFO_COLOR,
                            APP_TIMER_CONFIG_DEBUG_COLOR,
                            APP_TIMER_CONFIG_INITIAL_LOG_LEVEL,
                            APP_TIMER_CONFIG_LOG_ENABLED ?                                  
                                        APP_TIMER_CONFIG_LOG_LEVEL : NRF_LOG_SEVERITY_NONE);

static app_timer_t timestamp_timer_data = {
        .active = false,
        NRF_LOG_INSTANCE_PTR_INIT(p_log, APP_TIMER_LOG_NAME, timer_id)
};
static app_timer_id_t timestamp_timer;// = &timestamp_timer_data;

void timestamp_timer_timeout_handler(void * p_context) {}
static void timers_init()
{
    timestamp_timer = &timestamp_timer_data;
    // Initialize timer module, making it use the scheduler
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    app_timer_create(&timestamp_timer, APP_TIMER_MODE_REPEATED, timestamp_timer_timeout_handler);
    err_code = app_timer_start(timestamp_timer, 32768, NULL);
    APP_ERROR_CHECK(err_code);
}
