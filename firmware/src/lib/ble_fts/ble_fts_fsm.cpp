#include "ble_fts_fsm.h"

namespace ble
{
namespace fts
{

FtsStateMachine::FtsStateMachine() { }

void FtsStateMachine::start()
{
    _state = State::IDLE;
}

void FtsStateMachine::stop()
{
    _state = State::INVALID;
}

void FtsStateMachine::process_command(const BleCommands command) 
{ 
    switch (_state)
    {
        case State::INVALID:
        case State::IDLE:
        case State::FS_INFO_TRANSMISSION:
        case State::NEXT_FILE_INFO_TRANSMISSION:
        case State::FILE_TRANSMISSION_START:
        case State::FILE_TRANSMISSION_RUNNING:
        case State::FILE_TRANSMISSION_END:
        break;
    }
}

} // namespace fts
} // namespace ble