// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "task_state.h"
#include "BleSystem.h"
#include "block_device_api.h"
#include "spi_flash.h"

#include <app_timer.h>
#include <nrf_gpio.h>
#include <nrf_log.h>
#include "battery_measurement.h"

#include <FreeRTOS.h>
#include <task.h>

namespace systemstate
{
void task_system_state(void *)
{
    NRF_LOG_INFO("task state: initialized");
    while(1)
    {
        vTaskDelay(10);
    }
}   
}

namespace application
{
AppSmState _applicationState{AppSmState::INIT};
filesystem::File _currentFile;
static const int SHUTDOWN_COUNTER_INIT_VALUE = 100;

battery::BatteryMeasurement _batteryMeasurement(2U);

const char* stateNames[] = {"INIT",
                            "PREPARE",
                            "RECORD_START",
                            "RECORD",
                            "RECORD_FINALIZATION",
                            "CONNECT",
                            "TRANSFER",
                            "DISCONNECT",
                            "FINALIZE",
                            "SHUTDOWN",
                            "RESTART"};

AppSmState getApplicationState()
{
    return _applicationState;
}

bool isRecordButtonPressed()
{
    return nrf_gpio_pin_read(BUTTON_PIN) > 0;
}

void application_init() { }

/**
 * Enum is defined within CPP file, as it's used only inside the implementation
 */
enum class CompletionStatus
{
    INVALID,
    PENDING,
    DONE,
    TIMEOUT,
    RESTART_DETECTED,
    ERROR
};

enum class InternalFsmState
{
    INIT,
    RUNNING,
    DONE
};

struct FsmContext
{
    InternalFsmState state;
    bool is_unrecoverable_error_detected;
    int counter;
    uint32_t start_timestamp;
    uint32_t last_file_transmission_report_timestamp;
};

// connection timeout in milliseconds (60 seconds)
static const uint32_t CONNECT_STATE_TIMEOUT = 30000;

// transfer timeout in milliseconds (1200 seconds)
static const uint32_t TRANSFER_STATE_TIMEOUT = 1200000;

static const uint32_t REPORT_TRANSMISSION_PROGRESS_TIMEOUT = 10000;

// activity timeout in milliseconds (120 seconds)
static const uint32_t TRANSFER_STATE_INACTIVITY_TIMEOUT = 120000;
/**
 * This context is used inside each of the do_<> functions in order to distinguish
 * first, last and other executions.
 */
FsmContext _context{InternalFsmState::DONE, false, 0, 0, 0};

/**
 * FSM action steps. 
 * If operation is not finished before the exit of the function, PENDING is returned.
 * If operation has been finished - DONE is returned.
 * If operation has timed out (states CONNECT and TRANSFER) - TIMEOUT is returned.
 * If a button press has been detected during BLE phases - RESTART_DETECTED is returned.
 * If an error that doesn't allow to continue to work is detected - ERROR is detected (f.e.
 *      crystal(s) haven't started, SPI Flash is absent, microphone is not working)
 */
CompletionStatus do_init();
CompletionStatus do_prepare();
CompletionStatus do_record_start();
CompletionStatus do_record();
CompletionStatus do_record_finalize();
CompletionStatus do_connect();
CompletionStatus do_transfer();
CompletionStatus do_disconnect();
CompletionStatus do_finalize();
CompletionStatus do_restart();
CompletionStatus do_shutdown();

/**
 * Cyclic function contains only the logic related to the FSM steps. 
 * Contents of the steps are extracted into do_<something> functions, to separate
 * FSM logic and step-related logic. 
 */
void application_cyclic()
{
    const auto timestamp = app_timer_cnt_get();
    const auto prevState = _applicationState;
    switch(_applicationState)
    {
    case AppSmState::INIT: {
        const auto res = do_init();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::PREPARE;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    case AppSmState::PREPARE: {
        const auto res = do_prepare();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::RECORD_START;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    case AppSmState::RECORD_START: {
        const auto res = do_record_start();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::RECORD;
        }
        else  if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    case AppSmState::RECORD: {
        const auto res = do_record();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::RECORD_FINALIZATION;
        }
        else if(CompletionStatus::ERROR == res)
        {
            _applicationState = AppSmState::FINALIZE;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    case AppSmState::RECORD_FINALIZATION: {
        const auto res = do_record_finalize();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::CONNECT;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    case AppSmState::CONNECT: {
        const auto res = do_connect();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::TRANSFER;
        }
        else if(CompletionStatus::RESTART_DETECTED == res)
        {
            _applicationState = AppSmState::RESTART;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    case AppSmState::TRANSFER: {
        const auto res = do_transfer();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::DISCONNECT;
        }
        else if(CompletionStatus::TIMEOUT == res)
        {
            _applicationState = AppSmState::FINALIZE;
        }
        else if(CompletionStatus::RESTART_DETECTED == res)
        {
            _applicationState = AppSmState::RESTART;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    case AppSmState::DISCONNECT: {
        const auto res = do_disconnect();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::FINALIZE;
        }
        else if(CompletionStatus::RESTART_DETECTED == res)
        {
            _applicationState = AppSmState::RESTART;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::FINALIZE;
        }        
    }
    break;
    case AppSmState::FINALIZE: {
        const auto res = do_finalize();
        if(CompletionStatus::RESTART_DETECTED == res)
        {
            _applicationState = AppSmState::RESTART;
        }
        else if(CompletionStatus::DONE == res || CompletionStatus::ERROR == res)
        {
            _applicationState = AppSmState::SHUTDOWN;
        }
        else if (CompletionStatus::PENDING == res)
        {
            // do nothing, wait
        }
        else
        {
            _applicationState = AppSmState::SHUTDOWN;
        }
    }
    break;
    case AppSmState::SHUTDOWN: {
        do_shutdown();
    }
    break;
    case AppSmState::RESTART: {
        const auto res = do_restart();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::PREPARE;
        }
        else
        {
            // by default restart is not supported, proceed directly to finalize
            //_applicationState = AppSmState::FINALIZE;
        }
    }
    break;
    }
    if(prevState != _applicationState)
    {
        // TODO: log state change
        const int idx1 = static_cast<int>(prevState);
        const int idx2 = static_cast<int>(_applicationState);
        NRF_LOG_INFO("state %s->%s", stateNames[idx1], stateNames[idx2]);
        _context.start_timestamp = app_timer_cnt_get();
    }
}

CompletionStatus do_init()
{
    // pull LDO EN high
    nrf_gpio_pin_set(LDO_EN_PIN);

    // led::task_led_set_indication_state(led::PREPARING);

    return CompletionStatus::DONE;
}

CompletionStatus do_prepare()
{
    // TODO: check if microphone is present

    if(_context.state == InternalFsmState::DONE)
    {
        if (!_batteryMeasurement.isInitialized())
        {
            _batteryMeasurement.init();
            _batteryMeasurement.start();
        }

        volatile auto fs_init_res = filesystem::init(integration::spi_flash_simple_fs_config);
        // mount the filesystem
        if(fs_init_res != result::Result::OK)
        {
            _context.state = InternalFsmState::DONE;
            _context.is_unrecoverable_error_detected = true;
            return CompletionStatus::ERROR;
        }
        const auto file_open_res = filesystem::create(_currentFile);
        if(file_open_res != result::Result::OK)
        {
            _context.state = InternalFsmState::DONE;
            _context.is_unrecoverable_error_detected = true;
            return CompletionStatus::ERROR;
        }

        _context.state = InternalFsmState::RUNNING;
        return CompletionStatus::PENDING;
    }
    else if(_context.state == InternalFsmState::RUNNING)
    {
        if (!_batteryMeasurement.isBusy())
        {
            _context.state = InternalFsmState::DONE;
            if (_batteryMeasurement.voltage() < battery::BatteryMeasurement::MINIMAL_OPERATIONAL_VOLTAGE)
            {
                NRF_LOG_ERROR("Battery low (%dmV)", static_cast<int>(_batteryMeasurement.voltage() * 1000));
                return CompletionStatus::ERROR;
            }
            return CompletionStatus::DONE;
        }
        return CompletionStatus::PENDING;
    }

    _context.state = InternalFsmState::RUNNING;
    return CompletionStatus::PENDING;
}

CompletionStatus do_record_start()
{
    // trigger start of PDM
    // audio::audio_start_record(_currentFile);

    NRF_LOG_INFO("Record: saving data into a file");
    // led::task_led_set_indication_state(led::RECORDING);

    return CompletionStatus::DONE;
}

CompletionStatus do_record()
{
    // basically all operations are done in interrupts.
    // here we only have to track button status and handle operation errors
    const auto isButtonPressed = isRecordButtonPressed();
    if(!isButtonPressed)
    {
        // const auto result = audio::audio_stop_record();
        // if(result == result::Result::OK)
        // {
        //     return CompletionStatus::DONE;
        // }
        _context.is_unrecoverable_error_detected = true;
        _context.state = InternalFsmState::DONE;
        return CompletionStatus::ERROR;
    }

    return CompletionStatus::PENDING;
}

CompletionStatus do_record_finalize()
{
    if (_context.state == InternalFsmState::DONE)
    {
        // close the audio file
        const auto file_size = _currentFile.ram.size;
        const auto close_res = filesystem::close(_currentFile);
        if(close_res != result::Result::OK)
        {
            NRF_LOG_ERROR("Record finalize: failed to close the file");
            _context.is_unrecoverable_error_detected = true;
            _context.state = InternalFsmState::DONE;
            return CompletionStatus::ERROR;
        }
        _context.state = InternalFsmState::RUNNING;
    }
    else if (_context.state == InternalFsmState::RUNNING)
    {
        if (_batteryMeasurement.isBusy())
        {
            return CompletionStatus::PENDING;
        }
        else
        {
            _context.state = InternalFsmState::DONE;
            return CompletionStatus::DONE;        
        }
    }
    
    return CompletionStatus::DONE;
}

CompletionStatus do_connect()
{
    // Before connecting, perform integrity check for the filesystem
    // Figure out how many files we have to be transmitted
    filesystem::FilesCount files_count;
    const auto count_result = filesystem::get_files_count(files_count);

    if (result::Result::OK != count_result)
    {
        _context.is_unrecoverable_error_detected = true;
        _context.state = InternalFsmState::DONE;
        return CompletionStatus::ERROR;
    }

    // Find first file bigger than 0 bytes and invalidate all empty files before it.
    int file_size = 0;
    do 
    {
        const auto open_result = filesystem::open(_currentFile);
        if(open_result != result::Result::OK)
        {
            NRF_LOG_ERROR("task_state: file opening failure (%d)", open_result);
            _context.is_unrecoverable_error_detected = true;
            _context.state = InternalFsmState::DONE;
            return CompletionStatus::ERROR;
        }
        file_size = _currentFile.rom.size;
        if (file_size == 0)
        {
            filesystem::invalidate(_currentFile);
        }
        else
        {
            // close first not-empty file without an invalidation. 
            filesystem::close(_currentFile);
        }
    } while (file_size == 0);

    // start advertising, establish connection to the phone
    if(!ble::BleSystem::getInstance().isActive())
    {
        ble::BleSystem::getInstance().start();

        // led::task_led_set_indication_state(led::CONNECTING);

    }

    if(ble::BleSystem::getInstance().getConnectionHandle() != BLE_CONN_HANDLE_INVALID)
    {
        ble::BleSystem::getInstance().getServices().setFtsTimestampFunction(app_timer_cnt_get);
        return CompletionStatus::DONE;
    }

    const auto isButtonPressed = isRecordButtonPressed();
    if (isButtonPressed)
    {
        ble::BleSystem::getInstance().stop();
        _context.state = InternalFsmState::DONE;
        return CompletionStatus::RESTART_DETECTED;
    }
    
    const auto timestamp = app_timer_cnt_get();
    if (timestamp - _context.start_timestamp > CONNECT_STATE_TIMEOUT)
    {
        NRF_LOG_ERROR("Connection timeout error.");
        return CompletionStatus::TIMEOUT;
    }

    return CompletionStatus::PENDING;
}

CompletionStatus do_transfer()
{
    // led::task_led_set_indication_state(led::SENDING);

    // transfer the data to the phone
    if(ble::BleSystem::getInstance().getServices().isFileTransmissionComplete())
    {
        _context.state = InternalFsmState::DONE;
        return CompletionStatus::DONE;
    }

    const auto isButtonPressed = isRecordButtonPressed();
    if (isButtonPressed)
    {
        ble::BleSystem::getInstance().stop();
        _context.state = InternalFsmState::DONE;
        return CompletionStatus::RESTART_DETECTED;
    }

    // TODO: add transmission error detection (or incapsulate it in FTS)
    const auto timestamp = app_timer_cnt_get();
    if ((timestamp - _context.start_timestamp > TRANSFER_STATE_TIMEOUT) || 
        (ble::BleSystem::getInstance().getServices().getTimeSinceLastFtsActivity() > TRANSFER_STATE_INACTIVITY_TIMEOUT))
    {
        NRF_LOG_ERROR("Transfer timeout error.");
        return CompletionStatus::TIMEOUT;
    }

    if (timestamp - _context.last_file_transmission_report_timestamp > REPORT_TRANSMISSION_PROGRESS_TIMEOUT)
    {
        _context.last_file_transmission_report_timestamp = timestamp;
        const auto report = ble::BleSystem::getInstance().getServices().getProgressReportData();
        if (report.files_left_count == 0U)
        {
            // This is a sign of inconsistency in the state machine operation on the phone's side.
            // It's safe to say that at this stage transmission won't happen, so timeout can be reported
            NRF_LOG_ERROR("Phone has failed to initiate transfer. Timeout error, causing a shutdown.");
            return CompletionStatus::TIMEOUT;
        }

        NRF_LOG_INFO("progress: %d, %d %", report.files_left_count, (report.transferred_data_size * 100)/(report.current_file_size+1));
    }

    return CompletionStatus::PENDING;
}

CompletionStatus do_disconnect()
{
    // TODO: close the BLE connection
    ble::BleSystem::getInstance().disconnect();
    _context.state = InternalFsmState::DONE;
    return CompletionStatus::DONE;
}

CompletionStatus do_finalize()
{

    // finalize Flash memory operations
    // if files have been succesfully transmitted - erase the memory and FS descriptos
    // if not - update FS descriptors
    if(_context.state == InternalFsmState::DONE)
    {
        _context.state = InternalFsmState::RUNNING;
        auto& flashmem = flash::SpiFlash::getInstance();
        if(!_context.is_unrecoverable_error_detected)
        {
            // led::task_led_set_indication_state(led::SHUTTING_DOWN);
            NRF_LOG_INFO("Finalize: unmounting the FS");
            filesystem::FilesCount files_stats;
            const auto files_stats_result = filesystem::get_files_count(files_stats);
            const auto occupied_mem_size = filesystem::get_occupied_memory_size();
            const auto total_size = integration::MEMORY_VOLUME;
            NRF_LOG_INFO("File stats: valid files: %d, invalid files: %d, memory occupied: %d/%d",
                         files_stats.valid,
                         files_stats.invalid,
                         occupied_mem_size,
                         total_size);
            if(occupied_mem_size * 100 / total_size > 80)
            {
                NRF_LOG_INFO("Finalize: memory is occupied by 80%. Formatting");
                flashmem.eraseChip();
                return CompletionStatus::PENDING;
            }
        }
        else
        {
            NRF_LOG_INFO("Finalize: unrecoverable error detected. Erasing.");
            flashmem.eraseChip();
            return CompletionStatus::PENDING;
        }
        _context.state = InternalFsmState::DONE;
        return CompletionStatus::DONE;
    }
    else if(_context.state == InternalFsmState::RUNNING)
    {
        auto& flashmem = flash::SpiFlash::getInstance();
        if(!flashmem.isBusy())
        {
            NRF_LOG_INFO("Finalize: erase done.");
            _context.state = InternalFsmState::DONE;
            return CompletionStatus::DONE;
        }
        return CompletionStatus::PENDING;
    }

    return CompletionStatus::INVALID;
}

CompletionStatus do_restart()
{
    // handle the previous state of the operation, assure consistency of FS and
    // trigger rec_start ASAP
    filesystem::close(_currentFile);
    filesystem::deinit();
    _context.state = InternalFsmState::DONE;

    return CompletionStatus::DONE;
}

CompletionStatus do_shutdown()
{
    if(_context.state == InternalFsmState::DONE)
    {
        // led::task_led_set_indication_state(led::INDICATION_OFF);
        // start finalization preparation.
        // downcount ~100-200 ms to let logger print data
        _context.counter = SHUTDOWN_COUNTER_INIT_VALUE;
        _context.state = InternalFsmState::RUNNING;
    }
    else if(_context.state == InternalFsmState::RUNNING)
    {
        if(--_context.counter == 0)
        {
            // pull LDO_EN down
            nrf_gpio_pin_clear(LDO_EN_PIN);
            while(1)
                ;
        }
    }
    //while(1);
    return CompletionStatus::DONE;
}

} // namespace application
