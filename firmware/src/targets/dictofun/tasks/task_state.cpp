// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "tasks/task_state.h"
#include "BleSystem.h"
#include "block_device_api.h"
#include "spi_flash.h"
#include "tasks/task_audio.h"
#include "tasks/task_led.h"
#include "tasks/task_memory.h"
#include <libraries/timer/app_timer.h>
#include <nrf_gpio.h>
#include <nrf_log.h>
#include "block_device_api.h"

namespace application
{
AppSmState _applicationState{AppSmState::INIT};
filesystem::File _currentFile;
static const int SHUTDOWN_COUNTER_INIT_VALUE = 100;

const char *stateNames[] =
{
    "INIT",
    "PREPARE",
    "RECORD_START",
    "RECORD",
    "RECORD_FINALIZATION",
    "CONNECT",
    "TRANSFER",
    "DISCONNECT",
    "FINALIZE",
    "SHUTDOWN",
    "RESTART"
};

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
    }
    break;
    case AppSmState::PREPARE: {
        const auto res = do_prepare();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::RECORD_START;
        }
        else //if (CompletionStatus::ERROR == res)
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
        else //if (CompletionStatus::ERROR == res)
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
        else if (CompletionStatus::ERROR == res)
        {
            _applicationState = AppSmState::FINALIZE;
        }
        else
        {
          // do nothing
        }
    }
    break;
    case AppSmState::RECORD_FINALIZATION: {
        const auto res = do_record_finalize();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::CONNECT;
        }
        else if(CompletionStatus::ERROR == res)
        {
            _applicationState = AppSmState::FINALIZE;
        }
        else if(CompletionStatus::INVALID == res)
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
        else if (CompletionStatus::ERROR == res)
        {
            _applicationState = AppSmState::FINALIZE;
        }
        else
        {
            // pending - do nothing 
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
        else if (CompletionStatus::ERROR == res)
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
            _applicationState = AppSmState::RECORD_START;
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
    }
}

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
};

/**
 * This context is used inside each of the do_<> functions in order to distinguish
 * first, last and other executions.
 */
FsmContext _context{InternalFsmState::DONE, false, 0};

CompletionStatus do_init()
{
    // pull LDO EN high
    nrf_gpio_pin_set(LDO_EN_PIN);

    led::task_led_set_indication_state(led::PREPARING);

    // TODO: check that crystals are operating

    return CompletionStatus::DONE;
}

CompletionStatus do_prepare()
{
    // TODO: check if SPI flash is operational
    // TODO: check if microphone is present

    if(_context.state == InternalFsmState::DONE)
    {
        volatile auto fs_init_res = filesystem::init(integration::spi_flash_simple_fs_config);
        _context.state == InternalFsmState::RUNNING;
        // mount the filesystem
        if (fs_init_res != result::Result::OK)
        {
            _context.state = InternalFsmState::DONE;
            _context.is_unrecoverable_error_detected = true;
            return CompletionStatus::ERROR;
        }
        const auto file_open_res = filesystem::open(_currentFile, filesystem::FileMode::WRONLY);
        if (file_open_res != result::Result::OK)
        {
            _context.state = InternalFsmState::DONE;
            _context.is_unrecoverable_error_detected = true;
            return CompletionStatus::ERROR;
        }

        _context.state == InternalFsmState::DONE;
        return CompletionStatus::DONE;
    }
    else if(_context.state == InternalFsmState::RUNNING)
    {
        return CompletionStatus::DONE;
    }

    _context.state = InternalFsmState::DONE;
    return CompletionStatus::DONE;
}

CompletionStatus do_record_start()
{
    // trigger start of PDM
    audio_start_record(_currentFile);

    NRF_LOG_INFO("Record: saving data into a file");
    led::task_led_set_indication_state(led::RECORDING);

    return CompletionStatus::DONE;
}

CompletionStatus do_record()
{
    // basically all operations are done in interrupts.
    // here we only have to track button status and handle operation errors
    const auto isButtonPressed = isRecordButtonPressed();
    if(!isButtonPressed)
    {
        const auto result = audio_stop_record();
        if (result == result::Result::OK)
        {
            return CompletionStatus::DONE;
        }
        _context.is_unrecoverable_error_detected = true;
        _context.state = InternalFsmState::DONE;
        return CompletionStatus::ERROR;
    }

    return CompletionStatus::PENDING;
}

CompletionStatus do_record_finalize()
{
    // close the audio file
    const auto file_size = _currentFile.ram.size;
    const auto close_res = filesystem::close(_currentFile);
    if (close_res != result::Result::OK)
    {
        NRF_LOG_ERROR("Record finalize: failed to close the file");
        _context.is_unrecoverable_error_detected = true;
        return CompletionStatus::ERROR;
    }
    if (file_size == 0)
    {
        return CompletionStatus::INVALID;
    }

    return CompletionStatus::DONE;
}

CompletionStatus do_connect()
{
    // start advertising, establish connection to the phone
    if(!ble::BleSystem::getInstance().isActive())
    {
        ble::BleSystem::getInstance().start();

        led::task_led_set_indication_state(led::CONNECTING);
    }

    if(ble::BleSystem::getInstance().getConnectionHandle() != BLE_CONN_HANDLE_INVALID)
    {
        return CompletionStatus::DONE;
    }

    // TODO: add restart detection
    // TODO: add timeout handler

    return CompletionStatus::PENDING;
}

CompletionStatus do_transfer()
{
    led::task_led_set_indication_state(led::SENDING);

    // transfer the data to the phone
    if(ble::BleSystem::getInstance().getServices().isFileTransmissionComplete())
    {
        return CompletionStatus::DONE;
    }

    // TODO: add restart detection
    // TODO: add transmission error detection (or incapsulate it in FTS)

    return CompletionStatus::PENDING;
}

CompletionStatus do_disconnect()
{
    // TODO: close the BLE connection
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
        if (!_context.is_unrecoverable_error_detected)
        {
            led::task_led_set_indication_state(led::SHUTTING_DOWN);
            NRF_LOG_INFO("Finalize: unmounting the FS");
            const auto files_stats = filesystem::get_files_count();
            const auto occupied_mem_size = filesystem::get_occupied_memory_size();
            const auto total_size = integration::MEMORY_VOLUME;
            NRF_LOG_INFO("File stats: valid files: %d, invalid files: %d, memory occupied: %d/%d", files_stats.valid, files_stats.invalid, occupied_mem_size, total_size);
            if (occupied_mem_size * 100 / total_size > 80)
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
    return CompletionStatus::INVALID;
}

CompletionStatus do_shutdown()
{
    if(_context.state == InternalFsmState::DONE)
    {
        led::task_led_set_indication_state(led::INDICATION_OFF);
        // start finalization preparation.
        // downcount ~100-200 ms to let logger print data
        _context.counter = SHUTDOWN_COUNTER_INIT_VALUE;
        _context.state = InternalFsmState::RUNNING;
    }
    else if (_context.state == InternalFsmState::RUNNING)
    {
        if (--_context.counter == 0)
        {
            // pull LDO_EN down
            nrf_gpio_pin_clear(LDO_EN_PIN);
            while(1);
        }
    }
    //while(1);
    return CompletionStatus::DONE;
}

} // namespace application
