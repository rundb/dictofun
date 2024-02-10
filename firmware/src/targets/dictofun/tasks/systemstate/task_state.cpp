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
#include "task_battery.h"
#include "task_ble.h"
#include "task_button.h"
#include "task_cli_logger.h"
#include "task_led.h"
#include "task_memory.h"

#include "nvconfig.h"

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <timers.h>
#include "noinit_mem.h"

namespace systemstate
{

logger::CliCommandQueueElement cli_command_buffer;
ble::RequestQueueElement ble_requests_buffer;
button::EventQueueElement button_event_buffer;
battery::MeasurementsQueueElement battery_measurement_buffer;
memory::StatusQueueElement memory_status_buffer;
memory::EventQueueElement memory_event_buffer;

application::NvConfig _nvconfig{vTaskDelay};
application::NvConfig::Configuration _configuration;

constexpr TickType_t cli_command_wait_ticks_type{2};
constexpr TickType_t ble_request_wait_ticks_type{1};
constexpr TickType_t button_event_wait_ticks_type{1};
constexpr TickType_t battery_request_wait_ticks_type{0};

constexpr TickType_t battery_print_period{10000};

constexpr TickType_t idle_ble_start_delay{800};

Context* context{nullptr};

void analyze_reset_reason(Context& context);
void process_button_event(button::Event event, Context& context);
void process_battery_event(battery::MeasurementsQueueElement measurement, Context& context);
void print_battery_voltage(float voltage);
void switch_to_ble_mode(Context& context);
static void process_cli_command(logger::CliCommand cliCommand, uint32_t* args, Context& context, application::NvConfig& nvConfig);
static button::ButtonState fetch_button_state(Context& context);

void handle_led_indication(const SystemState system_state);

enum class MemoryOwner{BLE, AUDIO};
void set_memory_ownership(const MemoryOwner owner, Context& context);

// Implements state machine, described in the diagram `df-states.drawio.png`
void task_system_state(void* context_ptr)
{
    configure_power_latch();
    NRF_LOG_DEBUG("task state: initialized");
    context = reinterpret_cast<Context*>(context_ptr);

    context->system_state = SystemState::INIT;

    analyze_reset_reason(*context);

    // TODO: add battery level evalutation at this point
    
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

            context->system_state = SystemState::DEVELOPMENT_MODE;
        }
        else
        {
            // figure out the current button state
            const auto button_state = fetch_button_state(*context);
            switch (button_state)
            {
                case button::ButtonState::PRESSED: 
                {
                    // start preparing record
                    context->system_state = SystemState::RECORD_PREPARE;
                    break;
                }
                case button::ButtonState::RELEASED: default:
                {
                    // switch to BLE operation
                    context->system_state = SystemState::INIT;
                    break;
                }
            }
        }
    }

    SystemState next_state{context->system_state};
    NRF_LOG_INFO("state: starting main with state %d", context->system_state);

    handle_led_indication(context->system_state);

    while(1)
    {
        switch (context->system_state)
        {
            case SystemState::INIT:
            {
                // check the button
                // if it is still released - wait until BLE is ready
                const auto button_state = fetch_button_state(*context);
                if (button_state == button::ButtonState::PRESSED)
                {
                    next_state = SystemState::RECORD_PREPARE;
                }
                if (xTaskGetTickCount() >= idle_ble_start_delay)
                {
                    next_state = SystemState::BLE_OPERATION;
                }

                break;
            }
            case SystemState::RECORD_PREPARE:
            {
                // prepare record - prepare filesystem, wait for the signal from FS
                if (context->memory_status == Context::MemoryStatus::BUSY)
                {
                    break;
                }
                
                if (context->memory_status == Context::MemoryStatus::READY)
                {
                    set_memory_ownership(MemoryOwner::AUDIO, *context);


                    const auto create_result = request_record_creation(*context);
                    if (result::Result::OK != create_result)
                    {
                        NRF_LOG_ERROR("state: record creation reqeuest has failed");
                        // TODO: apply a correct state transition
                        // Technically - this should never happen.
                        next_state = SystemState::MEMORY_IS_CORRUPT;
                    }
                    const auto record_start_result = request_record_start(*context);
                    if (result::Result::OK != record_start_result)
                    {
                        NRF_LOG_ERROR("state: record start request has failed");
                        // TODO: apply a correct state transition
                        // Technically - this should never happen.
                        next_state = SystemState::MEMORY_IS_CORRUPT;
                    }
                    else 
                    {
                        next_state = SystemState::RECORDING;
                    }
                }
                else if (context->memory_status == Context::MemoryStatus::OUT_OF_MEMORY) 
                {
                    // otherwise switch to out of memory or corrupt state
                    next_state = SystemState::OUT_OF_MEMORY;
                }
                else if (context->memory_status == Context::MemoryStatus::CORRUPT)
                {
                    next_state = SystemState::MEMORY_IS_CORRUPT;
                }
                break;
            }
            case SystemState::RECORDING:
            {
                // check status from memory
                if (context->memory_status == Context::MemoryStatus::OUT_OF_MEMORY || 
                    context->memory_status == Context::MemoryStatus::CORRUPT)
                {
                    [[maybe_unused]] const auto record_stop_result = request_record_stop(*context);
                    next_state = SystemState::RECORD_END;
                    break;
                }

                // check events from the button.
                const auto button_state = fetch_button_state(*context);
                if (button_state == button::ButtonState::RELEASED)
                {
                    NRF_LOG_DEBUG("state: polling button released");
                    next_state = SystemState::RECORD_END;
                    break;
                }

                // in both cases proceed eventually to record end state
                break;
            }
            case SystemState::RECORD_END:
            {
                // check that memory has not sent "corrupt"/"out of memory" statuses
                // in this case memory closes the file by itself
                if (context->memory_status == Context::MemoryStatus::OUT_OF_MEMORY)
                {
                    next_state = SystemState::OUT_OF_MEMORY;
                    break;
                }
                else if (context->memory_status == Context::MemoryStatus::CORRUPT)
                {
                    next_state = SystemState::MEMORY_IS_CORRUPT;
                    break;
                }

                // normal case: close the record file
                const auto closure_request_result = request_record_closure(*context);
                if (result::Result::OK != closure_request_result)
                {
                    NRF_LOG_ERROR("state: failed to close a record. TODO: define appropriate action");
                    next_state = SystemState::MEMORY_IS_CORRUPT;
                    break;
                }
                // proceed to BLE operation
                next_state = SystemState::BLE_OPERATION;

                break;
            }
            case SystemState::BLE_OPERATION:
            {
                // Process timeouts and events from the buttons
                const auto button_state = fetch_button_state(*context);
                if (button::ButtonState::PRESSED == button_state)
                {
                    // stop BLE subsystem
                    const auto disabling_result = disable_ble_subsystem(*context);
                    if (result::Result::OK != disabling_result)
                    {
                        NRF_LOG_ERROR("state: ble disable request has failed");
                        // TODO: define appropriate action
                    }

                    // set back the record prepare state
                    next_state = SystemState::RECORD_PREPARE;
                }

                break;
            }
            case SystemState::BATTERY_LOW:
            {
                // indicate with color, after several seconds - shutdown (before shutdown make sure button is not pressed)
                break;
            }
            case SystemState::OUT_OF_MEMORY:
            {
                // Indicate with color, after several seconds (and if the button is not pressed) - switch to BLE operation
                NRF_LOG_ERROR("out of memory");
                
                vTaskDelay(1000);
                break;
            }
            case SystemState::DEVELOPMENT_MODE:
            {
                // basically do nothing, CLI processing should be done at separate cycle element
                break;
            }
            case SystemState::MEMORY_IS_CORRUPT:
            {
                // issue repair command, stay in this state until memory reports readyness, then shutdown
                NRF_LOG_ERROR("memory is corrupt");
                vTaskDelay(1000);
                break;
            }
            case SystemState::SHUTDOWN:
            {
                NRF_LOG_ERROR("shutdown state");
                vTaskDelay(1000);
                shutdown_ldo();
                break;
            }
        }
        if (context->system_state != next_state)
        {
            NRF_LOG_INFO("state: transition %d->%d", (int)context->system_state, (int)next_state);
            // change state from another state to BLE state needs additional processing (to avoid adding extra context variable)
            if (next_state == SystemState::BLE_OPERATION)
            {
                const auto ble_start_result = enable_ble_subsystem(*context);
                if (result::Result::OK != ble_start_result)
                {
                    NRF_LOG_ERROR("state: ble enablement has failed");
                    // TODO: define appropriate actions
                }
            }

            context->system_state = next_state;
            handle_led_indication(context->system_state);
            
        }

        vTaskDelay(10);
        
        // This section runs through the relevant event queues and updates respective context fields. State machine then makes decisions based on the 
        // events, if it effects the execution.

        // At the beginning of the execution, wait for the readiness flag from the memory
        if (context->memory_status == Context::MemoryStatus::BUSY)
        {
            memory::StatusQueueElement response;
            const auto memoryStatusResult = xQueueReceive(context->memory_status_handle, reinterpret_cast<void *>(&response), 100);
            if (pdPASS == memoryStatusResult)
            {
                if (response.status == memory::Status::IS_READY)
                {
                    context->memory_status = Context::MemoryStatus::READY;
                    NRF_LOG_DEBUG("state: memory has reported readiness");
                }
                else if (response.status == memory::Status::ERROR_OUT_OF_MEMORY)
                {
                    context->memory_status = Context::MemoryStatus::OUT_OF_MEMORY;
                    NRF_LOG_WARNING("state: memory has reported out-of-memory");
                }
                else if (response.status == memory::Status::ERROR_FATAL || response.status == memory::Status::ERROR_GENERAL)
                {
                    context->memory_status = Context::MemoryStatus::CORRUPT;
                    NRF_LOG_WARNING("state: memory has reported corrupt");
                }
                else
                {
                    context->memory_status = Context::MemoryStatus::BUSY;
                    NRF_LOG_ERROR("state: memory reported an unknown state");
                }
            }
        }

        // Process CLI commands
        const auto cli_queue_receive_status = xQueueReceive(context->cli_commands_handle, reinterpret_cast<void*>(&cli_command_buffer), 0);
        if(pdPASS == cli_queue_receive_status)
        {
            process_cli_command(cli_command_buffer.command_id, cli_command_buffer.args, *context, _nvconfig);
        }

        // Process button events. Only for the case of sophisticated button events (like a sequence for a factory reset)
        // Generally now buttons work in a polling mode
        const auto button_event_receive_status = xQueueReceive(context->button_events_handle, &button_event_buffer, 0);
        if(pdPASS == button_event_receive_status)
        {
            process_button_event(button_event_buffer.event, *context);
        }

        // Process battery events
        const auto battery_measurement_receive_status = xQueueReceive(context->battery_measurements_handle, reinterpret_cast<void*>(&battery_measurement_buffer), 0);
        if(pdPASS == battery_measurement_receive_status)
        {
            process_battery_event(battery_measurement_buffer, *context);
        }

        const auto memory_event_receive_status = xQueueReceive(context->memory_event_handle, &memory_event_buffer, 0);
        if (pdPASS == memory_event_receive_status)
        {
            if (memory_event_buffer.status == memory::Status::ERROR_OUT_OF_MEMORY)
            {
                context->memory_status = Context::MemoryStatus::OUT_OF_MEMORY;
                NRF_LOG_ERROR("state: mem reported out of memory error");
            }
        }
            

        // Handle the deferred NVM initiation 
        if(!is_operation_mode_defined && xTaskGetTickCount() > nvconfig_definition_timestamp)
        {
            is_operation_mode_defined = true;
            load_nvconfig(*context);
        }

        // const auto is_timeout_expired = process_timeouts(*context);
        // if (is_timeout_expired && !context->_is_shutdown_demanded)
        // {
        //     context->_is_shutdown_demanded = true;
        //     context->timestamps.shutdown_procedure_start_timestamp = xTaskGetTickCount();
        //     NRF_LOG_INFO("ready for shutdown. Checking memory");
        //     // At this point request a memory check from the task_memory
        //     memory::CommandQueueElement cmd{memory::Command::PERFORM_MEMORY_CHECK, {0, 0}};
        //     const auto memcheck_res =
        //         xQueueSend(context->memory_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        //     if (pdPASS != memcheck_res)
        //     {
        //         // Failed to send the memcheck command => shut the system down.
        //         shutdown_ldo();
        //     }
        //     led::CommandQueueElement led_command{led::Color::PURPLE, led::State::SLOW_GLOW};
        //     xQueueSend(context->led_commands_handle, reinterpret_cast<void*>(&led_command), 0);

        //     static constexpr uint32_t MEMCHECK_INITIAL_RESPONSE_TIMEOUT{15000};
        //     static constexpr uint32_t MEMCHECK_FORMAT_TIMEOUT{50000};
        //     memory::StatusQueueElement response;
        //     xQueueReset(context->memory_status_handle);
        //     const auto memcheck_status = xQueueReceive(context->memory_status_handle, &response, MEMCHECK_INITIAL_RESPONSE_TIMEOUT);
        //     NRF_LOG_INFO("memcheck completed.");
        //     if (pdPASS != memcheck_status || response.status == memory::Status::FORMAT_REQUIRED)
        //     {
        //         cmd.command_id = memory::Command::FORMAT_FS;
        //         const auto format_res =
        //             xQueueSend(context->memory_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        //         if (pdPASS != format_res)
        //         {
        //             // Failed to send the memcheck command => shut the system down.
        //             shutdown_ldo();
        //         }

        //         led_command.state = led::State::FAST_GLOW;
        //         xQueueSend(context->led_commands_handle, reinterpret_cast<void*>(&led_command), 0);
        //         context->is_memory_busy = true;

        //         const auto format_status = xQueueReceive(context->memory_status_handle, &response, MEMCHECK_FORMAT_TIMEOUT);
        //         if (pdPASS != format_status)
        //         {
        //             NRF_LOG_ERROR("FS format has failed");
        //         }
        //         shutdown_ldo();
        //     }
        //     else 
        //     {
        //         // a small delay to flush the logs
        //         vTaskDelay(100);
        //         shutdown_ldo();
        //     }

        // }
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
    static constexpr uint32_t file_closure_max_wait_time{10000};
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
}

/// @brief Unfortunately, nvconfig can't contain all dependencies within the module.
/// This happens because fstorage has to use SD-aware implementation, and in FreeRTOS-based system
/// it works only if SD task is active. So in order to write-access fstorage we have to enable BLE
/// subsystem, if it's not enabled yet, and disable afterwards, if needed.
void load_nvconfig(Context& context)
{
    bool need_to_disable_ble_subsystem{false};
    if(!context.is_ble_system_active)
    {
        if(result::Result::OK != enable_ble_subsystem(context))
        {
            return;
        }
        need_to_disable_ble_subsystem = true;
        context.is_ble_system_active = true;
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
        //context.is_ble_system_active = false;
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
    set_memory_ownership(MemoryOwner::BLE, context);
    static constexpr uint32_t ble_activation_wait_ticks{200};

    if(!context.is_ble_system_active)
    {
        ble::CommandQueueElement cmd{ble::Command::START};
        const auto ble_start_status =
            xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), ble_activation_wait_ticks);
        if(ble_start_status != pdPASS)
        {
            NRF_LOG_ERROR("failed to queue BLE start operation");
            return result::Result::ERROR_GENERAL;
        }
        context.is_ble_system_active = true;
    }
    NRF_LOG_INFO("BLE start command has been sent");

    return result::Result::OK;
}

result::Result disable_ble_subsystem(Context& context)
{
    if(context.is_ble_system_active)
    {
        ble::CommandQueueElement cmd{ble::Command::STOP};
        const auto ble_stop_status =
            xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        if(ble_stop_status != pdPASS)
        {
            NRF_LOG_ERROR("failed to queue BLE stop operation");
            return result::Result::ERROR_GENERAL;
        }
        context.is_ble_system_active = false;
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

    if(context->should_record_be_stored)
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

// returns true, if any of the timeouts has expired.
bool process_timeouts(Context& context)
{
    const auto operation_mode = get_operation_mode();
    if(operation_mode == decltype(operation_mode)::DEVELOPMENT)
    {
        return false;
    }

    static constexpr uint32_t norecord_launch_timeout{20000};
    static constexpr uint32_t after_record_timeout{25000};
    static constexpr uint32_t ble_keepalive_timeout{15000};
    static constexpr uint32_t ble_after_disconnect_timeout{10000};
    // We should unconditionally shutdown after 8 minutes of operation
    static constexpr uint32_t max_operation_duration{8 * 60 * 1000};

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
            if (context.records_per_launch_counter <= 1)
            {
                // workaround: since BLE disconnect can happen waaay later than we expect, this timeout reason should be ignored, if more than one record has been made
                context.timestamps.ble_disconnect_event_timestamp = xTaskGetTickCount();
                led::CommandQueueElement led_command{led::Color::PURPLE, led::State::SLOW_GLOW};
                xQueueSend(context.led_commands_handle, reinterpret_cast<void*>(&led_command), 0);
            }
            else {
                context.timestamps.last_ble_activity_timestamp = xTaskGetTickCount();
            }
        }
    }

    // Make sure that erase operation (if running) is for sure completed
    if (context.memory_status != Context::MemoryStatus::READY)
    {
        return false;
    }

    // At this stage depending on record state and presence of timestamps make decision
    // whether we should shutdown
    const auto tick = xTaskGetTickCount();
    if(!context.timestamps.has_start_timestamp_been_updated())
    {
        // Record has never been launched.
        if(tick > norecord_launch_timeout)
        {
            NRF_LOG_DEBUG("shutdown: norecord timeout");
            return true;
        }
        return false;
    }
    if(context.is_record_active)
    {
        return false;
    }
    // At this point recording is over.
    const auto time_since_record_end = tick - context.timestamps.last_record_end_timestamp;
    if(!context.timestamps.has_ble_timestamp_been_updated() &&
       time_since_record_end > after_record_timeout)
    {
        NRF_LOG_DEBUG("shutdown: after record timeout");
        return true;
    }

    if(context.timestamps.has_ble_timestamp_been_updated())
    {
        if(context.timestamps.has_disconnect_timestamp_been_updated())
        {
            const auto time_since_disconnect =
                tick - context.timestamps.ble_disconnect_event_timestamp;
            if(time_since_disconnect > ble_after_disconnect_timeout)
            {
                NRF_LOG_DEBUG("shutdown: after disconnect timeout");
                return true;
            }
        }
        else
        {
            const auto time_since_last_keepalive =
                tick - context.timestamps.last_ble_activity_timestamp;
            if(time_since_last_keepalive > ble_keepalive_timeout)
            {
                NRF_LOG_DEBUG("shutdown: after keepalive timeout");
                return true;
            }
        }
    }
    if(tick > max_operation_duration)
    {
        NRF_LOG_DEBUG("shutdown: after max duration timeout");
        return true;
    }
    return false;
}

void print_battery_voltage(const float voltage)
{
    static TickType_t last_printout_tick{0};
    const auto tick = xTaskGetTickCount();
    if (tick - last_printout_tick > battery_print_period)
    {
        last_printout_tick = tick;
        const int voltage_x_100 = static_cast<int>(voltage * 100.0);
        // workaround here, as NRF_LOG is not capable of printing floats
        const auto whole_part = voltage_x_100 / 100;
        const auto fractional_part = voltage_x_100 % 100;
        NRF_LOG_INFO("state::batt:%d.%dV", whole_part, fractional_part);
    }
}

void switch_to_ble_mode(Context& context)
{
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

void analyze_reset_reason(Context& context)
{
    static constexpr uint32_t max_reset_count_before_destructive_reset{10};
    noinit::NoInitMemory::load();
    auto& nim = noinit::NoInitMemory::instance();

    if (nim.reset_count > max_reset_count_before_destructive_reset)
    {
        // TODO: enforce a file system format and a drop to the initial state
        NRF_LOG_ERROR("state: boot loop detected, a destructive format is required");
    }
    if (nim.reset_reason == noinit::ResetReason::FILE_SYSTEM_ERROR_DURING_RECORD)
    {
        NRF_LOG_WARNING("state: FS error during record has been detected");
    }
    else if (nim.reset_reason == noinit::ResetReason::FILE_SYSTEM_ERROR_DURING_BLE)
    {
        NRF_LOG_WARNING("state: FS error during BLE has been detected. Enabling BLE immediately");
        switch_to_ble_mode(context);
    }
}

void process_cli_command(logger::CliCommand cliCommand, uint32_t* args, Context& context, application::NvConfig& nvConfig)
{
    switch(cliCommand)
    {
        case logger::CliCommand::RECORD: {
            launch_cli_command_record(context, args[0], args[1] > 0);
            break;
        }
        case logger::CliCommand::MEMORY_TEST: {
            launch_cli_command_memory_test(context, args[0], args[1], args[2]);
            break;
        }
        case logger::CliCommand::BLE_COMMAND: {
            launch_cli_command_ble_operation(context, args[0]);
            break;
        }
        case logger::CliCommand::SYSTEM: {
            launch_cli_command_system(context, args[0]);
            break;
        }
        case logger::CliCommand::OPMODE: {
            launch_cli_command_opmode(context, args[0], nvConfig);
            break;
        }
        case logger::CliCommand::LED: {
            launch_cli_command_led(context, args[0], args[1]);
            break;
        }
    }
}

button::ButtonState fetch_button_state(Context& context)
{
    static constexpr uint32_t max_response_wait_time{50};
    button::ButtonState result = button::ButtonState::UNKNOWN;

    xQueueReset(context.button_requests_handle);
    button::RequestQueueElement cmd{button::Command::REQUEST_STATE};
    const auto send_result = xQueueSend(context.button_requests_handle, reinterpret_cast<void*>(&cmd), 0);

    if (pdPASS != send_result)
    {
        NRF_LOG_ERROR("state: failed to request button state");
        return result;
    }

    button::ResponseQueueElement response;
    const auto receive_result = xQueueReceive(context.button_response_handle, &response, max_response_wait_time);
    if (pdPASS == receive_result)
    {
        return response.state;
    }

    NRF_LOG_WARNING("state: failed to receive button state");
    return result;
}

void process_battery_event(battery::MeasurementsQueueElement measurement, Context& context)
{
    // TODO: make decisions based on battery level
    ble::BatteryDataElement battery_level_msg{ measurement.battery_percentage, measurement.battery_voltage_level};
    if (context.is_ble_system_active)
    {
        xQueueReset(context.battery_level_to_ble_handle);
        const auto batt_info_send_result =
            xQueueSend(context.battery_level_to_ble_handle, reinterpret_cast<void*>(&battery_level_msg), 0);
        if(pdPASS != batt_info_send_result)
        {
            NRF_LOG_WARNING("state: failed to send batt level to ble");
        }
    }
    // print battery voltage every 10 seconds.
    print_battery_voltage(battery_measurement_buffer.battery_voltage_level);
    if (measurement.battery_voltage_level< 3.1)
    {
        const int v_x_10 = static_cast<int>(10.0 * battery_measurement_buffer.battery_voltage_level);
        NRF_LOG_ERROR("Battery is low (%d.%dv). No operations permitted.", v_x_10 / 10, v_x_10%10);

        // next iteration of FSM processing will make a decision based on this flag
        context.is_battery_low = true;

        // xQueueSend(context->led_commands_handle, reinterpret_cast<void*>(&led_command), 0);

        // if (context->is_record_active)
        // {
        //     NRF_LOG_ERROR("Closing the record due to battery level");
        //     const auto record_stop_result = request_record_stop(*context);
        //     if(decltype(record_stop_result)::OK != record_stop_result)
        //     {
        //         NRF_LOG_ERROR("state: failed to stop record (battery low)");
        //     }
        //     else 
        //     {
        //         const auto closure_result = request_record_closure(*context);
        //         if(decltype(closure_result)::OK != closure_result)
        //         {
        //             NRF_LOG_ERROR("state: failed to close record upon button release");

        //         }
        //     }
        //     context->is_record_active = false;
        // }
    }
}

void handle_led_indication(const SystemState system_state)
{
    led::CommandQueueElement cmd{led::Color::GREEN, led::State::OFF};
    
    switch (system_state) 
    {
        case SystemState::INIT:
        {
            cmd.color = led::Color::GREEN;
            cmd.state = led::State::SLOW_GLOW;
            break;
        }
        case SystemState::BATTERY_LOW:
        {
            cmd.color = led::Color::YELLOW;
            cmd.state = led::State::FAST_GLOW;
            break;
        }
        case SystemState::DEVELOPMENT_MODE:
        {
            cmd.color = led::Color::WHITE;
            cmd.state = led::State::SLOW_GLOW;
            break;
        }
        case SystemState::RECORD_PREPARE:
        {
            cmd.color = led::Color::GREEN;
            cmd.state = led::State::FAST_GLOW;
            break;
        }
        case SystemState::RECORDING:
        {
            cmd.color = led::Color::RED;
            cmd.state = led::State::SLOW_GLOW;
            break;
        }
        case SystemState::RECORD_END:
        {
            cmd.color = led::Color::RED;
            cmd.state = led::State::SLOW_GLOW;
            break;
        }
        case SystemState::BLE_OPERATION:
        {
            // Only the initial state, afterwards BLE sybsystem controls LED
            cmd.color = led::Color::DARK_BLUE;
            cmd.state = led::State::SLOW_GLOW;
            break;
        }
        case SystemState::OUT_OF_MEMORY:
        {
            cmd.color = led::Color::PURPLE;
            cmd.state = led::State::SLOW_GLOW;
            break;
        }
        case SystemState::MEMORY_IS_CORRUPT:
        {
            cmd.color = led::Color::PURPLE;
            cmd.state = led::State::FAST_GLOW;
            break;
        }
        case SystemState::SHUTDOWN:
        {
            cmd.color = led::Color::WHITE;
            cmd.state = led::State::FAST_GLOW;
            break;
        }
    }

    xQueueSend(context->led_commands_handle, reinterpret_cast<void*>(&cmd), 0);
}

void set_memory_ownership(const MemoryOwner owner, Context& context)
{
    // At this point we should tell BLE to switch to access the real FS (not the simulated one). 
    // Operation result in this case doesn't really matter.
    ble::CommandQueueElement cmd{ble::Command::CONNECT_FS};
    xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), 0);

    xQueueReset(context.memory_status_handle);
    memory::CommandQueueElement command_to_memory{
        (owner == MemoryOwner::AUDIO) ? 
            memory::Command::SELECT_OWNER_AUDIO : 
            memory::Command::SELECT_OWNER_BLE,
        {0, 0}
    };
    const auto owner_switch_result = xQueueSend(
        context.memory_commands_handle, reinterpret_cast<void*>(&command_to_memory), 0);
    if(pdPASS != owner_switch_result)
    {
        NRF_LOG_ERROR("state: failed to request owner switch to %s", (owner == MemoryOwner::AUDIO) ? "audio" : "BLE");
    }
    memory::StatusQueueElement response;
    static constexpr uint32_t memory_ownership_switch_timeout{1000};
    const auto memory_response_result =
        xQueueReceive(context.memory_status_handle, reinterpret_cast<void*>(&response), memory_ownership_switch_timeout);
    if(pdPASS != memory_response_result ||
        response.status != decltype(response.status)::OK)
    {
        NRF_LOG_ERROR(
            "state: memory ownership switch ->%s has failed", (owner == MemoryOwner::AUDIO) ? "audio" : "BLE");
    }
}

} // namespace systemstate
