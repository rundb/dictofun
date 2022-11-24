// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "app_error.h"

#include "nrf_sdm.h"
#include "BleSystem.h"
#include "ble_dfu.h"
#include "spi.h"
#include "spi_flash.h"
#include <boards/boards.h>
#include <libraries/log/nrf_log.h>
#include <libraries/log/nrf_log_ctrl.h>
#include <libraries/log/nrf_log_default_backends.h>
#include <nrf_gpio.h>
#include "simple_fs.h"
#include "block_device_api.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_wdt.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <tasks/task_audio.h>
#include <tasks/task_led.h>
#include <tasks/task_state.h>

static void log_init();
static void idle_state_handle();
static void timers_init();

ble::BleSystem bleSystem{};
spi::Spi flashSpi(0, SPI_FLASH_CS_PIN);
flash::SpiFlash flashMemory(flashSpi);
flash::SpiFlash& getSpiFlash() { return flashMemory;}

nrf_drv_wdt_config_t wdt_config = 
{
    .behaviour          = (nrf_wdt_behaviour_t)NRFX_WDT_CONFIG_BEHAVIOUR, \
    .reload_value       = NRFX_WDT_CONFIG_RELOAD_VALUE,                   \
    NRFX_WDT_IRQ_CONFIG
};
nrf_drv_wdt_channel_id m_channel_id;
void init_watchdog();
void start_watchdog();
void feed_watchdog();

static const spi::Spi::Configuration flash_spi_config{NRF_DRV_SPI_FREQ_2M,
                                                      NRF_DRV_SPI_MODE_0,
                                                      NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
                                                      SPI_FLASH_SCK_PIN,
                                                      SPI_FLASH_MOSI_PIN,
                                                      SPI_FLASH_MISO_PIN};
int main()
{
    nrf_gpio_cfg_output(LDO_EN_PIN);
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_pin_set(LDO_EN_PIN);

    nrf_gpio_cfg(LDO_EN_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_DISCONNECT,
                 NRF_GPIO_PIN_PULLDOWN,
                 NRF_GPIO_PIN_H0S1,
                 NRF_GPIO_PIN_NOSENSE);

    const auto err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);

    log_init();

    // TODO: uncomment if bootloader is present
    // const auto err_code = ble_dfu_buttonless_async_svci_init();
    // APP_ERROR_CHECK(err_code);

    bsp_board_init(BSP_INIT_LEDS);
    timers_init();
    flashSpi.init(flash_spi_config);
    flashMemory.init();
    flashMemory.reset();
    integration::init_filesystem(&flashMemory);

    bleSystem.init();

    audio_init();
    application::application_init();
    led::task_led_init();

    //init_watchdog();
    //start_watchdog();

    for(;;)
    {
        bleSystem.cyclic();
        audio_frame_handle();
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
    //feed_watchdog();
}

uint32_t get_timestamp_delta(uint32_t base)
{
    uint32_t now = app_timer_cnt_get();

    if(base > now)
        return 0;
    else
        return now - base;
}

// Expanding APP_TIMER_DEF macro, as it's not compatible with C++
NRF_LOG_INSTANCE_REGISTER(APP_TIMER_LOG_NAME,
                          timestamp_timer,
                          APP_TIMER_CONFIG_INFO_COLOR,
                          APP_TIMER_CONFIG_DEBUG_COLOR,
                          APP_TIMER_CONFIG_INITIAL_LOG_LEVEL,
                          APP_TIMER_CONFIG_LOG_ENABLED ? APP_TIMER_CONFIG_LOG_LEVEL
                                                       : NRF_LOG_SEVERITY_NONE);

static app_timer_t timestamp_timer_data = {
    NRF_LOG_INSTANCE_PTR_INIT(p_log, APP_TIMER_LOG_NAME, timer_id)};
static app_timer_id_t timestamp_timer;

void timestamp_timer_timeout_handler(void* p_context) { }
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

void wdt_event_handler(void)
{
    //NOTE: The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs

    // TODO: pull LDO_EN low
}

void init_watchdog()
{
    auto err_code = nrf_drv_wdt_init(&wdt_config, wdt_event_handler);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_wdt_channel_alloc(&m_channel_id);
    APP_ERROR_CHECK(err_code);
}

void start_watchdog()
{
    nrf_drv_wdt_enable();
}

void feed_watchdog()
{
    nrf_drv_wdt_channel_feed(m_channel_id);
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    __disable_irq();
    NRF_LOG_FINAL_FLUSH();

#ifndef DEBUG
    NRF_LOG_ERROR("Fatal error");
#else
    switch (id)
    {
#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT
        case NRF_FAULT_ID_SD_ASSERT:
            NRF_LOG_ERROR("SOFTDEVICE: ASSERTION FAILED");
            break;
        case NRF_FAULT_ID_APP_MEMACC:
            NRF_LOG_ERROR("SOFTDEVICE: INVALID MEMORY ACCESS");
            break;
#endif
        case NRF_FAULT_ID_SDK_ASSERT:
        {
            assert_info_t * p_info = (assert_info_t *)info;
            NRF_LOG_ERROR("ASSERTION FAILED at %s:%u",
                          p_info->p_file_name,
                          p_info->line_num);
            break;
        }
        case NRF_FAULT_ID_SDK_ERROR:
        {
            error_info_t * p_info = (error_info_t *)info;
            NRF_LOG_ERROR("ERROR %u [%s] at %s:%u\r\nPC at: 0x%08x",
                          p_info->err_code,
                          nrf_strerror_get(p_info->err_code),
                          p_info->p_file_name,
                          p_info->line_num,
                          pc);
             NRF_LOG_ERROR("End of error report");
            break;
        }
        default:
            NRF_LOG_ERROR("UNKNOWN FAULT at 0x%08X", pc);
            break;
    }
#endif

    NRF_BREAKPOINT_COND;
    // On assert, the system can only recover with a reset.

    nrf_gpio_pin_clear(LDO_EN_PIN);
}