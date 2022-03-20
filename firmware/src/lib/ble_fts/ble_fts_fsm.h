// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "ble_file_transfer_service.h"

namespace ble
{
namespace fts
{

class FtsStateMachine
{ 
public:
    FtsStateMachine();
    enum class State
    {
        INVALID,
        IDLE,
        FS_INFO_TRANSMISSION,
        NEXT_FILE_INFO_TRANSMISSION,
        FILE_TRANSMISSION_START,
        FILE_TRANSMISSION_RUNNING,
        FILE_TRANSMISSION_END,
        DONE,
    };
    void start();
    void stop();
    void process_command(BleCommands command);

    inline bool is_transmission_complete() { return _state == State::DONE; }
private:
    State _state{State::INVALID};
};

} // namespace fts
} // namespace ble