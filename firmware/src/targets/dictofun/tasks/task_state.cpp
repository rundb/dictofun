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
            _applicationState = AppSmState::RECORD;
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

CompletionStatus do_init()
{
    // pull LDO EN high
    // check crystals' operation
    return CompletionStatus::INVALID;
}

CompletionStatus do_prepare()
{
    // check if SPI flash is operational
    // initialize FS
    // check if microphone is present

    return CompletionStatus::INVALID;
}

CompletionStatus do_record()
{
    return CompletionStatus::INVALID;
}

CompletionStatus do_record_finalize()
{
    return CompletionStatus::INVALID;
}

CompletionStatus do_connect()
{
    return CompletionStatus::INVALID;
}

CompletionStatus do_transfer()
{
    return CompletionStatus::INVALID;
}

CompletionStatus do_disconnect()
{
    return CompletionStatus::INVALID;
}

CompletionStatus do_finalize()
{
    return CompletionStatus::INVALID;
}

CompletionStatus do_restart()
{
    return CompletionStatus::INVALID;
}

CompletionStatus do_shutdown()
{
    return CompletionStatus::INVALID;
}

} // namespace application
