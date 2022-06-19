#include "ble_fts_fsm.h"
#include <nrf_log.h>
#include "simple_fs.h"
#include "drv_audio.h"

const char* stateNames[] = {
        "INVALID",
        "IDLE",
        "FS_INFO_TRANSMISSION",
        "NEXT_FILE_INFO_TRANSMISSION",
        "FILE_TRANSMISSION_START",
        "FILE_TRANSMISSION_RUNNING",
        "FILE_TRANSMISSION_END",
        "DONE",
    };

/// Glue code elements, connecting standard Nordic C API and CPP code of this repo
BLE_FTS_DEF(m_fts, NRF_SDH_BLE_TOTAL_LINK_COUNT);
namespace ble
{
namespace fts
{
    
ble_fts_t& get_fts_instance() { return m_fts; }
extern "C" ble_fts_t * get_fts_instance_c() { return &m_fts; }

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
    const State prev_state{_state};
    switch (_state)
    {
        case State::INVALID:
        {
            // TODO: consider assertion here
            NRF_LOG_ERROR("fts fsm: invalid state");
            break;
        };
        case State::IDLE:
        {
            switch (command)
            {
                case CMD_GET_FILE:
                {
                    _state = State::FILE_TRANSMISSION_START;
                    break;
                }
                case CMD_GET_FILE_INFO:
                {
                    _state = State::NEXT_FILE_INFO_TRANSMISSION;
                    break;
                }
                case CMD_GET_VALID_FILES_COUNT:
                {
                    _state = State::FS_INFO_TRANSMISSION;
                    break;
                }
                default: break;
            }
            break;
        };
        case State::FS_INFO_TRANSMISSION:
        {
            _context.files_count = filesystem::get_files_count();
            NRF_LOG_DEBUG("Sending files' count: %d", _context.files_count.valid);

            ble_fts_filesystem_info_t fs_info;
            fs_info.valid_files_count = _context.files_count.valid;
            const auto res = ble_fts_filesystem_info_send(&m_fts, &fs_info);
            if (res != 0)
            {
                NRF_LOG_ERROR("fts filesystem info send has failed, error code %d", res);
            }
            else 
            {
              _state = State::IDLE;
              save_activity_timestamp();
            }

          
            break;
        }
        case State::NEXT_FILE_INFO_TRANSMISSION:
        {
            // This particular part of the logic is not elegant. I assume here that if 
            // the communication partner asks for next file size, it implies that we start 
            // the file transfer in the next communication iteration.
            bool is_next_file_found{false};
            while (!is_next_file_found)
            {
                const auto open_result = filesystem::open(_context.file, filesystem::FileMode::RDONLY);
                if (result::Result::OK != open_result)
                {
                    // TODO:
                    _state = State::INVALID;
                    NRF_LOG_ERROR("Failed to open next file for reading");
                    return;
                }
                if (_context.file.rom.size == 0)
                {
                    NRF_LOG_INFO("Empty file has been discovered, continue");
                    // We are not transferring empty files, it breaks the state machine. Close the file
                    const auto close_res = filesystem::close(_context.file);
                    if (close_res != result::Result::OK)
                    {
                        NRF_LOG_ERROR("Failed to close the file. FS invalidation might be required.");
                        _state = State::INVALID;
                        return;
                    }
                    continue;
                }
                _context.current_file_size = _context.file.rom.size;
                is_next_file_found = true;
            }

            ble_fts_file_info_t file_info;
            file_info.file_size_bytes = _context.current_file_size;
            const auto result = ble_fts_file_info_send(&m_fts, &file_info);
            NRF_LOG_DEBUG("Sending file info, size %d, result(%d)", _context.current_file_size, result);

            save_activity_timestamp();
            _state = State::IDLE;
            break;
        }
        case State::FILE_TRANSMISSION_START:
        {
            // By this time file should've been open. If not, we either can't enter this area or
            // the SM has been corrupt.
            if (!filesystem::is_file_open(_context.file))
            {
                NRF_LOG_ERROR("SM is corrupt.File transmission can't be started");
                _state = State::INVALID;
                break;
            }
            // Read first chunk of data out and apply the WAV header 
            size_t read_size{0U};
            const auto read_result = filesystem::read(_context.file, _context.read_buffer, Context::READ_BUFFER_SIZE, read_size);
            if (result::Result::OK != read_result)
            {
                NRF_LOG_ERROR("Failed to read data, file transmission can't be completed.");
                _state = State::INVALID;
                break;
            }

            // exceptional case when read_size at first run is less than WAV header is not considerd.
            drv_audio_wav_header_apply(_context.read_buffer, _context.current_file_size);
            send_data(_context, read_size);
            _context.transferred_data_size = read_size;
            
            save_activity_timestamp();
            _state = State::FILE_TRANSMISSION_RUNNING;
            break;
        }
        case State::FILE_TRANSMISSION_RUNNING:
        {
            // TODO: add file readout and transaction logic
            size_t read_size{0U};
            const auto read_result = filesystem::read(_context.file, _context.read_buffer, Context::READ_BUFFER_SIZE, read_size);
            if (result::Result::OK != read_result)
            {
                NRF_LOG_ERROR("Failed to read data, file transmission can't be completed.");
                _state = State::INVALID;
                break;
            }
            send_data(_context, read_size);
            _context.transferred_data_size += read_size; 
            // TODO: add progress bar 
            if (read_size != Context::READ_BUFFER_SIZE)
            {
                _state = State::FILE_TRANSMISSION_END;
            }
            save_activity_timestamp();
            break;
        }
        case State::FILE_TRANSMISSION_END:
        {
            // close the file
            const auto close_res = filesystem::close(_context.file);
            if (close_res != result::Result::OK)
            {
                NRF_LOG_ERROR("Failed to close the file. FS invalidation might be required.");
                _state = State::INVALID;
                break;
            }
            // send the last chunk of data
            
            _context.files_count = filesystem::get_files_count();
            NRF_LOG_INFO("files count: valid=%d, invalid=%d", _context.files_count.valid, _context.files_count.invalid);

            _state = (_context.files_count.valid > 0) ? State::IDLE : State::DONE;
            save_activity_timestamp();
            break;
        }
        break;
    }
    if (_state != prev_state)
    {
        NRF_LOG_INFO("%s->%s(%d)", stateNames[(int)prev_state], stateNames[(int)_state], command);
    }
}

result::Result FtsStateMachine::send_data(Context& context, const size_t size)
{
    if(size > 0)
    {
        unsigned i = 0;

        size_t size_left = size;
        uint8_t* send_buffer = (uint8_t*)context.read_buffer;

        while(size_left)
        {
            uint8_t send_size = MIN(size_left, BLE_ITS_MAX_DATA_LEN);
            size_left -= send_size;

            uint32_t err_code = NRF_SUCCESS;
            while(true)
            {
                err_code = ble_fts_send_file_fragment(&m_fts, send_buffer, send_size);
                if(err_code == NRF_SUCCESS)
                {
                    break;
                }
                else if(err_code != NRF_ERROR_RESOURCES)
                {
                    NRF_LOG_ERROR("Failed to send file, err = %d", err_code);
                    return result::Result::ERROR_GENERAL;
                }
            }

            send_buffer += send_size;
        }
    }
    return result::Result::OK;
}

} // namespace fts
} // namespace ble