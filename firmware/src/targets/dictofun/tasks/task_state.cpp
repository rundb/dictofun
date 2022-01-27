/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tasks/task_state.h"
#include "BleSystem.h"
#include "tasks/task_audio.h"
#include "tasks/task_led.h"
#include "tasks/task_memory.h"
#include <libraries/timer/app_timer.h>
#include <nrf_gpio.h>
#include <nrf_log.h>
#include "block_device_api.h"
#include "spi_flash.h"

namespace application
{
AppSmState _applicationState{AppSmState::INIT};

AppSmState getApplicationState()
{
    return _applicationState;
}

bool isRecordButtonPressed()
{
    return nrf_gpio_pin_read(BUTTON_PIN) > 0;
}

lfs_t * _fs{nullptr};
void application_init(lfs_t * fs)
{
    _fs = fs;
}

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
    }
    break;
    case AppSmState::RECORD_START: {
        const auto res = do_record_start();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::RECORD;
        }
    }
    break;
    case AppSmState::RECORD: {
        const auto res = do_record();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::RECORD_FINALIZATION;
        }
    }
    break;
    case AppSmState::RECORD_FINALIZATION: {
        const auto res = do_record_finalize();
        if(CompletionStatus::DONE == res)
        {
            _applicationState = AppSmState::CONNECT;
        }
        else if (CompletionStatus::ERROR == res)
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
        else if(CompletionStatus::TIMEOUT == res)
        {
            _applicationState = AppSmState::FINALIZE;
        }
        else if(CompletionStatus::RESTART_DETECTED == res)
        {
            _applicationState = AppSmState::RESTART;
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
        else if(CompletionStatus::DONE == res)
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
        NRF_LOG_INFO("state %d->%d", prevState, _applicationState);
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
};

/**
 * This context is used inside each of the do_<> functions in order to distinguish
 * first, last and other executions.
 */
FsmContext _context{InternalFsmState::DONE};

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

    if (_context.state == InternalFsmState::DONE)
    {
        _context.state == InternalFsmState::RUNNING;
        // mount the filesystem
        int err = lfs_mount(_fs, &lfs_configuration);

        // reformat if we can't mount the filesystem
        // this should only happen on the first boot
        if (err) {
            const auto format_res = lfs_format(_fs, &lfs_configuration);
            const auto mount_res = lfs_mount(_fs, &lfs_configuration);
            if (format_res || mount_res)
            {
                // TODO: specify this error handling
                NRF_LOG_ERROR("FS mounting has failed");
                auto& flashmem = flash::SpiFlash::getInstance();
                flashmem.eraseChip();
                _context.state == InternalFsmState::RUNNING;

                return CompletionStatus::PENDING;
            }
        } 
        _context.state == InternalFsmState::DONE;
        return CompletionStatus::DONE;
    }
    else if (_context.state == InternalFsmState::RUNNING)
    {
        auto& flashmem = flash::SpiFlash::getInstance();
        if (!flashmem.isBusy())
        {
            _context.state = InternalFsmState::DONE;   
            NRF_LOG_ERROR("Chip erase had to be called. Performing no record, just finalizing");
            return CompletionStatus::ERROR;
        }
        return CompletionStatus::PENDING;
    }

    _context.state = InternalFsmState::DONE;
    return CompletionStatus::DONE;
}

CompletionStatus do_record_start()
{
    // trigger start of FS operations
    // trigger start of PDM
    audio_start_record();

    led::task_led_set_indication_state(led::RECORDING);

    return CompletionStatus::DONE;
}

CompletionStatus do_record()
{
    // basically all operations are done in interrupts.
    // here we only have to track button status and handle operation errors
    const auto isButtonPressed = isRecordButtonPressed();
    if (!isButtonPressed)
    {
        // TODO: maybe a good idea to implement a slightly delayed end of writing
        audio_stop_record();
        return CompletionStatus::DONE;
    }

    return CompletionStatus::PENDING;
}

CompletionStatus do_record_finalize()
{
    // close the audio file
    // update the FS
    const auto record_size = audio_get_record_size();
    NRF_LOG_INFO("Recorded %d bytes", record_size);
    if (0U == record_size)
    {
        return CompletionStatus::ERROR;
    }

    return CompletionStatus::DONE;
}

CompletionStatus do_connect()
{
    // start advertising, establish connection to the phone
    if (!ble::BleSystem::getInstance().isActive())
    {
        ble::BleSystem::getInstance().start();
        const auto record_size = audio_get_record_size();
        ble::BleSystem::getInstance().getServices().setFileSizeForTransfer(record_size);

        led::task_led_set_indication_state(led::CONNECTING);
    }

    if (ble::BleSystem::getInstance().getConnectionHandle() != BLE_CONN_HANDLE_INVALID)
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
    if (ble::BleSystem::getInstance().getServices().isFileTransmissionComplete())
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
    if (_context.state == InternalFsmState::DONE)
    {
        _context.state = InternalFsmState::RUNNING;
        led::task_led_set_indication_state(led::SHUTTING_DOWN);
        NRF_LOG_INFO("Finalize: formatting the FS");
        const auto umount_res = lfs_unmount(_fs);
        const auto format_res = lfs_format(_fs, &lfs_configuration);
        if (format_res)
        {
            NRF_LOG_ERROR("Finalize: formatting has failed. Triggering chip erase.");
            auto& flashmem = flash::SpiFlash::getInstance();
            flashmem.eraseChip();
            return CompletionStatus::PENDING;
        }
        
        return CompletionStatus::DONE;
    }
    else if (_context.state == InternalFsmState::RUNNING)
    {
        auto& flashmem = flash::SpiFlash::getInstance();
        if (!flashmem.isBusy())
        {
            NRF_LOG_INFO("Finalize: erase done");
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
    // pull LDO_EN down
    nrf_gpio_pin_clear(LDO_EN_PIN);
    return CompletionStatus::DONE;
}

} // namespace application
