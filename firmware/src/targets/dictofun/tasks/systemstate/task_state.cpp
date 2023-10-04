// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_state.h"
#include "result.h"

#include <nrf_gpio.h>
#include <nrf_log.h>

#include "task_audio.h"
#include "task_audio_tester.h"
#include "task_ble.h"
#include "task_button.h"
#include "task_cli_logger.h"
#include "task_led.h"
#include "task_memory.h"
#include "task_battery.h"

#include "nvconfig.h"

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <timers.h>

namespace systemstate
{

logger::CliCommandQueueElement cli_command_buffer;
ble::RequestQueueElement ble_requests_buffer;
button::EventQueueElement button_event_buffer;
battery::MeasurementsQueueElement battery_measurement_buffer;

application::NvConfig _nvconfig{vTaskDelay};
application::NvConfig::Configuration _configuration;

constexpr TickType_t cli_command_wait_ticks_type{10};
constexpr TickType_t ble_request_wait_ticks_type{1};
constexpr TickType_t button_event_wait_ticks_type{1};
constexpr TickType_t battery_request_wait_ticks_type{0};

Context* context{nullptr};

void process_button_event(button::Event event, Context& context);

void task_system_state(void* context_ptr)
{
    configure_power_latch();
    NRF_LOG_INFO("task state: initialized");
    context = reinterpret_cast<Context*>(context_ptr);
    // Process NV configuration. If it doesn't exist - memory operation should be scheduled.
    const auto nvconfig_load_result = _nvconfig.load_early(_configuration);
    bool is_operation_mode_defined{true};
    static constexpr uint32_t nvconfig_definition_timestamp{2000};
    if(result::Result::OK != nvconfig_load_result)
    {
        NRF_LOG_WARNING("state: failed to early load application config");
        is_operation_mode_defined = false;
    }
    else
    {
        NRF_LOG_INFO("state: operation mode %s",
                     _configuration.mode == application::DEVELOPMENT   ? "DEV"
                     : _configuration.mode == application::ENGINEERING ? "ENG"
                                                                       : "FIELD");
        if(application::DEVELOPMENT == _configuration.mode)
        {
            led::CommandQueueElement led_command{led::Color::WHITE, led::State::SLOW_GLOW};
            xQueueSend(context->led_commands_handle, reinterpret_cast<void*>(&led_command), 0);
        }
        else
        {
            led::CommandQueueElement led_command{led::Color::GREEN, led::State::SLOW_GLOW};
            xQueueSend(context->led_commands_handle, reinterpret_cast<void*>(&led_command), 0);
            // TODO: add BLE enable, if button was not pressed
        }
    }

    while(1)
    {
        const auto button_event_receive_status = xQueueReceive(
            context->button_events_handle, &button_event_buffer, button_event_wait_ticks_type);
        if(pdPASS == button_event_receive_status)
        {
            process_button_event(button_event_buffer.event, *context);
        }

        const auto cli_queue_receive_status =
            xQueueReceive(context->cli_commands_handle,
                          reinterpret_cast<void*>(&cli_command_buffer),
                          cli_command_wait_ticks_type);
        if(pdPASS == cli_queue_receive_status)
        {
            switch(cli_command_buffer.command_id)
            {
            case logger::CliCommand::RECORD: {
                launch_cli_command_record(
                    *context, cli_command_buffer.args[0], cli_command_buffer.args[1] > 0);
                break;
            }
            case logger::CliCommand::MEMORY_TEST: {
                launch_cli_command_memory_test(*context, cli_command_buffer.args[0]);
                break;
            }
            case logger::CliCommand::BLE_COMMAND: {
                launch_cli_command_ble_operation(*context, cli_command_buffer.args[0]);
                break;
            }
            case logger::CliCommand::SYSTEM: {
                launch_cli_command_system(*context, cli_command_buffer.args[0]);
                break;
            }
            case logger::CliCommand::OPMODE: {
                launch_cli_command_opmode(*context, cli_command_buffer.args[0], _nvconfig);
                break;
            }
            case logger::CliCommand::LED: {
                launch_cli_command_led(
                    *context, cli_command_buffer.args[0], cli_command_buffer.args[1]);
                break;
            }
            }
        }
        const auto ble_request_receive_status =
            xQueueReceive(context->ble_requests_handle,
                          reinterpret_cast<void*>(&ble_requests_buffer),
                          ble_request_wait_ticks_type);
        if(pdPASS == ble_request_receive_status)
        {
            if(ble_requests_buffer.request == ble::Request::LED)
            {
                led::CommandQueueElement cmd{led::user_color,
                                             (ble_requests_buffer.args[0] == 0) ? led::State::OFF
                                                                                : led::State::ON};
                const auto led_command_send_status =
                    xQueueSend(context->led_commands_handle, reinterpret_cast<void*>(&cmd), 0);
                if(pdPASS != led_command_send_status)
                {
                    NRF_LOG_ERROR("state: failed to send led enable request");
                }
            }
        }
        const auto battery_measurement_receive_status = 
            xQueueReceive(
                context->battery_measurements_handle, 
                reinterpret_cast<void *>(&battery_measurement_buffer), 
                battery_request_wait_ticks_type);
        if (pdPASS == battery_measurement_receive_status)
        {
            // TODO: make decisions based on battery level, and provide the level to the BLE task
        }

        if(!is_operation_mode_defined && xTaskGetTickCount() > nvconfig_definition_timestamp)
        {
            is_operation_mode_defined = true;
            load_nvconfig(*context);
        }

        process_timeouts(*context);
    }
}

result::Result request_record_creation(const Context& context)
{
    // First cleanup response from memory queue
    xQueueReset(context.memory_status_handle);

    memory::CommandQueueElement cmd{memory::Command::CREATE_RECORD, {0, 0}};
    const auto create_file_res =
        xQueueSend(context.memory_commands_handle, reinterpret_cast<void*>(&cmd), 0);
    if(create_file_res != pdPASS)
    {
        return result::Result::ERROR_GENERAL;
    }
    memory::StatusQueueElement response;
    static constexpr uint32_t file_creation_wait_time{6000};
    const auto create_file_response_res = xQueueReceive(
        context.memory_status_handle, reinterpret_cast<void*>(&response), file_creation_wait_time);
    if(create_file_response_res != pdPASS)
    {
        NRF_LOG_WARNING(
            "task state: response from mem timeout. Record file might not have been created");
        return result::Result::ERROR_TIMEOUT;
    }
    return result::Result::OK;
}

result::Result request_record_start(const Context& context)
{
    audio::CommandQueueElement cmd{audio::Command::RECORD_START};
    const auto record_start_status =
        xQueueSend(context.audio_commands_handle, reinterpret_cast<void*>(&cmd), 0);
    return (record_start_status == pdPASS) ? result::Result::OK : result::Result::ERROR_GENERAL;
}

result::Result request_record_stop(const Context& context)
{
    audio::CommandQueueElement cmd{audio::Command::RECORD_STOP};
    const auto record_stop_result =
        xQueueSend(context.audio_commands_handle, reinterpret_cast<void*>(&cmd), 0);
    return (record_stop_result == pdPASS) ? result::Result::OK : result::Result::ERROR_GENERAL;
}

result::Result request_record_closure(const Context& context)
{
    xQueueReset(context.memory_status_handle);

    memory::CommandQueueElement cmd{memory::Command::CLOSE_WRITTEN_FILE, {0, 0}};
    const auto close_file_res =
        xQueueSend(context.memory_commands_handle, reinterpret_cast<void*>(&cmd), 0);
    if(close_file_res != pdPASS)
    {
        NRF_LOG_ERROR("task state: failed to request file closure");
        return result::Result::ERROR_GENERAL;
    }
    memory::StatusQueueElement response;
    static constexpr uint32_t file_closure_max_wait_time{1000};
    const auto file_closure_status_result = xQueueReceive(context.memory_status_handle,
                                                          reinterpret_cast<void*>(&response),
                                                          file_closure_max_wait_time);
    if(pdTRUE != file_closure_status_result)
    {
        return result::Result::ERROR_TIMEOUT;
    }
    if(response.status != memory::Status::OK)
    {
        NRF_LOG_ERROR("state: file closure has failed");
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

void process_button_event(button::Event event, Context& context)
{
    const auto operation_mode = get_operation_mode();
    switch(operation_mode)
    {
    case decltype(operation_mode)::DEVELOPMENT: {
        // There is no reaction required in this case
        break;
    }
    case decltype(operation_mode)::ENGINEERING:
    case decltype(operation_mode)::FIELD: {
        if(event == decltype(event)::SINGLE_PRESS_ON)
        {
            if(context._is_ble_system_active)
            {
                const auto ble_disable_status = disable_ble_subsystem(context);
                if(result::Result::OK != ble_disable_status)
                {
                    NRF_LOG_ERROR("state: failed to request BLE disable");
                }

                xQueueReset(context.memory_status_handle);
                memory::CommandQueueElement command_to_memory{memory::Command::SELECT_OWNER_AUDIO,
                                                              {0, 0}};
                const auto owner_switch_result = xQueueSend(
                    context.memory_commands_handle, reinterpret_cast<void*>(&command_to_memory), 0);
                if(pdPASS != owner_switch_result)
                {
                    NRF_LOG_ERROR("state: failed to request owner switch to audio. Record might "
                                  "not be saved");
                }
                memory::StatusQueueElement response;
                static constexpr uint32_t memory_ownership_switch_timeout{1000};
                const auto memory_response_result =
                    xQueueReceive(context.memory_status_handle,
                                  reinterpret_cast<void*>(&response),
                                  memory_ownership_switch_timeout);
                if(pdPASS != memory_response_result ||
                   response.status != decltype(response.status)::OK)
                {
                    NRF_LOG_ERROR(
                        "state: memory ownership switch has failed. Record won't be launched");
                    return;
                }
            }

            const auto creation_result = request_record_creation(context);
            if(decltype(creation_result)::OK != creation_result)
            {
                NRF_LOG_ERROR("state: failed to create record upon button press");
                return;
            }
            context.is_record_active = true;
            const auto record_start_result = request_record_start(context);
            if(decltype(record_start_result)::OK != record_start_result)
            {
                NRF_LOG_ERROR("state: failed to launch record upon button press");
                return;
            }
            NRF_LOG_INFO("state: launched record start");
            context.timestamps.last_record_start_timestamp = xTaskGetTickCount();

            // This is the point where LED indication should be changed
            led::CommandQueueElement led_command{led::Color::RED, led::State::FAST_GLOW};
            xQueueSend(context.led_commands_handle, reinterpret_cast<void*>(&led_command), 0);

            // Also at this point we should tell BLE to switch to access the real FS (not the simulated one). Operation result in this case doesn't really matter.
            ble::CommandQueueElement cmd{ble::Command::CONNECT_FS};
            xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        }
        else
        {
            const auto record_stop_result = request_record_stop(context);
            if(decltype(record_stop_result)::OK != record_stop_result)
            {
                NRF_LOG_ERROR("state: failed to stop record upon button release");
                return;
            }
            const auto closure_result = request_record_closure(context);
            if(decltype(closure_result)::OK != closure_result)
            {
                NRF_LOG_ERROR("state: failed to close record upon button release");
                return;
            }
            context.is_record_active = false;
            NRF_LOG_INFO("state: stopped and saved record");

            // Change memory owner
            memory::CommandQueueElement command_to_memory{memory::Command::SELECT_OWNER_BLE,
                                                          {0, 0}};
            const auto owner_switch_result = xQueueSend(
                context.memory_commands_handle, reinterpret_cast<void*>(&command_to_memory), 0);
            if(pdPASS != owner_switch_result)
            {
                NRF_LOG_ERROR("state: failed to request owner switch to BLE.");
            }

            led::CommandQueueElement led_command{led::Color::DARK_BLUE, led::State::SLOW_GLOW};
            xQueueSend(context.led_commands_handle, reinterpret_cast<void*>(&led_command), 0);

            // Enable BLE subsystem, if it's not enabled yet
            const auto ble_enable_result = enable_ble_subsystem(context);
            if(decltype(ble_enable_result)::OK != ble_enable_result)
            {
                NRF_LOG_ERROR("state: failed to enable BLE subsystem");
            }
        }
        break;
    }
    }
}

/// @brief Unfortunately, nvconfig can't contain all dependencies within the module.
/// This happens because fstorage has to use SD-aware implementation, and in FreeRTOS-based system
/// it works only if SD task is active. So in order to write-access fstorage we have to enable BLE
/// subsystem, if it's not enabled yet, and disable afterwards, if needed.
void load_nvconfig(Context& context)
{
    bool need_to_disable_ble_subsystem{false};
    if(!context._is_ble_system_active)
    {
        if(result::Result::OK != enable_ble_subsystem(context))
        {
            return;
        }
        need_to_disable_ble_subsystem = true;
        context._is_ble_system_active = true;
        // Wait to make sure BLE task has been started
        static constexpr uint32_t ble_start_wait_ticks{10};
        vTaskDelay(ble_start_wait_ticks);
    }

    const auto nvconfig_load_result = _nvconfig.load(_configuration);
    if(result::Result::OK != nvconfig_load_result)
    {
        NRF_LOG_ERROR("deferred load nvconfig has failed.");
    }

    if(need_to_disable_ble_subsystem)
    {
        ble::CommandQueueElement cmd{ble::Command::STOP};
        const auto ble_comm_status =
            xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        if(ble_comm_status != pdPASS)
        {
            NRF_LOG_ERROR("nvconfig: failed to queue BLE stop operation");
            return;
        }
        // FIXME: so far BLE subsystem can't be stopped/suspended due to how freertos SDH task API is implemented.
        //context._is_ble_system_active = false;
    }
}

application::Mode get_operation_mode()
{
    return _configuration.mode;
}

bool is_record_start_by_cli_allowed(Context& context)
{
    const auto operation_mode = get_operation_mode();
    if((operation_mode != decltype(operation_mode)::DEVELOPMENT) || context.is_record_active)
    {
        return false;
    }
    return true;
}

result::Result enable_ble_subsystem(Context& context)
{
    if(!context._is_ble_system_active)
    {
        ble::CommandQueueElement cmd{ble::Command::START};
        const auto ble_start_status =
            xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        if(ble_start_status != pdPASS)
        {
            NRF_LOG_ERROR("failed to queue BLE start operation");
            return result::Result::ERROR_GENERAL;
        }
        context._is_ble_system_active = true;
    }

    return result::Result::OK;
}

result::Result disable_ble_subsystem(Context& context)
{
    if(context._is_ble_system_active)
    {
        ble::CommandQueueElement cmd{ble::Command::STOP};
        const auto ble_stop_status =
            xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        if(ble_stop_status != pdPASS)
        {
            NRF_LOG_ERROR("failed to queue BLE stop operation");
            return result::Result::ERROR_GENERAL;
        }
        context._is_ble_system_active = false;
    }
    return result::Result::OK;
}

void record_end_callback(TimerHandle_t timer)
{
    context->is_record_active = false;
    if(result::Result::OK != request_record_stop(*context))
    {
        NRF_LOG_ERROR("state: failed to request audio record stop");
    }

    if(context->_should_record_be_stored)
    {
        const auto closure_result = request_record_closure(*context);
        if(decltype(closure_result)::OK != closure_result)
        {
            NRF_LOG_ERROR("state: failed to closed record (requested by CLI)");
        }
    }
    else
    {
        audio::tester::ControlQueueElement tester_cmd;
        tester_cmd.should_enable_tester = false;
        [[maybe_unused]] const auto tester_start_status = xQueueSend(
            context->audio_tester_commands_handle, reinterpret_cast<void*>(&tester_cmd), 0);
    }
}

result::Result launch_record_timer(const TickType_t record_duration, Context& context)
{
    const auto period_change_result =
        xTimerChangePeriod(context.record_timer_handle, record_duration, 0);

    if(pdPASS != period_change_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    const auto timer_start_result = xTimerStart(context.record_timer_handle, 0);
    if(timer_start_result != pdPASS)
    {
        return result::Result::ERROR_GENERAL;
    }

    context.is_record_active = true;
    return result::Result::OK;
}

void process_timeouts(Context& context)
{
    const auto operation_mode = get_operation_mode();
    if(operation_mode == decltype(operation_mode)::DEVELOPMENT)
    {
        return;
    }

    static constexpr uint32_t norecord_launch_timeout{15000};
    static constexpr uint32_t after_record_timeout{15000};
    static constexpr uint32_t ble_keepalive_timeout{10000};
    static constexpr uint32_t ble_after_disconnect_timeout{5000};
    // We should unconditionally shutdown after 10 minutes of operation
    static constexpr uint32_t max_operation_duration{10 * 60 * 1000};

    // First update all relevant timeouts
    if(context.is_record_active)
    {
        context.timestamps.last_record_end_timestamp = xTaskGetTickCount();
    }
    ble::KeepaliveQueueElement keepalive;
    const auto ble_keepalive_receive_result =
        xQueueReceive(context.ble_keepalive_handle, &keepalive, 0);
    if(pdTRUE == ble_keepalive_receive_result)
    {
        const auto event = keepalive.event;
        using event_type = decltype(event);
        if(event == event_type::CONNECTED)
        {
            led::CommandQueueElement led_command{led::Color::DARK_BLUE, led::State::FAST_GLOW};
            xQueueSend(context.led_commands_handle, reinterpret_cast<void*>(&led_command), 0);
        }
        if(event == event_type::CONNECTED || event == event_type::FILESYSTEM_EVENT)
        {
            context.timestamps.last_ble_activity_timestamp = xTaskGetTickCount();
        }
        else if(event == event_type::DISCONNECTED)
        {
            context.timestamps.ble_disconnect_event_timestamp = xTaskGetTickCount();
            led::CommandQueueElement led_command{led::Color::PURPLE, led::State::SLOW_GLOW};
            xQueueSend(context.led_commands_handle, reinterpret_cast<void*>(&led_command), 0);
        }
    }

    // At this stage depending on record state and presence of timestamps make decision
    // whether we should shutdown
    const auto tick = xTaskGetTickCount();
    if(!context.timestamps.has_start_timestamp_been_updated())
    {
        // Record has never been launched.
        if(tick > norecord_launch_timeout)
        {
            shutdown_ldo();
        }
        return;
    }
    if(context.is_record_active)
    {
        return;
    }
    // At this point recording is over.
    const auto time_since_record_end = tick - context.timestamps.last_record_end_timestamp;
    if(!context.timestamps.has_ble_timestamp_been_updated() &&
       time_since_record_end > after_record_timeout)
    {
        shutdown_ldo();
    }

    if(context.timestamps.has_ble_timestamp_been_updated())
    {
        if(context.timestamps.has_disconnect_timestamp_been_updated())
        {
            const auto time_since_disconnect =
                tick - context.timestamps.ble_disconnect_event_timestamp;
            if(time_since_disconnect > ble_after_disconnect_timeout)
            {
                shutdown_ldo();
            }
        }
        else
        {
            const auto time_since_last_keepalive =
                tick - context.timestamps.last_ble_activity_timestamp;
            if(time_since_last_keepalive > ble_keepalive_timeout)
            {
                shutdown_ldo();
            }
        }
    }
    if(tick > max_operation_duration)
    {
        shutdown_ldo();
    }
}

} // namespace systemstate
