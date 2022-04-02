// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "ble_file_transfer_service.h"
#include "simple_fs.h"

namespace ble
{
namespace fts
{

ble_fts_t& get_fts_instance();
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

    inline bool is_transmission_complete()
    {
        return _state == State::DONE || _state == State::INVALID;
    }

private:
    State _state{State::INVALID};
    struct Context
    {
        filesystem::FilesCount files_count;
        filesystem::File file;
        size_t current_file_size;
        size_t transmitted_size;
        static const size_t READ_BUFFER_SIZE = 256;
        uint8_t read_buffer[READ_BUFFER_SIZE];
        uint32_t last_timestamp;
    };

    Context _context;
    result::Result send_data(Context& context, size_t size);

    static const uint16_t BLE_ITS_MAX_DATA_LEN = BLE_GATT_ATT_MTU_DEFAULT - 3;
};

} // namespace fts
} // namespace ble