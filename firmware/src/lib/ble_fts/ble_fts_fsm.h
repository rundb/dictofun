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
    typedef uint32_t (*TimestampFunction)();

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

    void set_timestamp_function(TimestampFunction timestamp_function) { _timestamp_function = timestamp_function; } 

    inline bool is_transmission_complete()
    {
        return _state == State::DONE;
    }

    uint32_t get_time_since_last_activity() 
    {
        if (nullptr == _timestamp_function)
        {
            return 0;
        }
        return _timestamp_function() - _last_activity_timestamp;
    }

    void save_activity_timestamp() 
    {
      if (nullptr == _timestamp_function)
      {
          return;
      }
      _last_activity_timestamp = _timestamp_function();
    }

    struct ProgressReportData {
      size_t files_left_count;
      size_t current_file_size;
      size_t transferred_data_size;
    };

    ProgressReportData get_progress_report_data() {
      _progress_report_data.files_left_count = _context.files_count.valid;
      _progress_report_data.current_file_size = _context.current_file_size;
      _progress_report_data.transferred_data_size = _context.transferred_data_size;

      return _progress_report_data;
    }
private:
    State _state{State::INVALID};
    TimestampFunction _timestamp_function{nullptr};
    uint32_t _last_activity_timestamp{0};
    ProgressReportData _progress_report_data{0,0,0};
    struct Context
    {
        filesystem::FilesCount files_count;
        filesystem::File file;
        size_t current_file_size;
        size_t transferred_data_size;
        static const size_t READ_BUFFER_SIZE = 256;
        uint8_t read_buffer[READ_BUFFER_SIZE];
    };

    Context _context;
    result::Result send_data(Context& context, size_t size);

    static const uint16_t BLE_ITS_MAX_DATA_LEN = BLE_GATT_ATT_MTU_DEFAULT - 3;

    struct CommandSequenceTimestamps
    {
        uint32_t last_get_fs_info_timestamp{INVALID_TIMESTAMP};
        uint32_t last_get_file_info_timestamp{INVALID_TIMESTAMP};
        bool is_command_sequence_malfunction_detection_active{false};
        bool is_last_command_updated{false};
        BleCommands last_command{CMD_EMPTY};
        static const uint32_t INVALID_TIMESTAMP{0xFEDEEDEFUL};
        static const uint32_t INVALID_STATE_THRESHOLD_MS{2000};
    };
    CommandSequenceTimestamps _malfunction_detection_context;
    bool detect_command_sequence_malfunction();
};

} // namespace fts
} // namespace ble