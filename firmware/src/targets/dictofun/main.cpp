// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "app_error.h"

#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include <boards.h>
#include <nrf_gpio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

// TODO: this dependency here is really bad
#include "codec_decimate.h"
#include "microphone_pdm.h"

#include "task_audio.h"
#include "task_audio_tester.h"
#include "task_battery.h"
#include "task_ble.h"
#include "task_button.h"
#include "task_cli_logger.h"
#include "task_led.h"
#include "task_memory.h"
#include "task_rtc.h"
#include "task_state.h"
#include "task_wdt.h"

#include <stdint.h>

#include "freertos_wrappers.hpp"

// clang-format off
// ============================= Tasks ======================================

application::TaskDescriptor<256,  3> audio_task;
application::TaskDescriptor<256,  1> audio_tester_task;
application::TaskDescriptor<330,  2> log_task;
application::TaskDescriptor<256,  3> systemstate_task;
application::TaskDescriptor<1500, 2> memory_task;
application::TaskDescriptor<1250, 2> ble_task;
application::TaskDescriptor<96,   1>  led_task;
application::TaskDescriptor<128,  2>  rtc_task;
application::TaskDescriptor<96,   3>  button_task;
application::TaskDescriptor<256,  2>  battery_task;
application::TaskDescriptor<96,   1>  wdt_task;

// ============================= Queues =====================================

application::QueueDescriptor<logger::CliCommandQueueElement, 1>      cli_commands_queue;
application::QueueDescriptor<logger::CliStatusQueueElement, 1>       cli_status_queue; // This thing is under a big doubt, I don't think it's needed
application::QueueDescriptor<audio::CommandQueueElement, 1>          audio_commands_queue;
application::QueueDescriptor<audio::StatusQueueElement, 1>           audio_status_queue;
application::QueueDescriptor<audio::tester::ControlQueueElement, 1>  audio_tester_commands_queue;

application::QueueDescriptor<audio::CodecOutputType, 4>              audio_data_queue;

application::QueueDescriptor<memory::CommandQueueElement, 1>         memory_commands_queue;
application::QueueDescriptor<memory::StatusQueueElement, 1>          memory_status_queue; 

application::QueueDescriptor<ble::CommandQueueElement, 2>            ble_commands_queue;
application::QueueDescriptor<ble::RequestQueueElement, 1>            ble_requests_queue;
application::QueueDescriptor<ble::KeepaliveQueueElement, 1>          ble_keepalive_queue;
application::QueueDescriptor<ble::BatteryDataElement, 1>             ble_batt_info_queue;

application::QueueDescriptor<ble::CommandToMemoryQueueElement, 1>    ble_to_mem_commands_queue;
application::QueueDescriptor<ble::StatusFromMemoryQueueElement, 1>   ble_from_mem_status_queue;
application::QueueDescriptor<ble::FileDataFromMemoryQueueElement, 1> ble_from_mem_data_queue;

application::QueueDescriptor<led::CommandQueueElement, 3>            led_commands_queue;

application::QueueDescriptor<rtc::CommandQueueElement, 1>            rtc_commands_queue;
application::QueueDescriptor<rtc::ResponseQueueElement, 1>           rtc_response_queue;

application::QueueDescriptor<button::EventQueueElement, 1>           button_event_queue;
application::QueueDescriptor<button::RequestQueueElement, 1>         button_requests_queue;
application::QueueDescriptor<button::ResponseQueueElement, 1>        button_response_queue;

application::QueueDescriptor<battery::MeasurementsQueueElement, 1>   battery_measurements_queue;

// ============================= Timers =====================================

StaticTimer_t record_timer_buffer;
TimerHandle_t record_timer_handle{nullptr};

// ============================ Contexts ====================================
logger::CliContext      cli_context;
systemstate::Context    systemstate_context;
audio::Context          audio_context;
audio::tester::Context  audio_tester_context;
memory::Context         memory_context;
ble::Context            ble_context;
led::Context            led_context;
rtc::Context            rtc_context;
button::Context         button_context;
battery::Context        battery_context;

// clang-format on

void latch_ldo_enable()
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
}

int main()
{
    latch_ldo_enable();

    const auto err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);

    logger::log_init();

    bsp_board_init(BSP_INIT_LEDS);

    // Queues' initialization
    const auto cli_commands_init_result = cli_commands_queue.init();
    if(result::Result::OK != cli_commands_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto cli_status_init_result = cli_status_queue.init();
    if(result::Result::OK != cli_status_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto audio_commands_init_result = audio_commands_queue.init();
    if(result::Result::OK != audio_commands_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto audio_data_init_result = audio_data_queue.init();
    if(result::Result::OK != audio_data_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto audio_status_init_result = audio_status_queue.init();
    if(result::Result::OK != audio_status_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto audio_tester_command_init_result = audio_tester_commands_queue.init();
    if(result::Result::OK != audio_tester_command_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto memory_commands_queue_init_result = memory_commands_queue.init();
    if(result::Result::OK != memory_commands_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto memory_status_queue_init_result = memory_status_queue.init();
    if(result::Result::OK != memory_status_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto ble_commands_queue_init_result = ble_commands_queue.init();
    if(result::Result::OK != ble_commands_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto ble_requests_queue_init_result = ble_requests_queue.init();
    if(result::Result::OK != ble_requests_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto ble_keepalive_queue_init_result = ble_keepalive_queue.init();
    if(result::Result::OK != ble_keepalive_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto ble_batt_info_queue_init_result = ble_batt_info_queue.init();
    if(result::Result::OK != ble_batt_info_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto ble_to_mem_commands_queue_init_result = ble_to_mem_commands_queue.init();
    if(result::Result::OK != ble_to_mem_commands_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto ble_from_mem_status_queue_init_result = ble_from_mem_status_queue.init();
    if(result::Result::OK != ble_from_mem_status_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto ble_from_mem_data_queue_init_result = ble_from_mem_data_queue.init();
    if(result::Result::OK != ble_from_mem_data_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto led_commands_queue_init_result = led_commands_queue.init();
    if(result::Result::OK != led_commands_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto rtc_commands_queue_init_result = rtc_commands_queue.init();
    if(result::Result::OK != rtc_commands_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto rtc_response_queue_init_result = rtc_response_queue.init();
    if(result::Result::OK != rtc_response_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto button_event_queue_init_result = button_event_queue.init();
    if(result::Result::OK != button_event_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto button_requests_queue_init_result = button_requests_queue.init();
    if(result::Result::OK != button_requests_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto button_response_queue_init_result = button_response_queue.init();
    if(result::Result::OK != button_response_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto battery_measurements_queue_init_result = battery_measurements_queue.init();
    if(result::Result::OK != battery_measurements_queue_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    // Timers' initialization
    record_timer_handle = xTimerCreateStatic(
        "AUDIO", 1, pdFALSE, nullptr, systemstate::record_end_callback, &record_timer_buffer);
    if(nullptr == record_timer_handle)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    // Tasks' initialization
    audio_context.commands_queue = audio_commands_queue.handle;
    audio_context.status_queue = audio_status_queue.handle;
    audio_context.data_queue = audio_data_queue.handle;

    const auto audio_task_init_result = audio_task.init(audio::task_audio, "AUDIO", &audio_context);
    if(result::Result::OK != audio_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    audio_tester_context.data_queue = audio_data_queue.handle;
    audio_tester_context.commands_queue = audio_tester_commands_queue.handle;

    const auto audio_tester_task_init_result =
        audio_tester_task.init(audio::tester::task_audio_tester, "AUTEST", &audio_tester_context);
    if(result::Result::OK != audio_tester_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    cli_context.cli_commands_handle = cli_commands_queue.handle;
    cli_context.cli_status_handle = cli_status_queue.handle;

    const auto log_task_init_result = log_task.init(logger::task_cli_logger, "CLI", &cli_context);
    if(result::Result::OK != log_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    systemstate_context.cli_commands_handle = cli_commands_queue.handle;
    systemstate_context.cli_status_handle = cli_status_queue.handle;
    systemstate_context.audio_commands_handle = audio_commands_queue.handle;
    systemstate_context.audio_status_handle = audio_status_queue.handle;
    systemstate_context.record_timer_handle = record_timer_handle;
    systemstate_context.audio_tester_commands_handle = audio_tester_commands_queue.handle;
    systemstate_context.memory_commands_handle = memory_commands_queue.handle;
    systemstate_context.memory_status_handle = memory_status_queue.handle;
    systemstate_context.ble_commands_handle = ble_commands_queue.handle;
    systemstate_context.ble_requests_handle = ble_requests_queue.handle;
    systemstate_context.ble_keepalive_handle = ble_keepalive_queue.handle;
    systemstate_context.led_commands_handle = led_commands_queue.handle;
    systemstate_context.button_events_handle = button_event_queue.handle;
    systemstate_context.button_requests_handle = button_requests_queue.handle;
    systemstate_context.button_response_handle = button_response_queue.handle;
    systemstate_context.battery_measurements_handle = battery_measurements_queue.handle;
    systemstate_context.battery_level_to_ble_handle = ble_batt_info_queue.handle;

    const auto systemstate_task_init_result =
        systemstate_task.init(systemstate::task_system_state, "STATE", &systemstate_context);
    if(result::Result::OK != systemstate_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    memory_context.command_queue = memory_commands_queue.handle;
    memory_context.status_queue = memory_status_queue.handle;
    memory_context.command_from_ble_queue = ble_to_mem_commands_queue.handle;
    memory_context.status_to_ble_queue = ble_from_mem_status_queue.handle;
    memory_context.data_to_ble_queue = ble_from_mem_data_queue.handle;
    memory_context.audio_data_queue = audio_data_queue.handle;
    memory_context.commands_to_rtc_queue = rtc_commands_queue.handle;
    memory_context.response_from_rtc_queue = rtc_response_queue.handle;
    const auto memory_task_init_result =
        memory_task.init(memory::task_memory, "MEM", &memory_context);

    if(result::Result::OK != memory_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    ble_context.command_queue = ble_commands_queue.handle;
    ble_context.requests_queue = ble_requests_queue.handle;
    ble_context.keepalive_queue = ble_keepalive_queue.handle;
    ble_context.command_to_mem_queue = ble_to_mem_commands_queue.handle;
    ble_context.status_from_mem_queue = ble_from_mem_status_queue.handle;
    ble_context.data_from_mem_queue = ble_from_mem_data_queue.handle;
    ble_context.commands_to_rtc_queue = rtc_commands_queue.handle;
    ble_context.battery_to_ble_queue = ble_batt_info_queue.handle;
    const auto ble_task_init_result = ble_task.init(ble::task_ble, "BLE", &ble_context);

    if(result::Result::OK != ble_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    led_context.commands_queue = led_commands_queue.handle;
    const auto led_task_init_result = led_task.init(led::task_led, "LED", &led_context);

    if(result::Result::OK != led_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    rtc_context.command_queue = rtc_commands_queue.handle;
    rtc_context.response_queue = rtc_response_queue.handle;
    const auto rtc_task_init_result = rtc_task.init(rtc::task_rtc, "RTC", &rtc_context);

    if(result::Result::OK != rtc_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    button_context.events_handle = button_event_queue.handle;
    button_context.response_handle = button_response_queue.handle;
    button_context.commands_handle = button_requests_queue.handle;
    const auto button_task_init_result =
        button_task.init(button::task_button, "BTN", &button_context);

    if(result::Result::OK != button_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    battery_context.measurements_handle = battery_measurements_queue.handle;
    const auto battery_task_init_result =
        battery_task.init(battery::task_battery, "BAT", &battery_context);

    if(result::Result::OK != battery_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto wdt_task_init_result = wdt_task.init(wdt::task_wdt, "WDT", nullptr);
    if (result::Result::OK != wdt_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    vTaskStartScheduler();
}
