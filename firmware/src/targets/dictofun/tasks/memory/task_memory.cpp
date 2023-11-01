// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_memory.h"
#include "nrf_log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "block_api.h"
#include "boards.h"
#include "littlefs_access.h"
#include "spi_flash.h"
#include <cstdio>
#include <cstdlib>
// TODO: this dependency here is really bad

#include "codec_decimate.h"
#include "task_audio.h"
#include "task_ble.h"
#include "task_rtc.h"

#include "time_profiler.h"

#include "lfs.h"

namespace memory
{

spi::Spi flash_spi(0, SPI_FLASH_CS_PIN);

static const spi::Spi::Configuration flash_spi_config{NRF_DRV_SPI_FREQ_8M,
                                                      NRF_DRV_SPI_MODE_0,
                                                      NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
                                                      SPI_FLASH_SCK_PIN,
                                                      SPI_FLASH_MOSI_PIN,
                                                      SPI_FLASH_MISO_PIN};

flash::SpiFlash flash{flash_spi, vTaskDelay, xTaskGetTickCount};
constexpr uint32_t flash_page_size{256};
constexpr uint32_t flash_sector_size{4096};
constexpr uint32_t flash_total_size{16 * 1024 * 1024 - 4096};
constexpr uint32_t flash_sectors_count{flash_total_size / flash_sector_size};

constexpr size_t CACHE_SIZE{flash_page_size};
constexpr size_t LOOKAHEAD_BUFFER_SIZE{64};
uint8_t prog_buffer[CACHE_SIZE];
uint8_t read_buffer[CACHE_SIZE];
uint8_t lookahead_buffer[CACHE_SIZE];

enum class MemoryOwner
{
    AUDIO,
    BLE,
};
static MemoryOwner _memory_owner{MemoryOwner::AUDIO};

lfs_t lfs;

const struct lfs_config lfs_configuration = {
    .read = memory::block_device::read,
    .prog = memory::block_device::program,
    .erase = memory::block_device::erase,
    .sync = memory::block_device::sync,

    // block device configuration
    .read_size = 16,
    .prog_size = flash_page_size,
    .block_size = flash_sector_size,
    .block_count = flash_sectors_count,
    .block_cycles = 500,
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_BUFFER_SIZE,
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

constexpr uint32_t cmd_wait_idle_ticks{5};
constexpr uint32_t cmd_wait_fast_ticks{1};
constexpr uint32_t ble_command_wait_ticks{5};
constexpr uint32_t data_send_wait_ticks{10};
constexpr uint32_t audio_data_wait_ticks{5};

constexpr uint32_t file_name_size_bytes{16UL};

// This element allocated statically, as it's rather big (~260 bytes)
ble::FileDataFromMemoryQueueElement data_queue_elem;

// Single audio sample from audio module
audio::CodecOutputType audio_data_queue_element;

static struct FileOperationContext
{
    ble::fts::file_id_type file_id{0};
    bool is_file_open{false};
} _file_operation_context;

char active_record_name[sizeof(ble::fts::file_id_type) + 1]{0};

static uint32_t written_record_size{0};

static bool is_ble_access_allowed();
void process_request_from_ble(Context& context,
                              ble::CommandToMemory command_id,
                              ble::fts::file_id_type file_id);
void process_request_from_state(Context& context, Command command_id, uint32_t arg0 = 0, uint32_t arg1 = 0);

void task_memory(void* context_ptr)
{
    NRF_LOG_DEBUG("task memory: initialized");
    Context& context = *(reinterpret_cast<Context*>(context_ptr));
    CommandQueueElement command;
    ble::CommandToMemoryQueueElement command_from_ble;

    flash_spi.init(flash_spi_config);
    flash.init();
    flash.reset();
    uint8_t jedec_id[6];
    flash.readJedecId(jedec_id);
    NRF_LOG_INFO("memory id: %x-%x-%x", jedec_id[0], jedec_id[1], jedec_id[2]);

    memory::block_device::register_flash_device(
        &flash, flash_sector_size, flash_page_size, flash_total_size);

    const auto init_result = memory::filesystem::init_littlefs(lfs, lfs_configuration);
    if(result::Result::OK != init_result)
    {
        NRF_LOG_ERROR("mem: failed to mount littlefs");
    }

    while(1)
    {
        const auto cmd_queue_receive_status = xQueueReceive(
            context.command_queue,
            reinterpret_cast<void*>(&command),
            _file_operation_context.is_file_open ? cmd_wait_fast_ticks : cmd_wait_idle_ticks);
        if(pdPASS == cmd_queue_receive_status)
        {
            process_request_from_state(context, command.command_id, command.args[0], command.args[1]);
        }
        if(_memory_owner == MemoryOwner::BLE)
        {
            const auto cmd_from_ble_queue_receive_status =
                xQueueReceive(context.command_from_ble_queue,
                              reinterpret_cast<void*>(&command_from_ble),
                              ble_command_wait_ticks);
            if(pdPASS == cmd_from_ble_queue_receive_status)
            {
                process_request_from_ble(
                    context, command_from_ble.command_id, command_from_ble.file_id);
            }
        }
        else
        {
            if(_file_operation_context.is_file_open)
            {
                BaseType_t audio_data_receive_status = pdTRUE;
                while(audio_data_receive_status == pdTRUE)
                {
                    audio_data_receive_status =
                        xQueueReceive(context.audio_data_queue,
                                      reinterpret_cast<void*>(&audio_data_queue_element),
                                      audio_data_wait_ticks);
                    if(pdPASS == audio_data_receive_status)
                    {
                        const auto write_result = memory::filesystem::write_data(
                            lfs, audio_data_queue_element.data, sizeof(audio_data_queue_element));

                        if(result::Result::OK != write_result)
                        {
                            NRF_LOG_ERROR("mem: data write failed");
                        }
                        else
                        {
                            written_record_size += sizeof(audio_data_queue_element);
                        }
                    }
                }
            }
        }
    }
}

static bool is_ble_access_allowed()
{
    return _memory_owner == MemoryOwner::BLE;
}

static constexpr uint32_t max_file_name_size{ble::fts::file_id_size + 1};

void convert_file_id_to_string(ble::fts::file_id_type file_id, char* buffer)
{
    memcpy(buffer, &file_id, sizeof(ble::fts::file_id_type));
}

void process_request_from_ble(Context& context,
                              ble::CommandToMemory command_id,
                              ble::fts::file_id_type file_id)
{
    ble::StatusFromMemoryQueueElement status{ble::StatusFromMemory::OK, 0};
    if(!is_ble_access_allowed())
    {
        status.status = ble::StatusFromMemory::ERROR_PERMISSION_DENIED;
        status.data_size = 0;
        const auto send_result = xQueueSend(context.status_to_ble_queue, &status, 0);
        if(pdTRUE != send_result)
        {
            NRF_LOG_ERROR("mem: failed to send response to BLE");
        }
        return;
    }

    switch(command_id)
    {
    case ble::CommandToMemory::GET_FILES_LIST: {
        uint32_t total_files_list_data_size{0};
        memory::TimeProfile tp("get_files_list");
        const auto ls_result =
            memory::filesystem::get_files_list(lfs,
                                               total_files_list_data_size,
                                               data_queue_elem.data,
                                               ble::fts::FtsService::get_file_list_char_size());
        if(result::Result::OK != ls_result)
        {
            NRF_LOG_ERROR("mem: failed to fetch files list");
            status.status = ble::StatusFromMemory::ERROR_OTHER;
            status.data_size = 0;
        }
        data_queue_elem.size =
            std::min(total_files_list_data_size,
                     static_cast<uint32_t>(ble::FileDataFromMemoryQueueElement::element_max_size));
        status.data_size = total_files_list_data_size;
        NRF_LOG_INFO("get_files_list: %d files", total_files_list_data_size / file_name_size_bytes);
        break;
    }

    case ble::CommandToMemory::GET_FILES_LIST_NEXT: {
        const auto ls_result = memory::filesystem::get_files_list_next(
            lfs,
            data_queue_elem.size,
            data_queue_elem.data,
            ble::fts::FtsService::get_file_list_char_size());
        if(result::Result::OK != ls_result)
        {
            NRF_LOG_ERROR("mem: failed to continue fetching files list");
            status.status = ble::StatusFromMemory::ERROR_OTHER;
            status.data_size = 0;
        }
        break;
    }
    case ble::CommandToMemory::GET_FILE_INFO: {
        char target_file_name[max_file_name_size] = {0};
        convert_file_id_to_string(file_id, target_file_name);
        
        const auto file_info_result = memory::filesystem::get_file_info(
            lfs,
            target_file_name,
            data_queue_elem.data,
            data_queue_elem.size,
            ble::FileDataFromMemoryQueueElement::element_max_size);

        if(result::Result::OK != file_info_result)
        {
            NRF_LOG_ERROR("mem: failed to fetch file info");
            status.status = ble::StatusFromMemory::ERROR_OTHER;
            status.data_size = 0;
        }
        if(data_queue_elem.size == 0)
        {
            NRF_LOG_ERROR("mem: file not found");
            status.status = ble::StatusFromMemory::ERROR_FILE_NOT_FOUND;
        }
        break;
    }
    case ble::CommandToMemory::OPEN_FILE: {
        if(_file_operation_context.is_file_open)
        {
            status.status = ble::StatusFromMemory::ERROR_FILE_ALREADY_OPEN;
            status.data_size = 0;
            break;
        }
        memory::TimeProfile tp("ble::file_open");
        char target_file_name[max_file_name_size] = {0};
        convert_file_id_to_string(file_id, target_file_name);
        const auto file_open_result =
            memory::filesystem::open_file(lfs, target_file_name, status.data_size);
        if(result::Result::OK != file_open_result)
        {
            status.status = ble::StatusFromMemory::ERROR_FILE_NOT_FOUND;
            break;
        }
        _file_operation_context.is_file_open = true;
        _file_operation_context.file_id = file_id;

        break;
    }
    case ble::CommandToMemory::CLOSE_FILE: {
        if(!_file_operation_context.is_file_open || file_id != _file_operation_context.file_id)
        {
            status.status = ble::StatusFromMemory::ERROR_OTHER;
            break;
        }
        char target_file_name[max_file_name_size] = {0};
        convert_file_id_to_string(file_id, target_file_name);
        
        const auto file_close_result = memory::filesystem::close_file(lfs, target_file_name);
        if(result::Result::OK != file_close_result)
        {
            status.status = ble::StatusFromMemory::ERROR_FILE_NOT_FOUND;
            break;
        }
        _file_operation_context.is_file_open = false;
        _file_operation_context.file_id.reset();

        break;
    }
    case ble::CommandToMemory::GET_FILE_DATA: {
        const auto file_data_result = memory::filesystem::get_file_data(
            lfs,
            data_queue_elem.data,
            data_queue_elem.size,
            ble::FileDataFromMemoryQueueElement::element_max_size);

        if(result::Result::OK != file_data_result)
        {
            NRF_LOG_ERROR("mem: failed to fetch file data");
            status.status = ble::StatusFromMemory::ERROR_OTHER;
            status.data_size = 0;
            break;
        }
        status.data_size = data_queue_elem.size;

        break;
    }
    case ble::CommandToMemory::GET_FS_STATUS: {
        const auto fs_stat_result =
            memory::filesystem::get_fs_stat(lfs, data_queue_elem.data, lfs_configuration);
        if(result::Result::OK != fs_stat_result)
        {
            NRF_LOG_ERROR("mem: failed to fetch fs stat");
            status.status = ble::StatusFromMemory::ERROR_OTHER;
            status.data_size = 0;
            break;
        }
        status.data_size = sizeof(ble::fts::FileSystemInterface::FSStatus);
        data_queue_elem.size = status.data_size;
    }
    default: {
        NRF_LOG_ERROR("mem from ble: command %d not yet implemented", command_id);
        break;
    }
    }
    const auto send_result = xQueueSend(context.status_to_ble_queue, &status, 0);
    if(pdTRUE != send_result)
    {
        NRF_LOG_ERROR("mem: failed to send status to BLE");
        return;
    }
    if(status.status != ble::StatusFromMemory::OK)
    {
        return;
    }
    const auto send_data_result =
        xQueueSend(context.data_to_ble_queue, &data_queue_elem, data_send_wait_ticks);
    if(pdTRUE != send_data_result)
    {
        NRF_LOG_ERROR("mem: failed to send data to BLE");
        return;
    }
}

void process_request_from_state(Context& context, const Command command_id, uint32_t arg0, uint32_t arg1)
{
    switch(command_id)
    {
    case Command::MOUNT_LFS: {
        const auto init_result = memory::filesystem::init_littlefs(lfs, lfs_configuration);
        if(result::Result::OK != init_result)
        {
            NRF_LOG_ERROR("mem: failed to mount littlefs");
        }
        break;
    }
    case Command::CREATE_RECORD: {
        memory::generate_next_file_name(active_record_name, context);
        {
            memory::TimeProfile tp("create_record");
            const auto create_result = memory::filesystem::create_file(lfs, active_record_name);
            if(result::Result::OK != create_result)
            {
                NRF_LOG_ERROR("lfs: failed to create a new rec");
                StatusQueueElement response{Command::CREATE_RECORD, Status::ERROR_GENERAL};
                xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
                return;
            }
            NRF_LOG_DEBUG("created record [%s]", active_record_name);
            _file_operation_context.is_file_open = true;
            StatusQueueElement response{Command::CREATE_RECORD, Status::OK};
            xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
            written_record_size = 0;
        }
        break;
    }
    case Command::CLOSE_WRITTEN_FILE: {
        NRF_LOG_INFO("mem: closing file");
        memory::TimeProfile tp("close_record");
        const auto close_result = memory::filesystem::close_file(lfs, active_record_name);
        if(close_result != result::Result::OK)
        {
            NRF_LOG_ERROR("lfs: failed to close file after writing");
            StatusQueueElement response{Command::CLOSE_WRITTEN_FILE, Status::ERROR_GENERAL};
            xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
            return;
        }
        NRF_LOG_DEBUG("closed record. written size = %d bytes", written_record_size);
        _file_operation_context.is_file_open = false;
        StatusQueueElement response{Command::CLOSE_WRITTEN_FILE, Status::OK};
        xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
        break;
    }
    case Command::SELECT_OWNER_BLE: {
        const auto deinit_result = memory::filesystem::deinit_littlefs(lfs);
        if(result::Result::OK != deinit_result)
        {
            NRF_LOG_ERROR("lfs: deinit failed");
            StatusQueueElement response{Command::SELECT_OWNER_BLE, Status::ERROR_GENERAL};
            xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
            break;
        }
        const auto init_result = memory::filesystem::init_littlefs(lfs, lfs_configuration);
        if(result::Result::OK != init_result)
        {
            NRF_LOG_ERROR("lfs: init failed");
            StatusQueueElement response{Command::SELECT_OWNER_BLE, Status::ERROR_GENERAL};
            xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
            break;
        }
        NRF_LOG_INFO("mem: owner changed to ble");
        _memory_owner = MemoryOwner::BLE;

        StatusQueueElement response{Command::SELECT_OWNER_BLE, Status::OK};
        xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
        break;
    }
    case Command::SELECT_OWNER_AUDIO: {
        const auto deinit_result = memory::filesystem::deinit_littlefs(lfs);
        if(result::Result::OK != deinit_result)
        {
            NRF_LOG_ERROR("lfs: deinit failed");
            StatusQueueElement response{Command::SELECT_OWNER_AUDIO, Status::ERROR_GENERAL};
            xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
            break;
        }
        const auto init_result = memory::filesystem::init_littlefs(lfs, lfs_configuration);
        if(result::Result::OK != init_result)
        {
            NRF_LOG_ERROR("lfs: init failed");
            StatusQueueElement response{Command::SELECT_OWNER_AUDIO, Status::ERROR_GENERAL};
            xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
            break;
        }
        NRF_LOG_INFO("mem: owner changed to audio");
        _memory_owner = MemoryOwner::AUDIO;
        StatusQueueElement response{Command::SELECT_OWNER_AUDIO, Status::OK};
        xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
        break;
    }
    case Command::LAUNCH_TEST_1: {
        launch_test_1(flash);
        break;
    }
    case Command::LAUNCH_TEST_2: {
        launch_test_2(flash);
        break;
    }
    case Command::LAUNCH_TEST_3: {
        launch_test_3(lfs, lfs_configuration);
        break;
    }
    case Command::LAUNCH_TEST_4: {
        launch_test_4(lfs);
        break;
    }
    case Command::LAUNCH_TEST_5: {
        launch_test_5(flash, arg0, arg1);
        break;
    }
    case Command::UNMOUNT_LFS: {
        NRF_LOG_ERROR("mem: lfs unmount is not implemented");
        break;
    }
    case Command::PERFORM_MEMORY_CHECK: {
        static constexpr uint32_t max_files_count{30};
        const auto fs_stat_result =
            memory::filesystem::get_fs_stat(lfs, data_queue_elem.data, lfs_configuration);
        if(result::Result::OK != fs_stat_result)
        {
            NRF_LOG_ERROR("mem: failed to fetch fs stat");
            StatusQueueElement response{Command::PERFORM_MEMORY_CHECK, Status::ERROR_GENERAL};
            xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
        }
        ble::fts::FileSystemInterface::FSStatus fsStatus;
        memcpy(&fsStatus, &data_queue_elem.data, sizeof(fsStatus));
        NRF_LOG_INFO("FS stats: %d/%d(%d)", fsStatus.occupied_space, fsStatus.free_space, fsStatus.files_count);
        StatusQueueElement response{Command::PERFORM_MEMORY_CHECK, Status::OK};
        if ((fsStatus.occupied_space > (flash_total_size/2)) || (fsStatus.files_count > max_files_count))
        {
            response.status = Status::FORMAT_REQUIRED;
        }
        xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
        break;
    }
    case Command::FORMAT_FS:
    {
        // erase the flash chip
        NRF_LOG_INFO("task memory: launching chip erase. Memory task shall not accept commands during the execution of this command.");
        const auto start_tick = xTaskGetTickCount();
        flash.eraseChip();
        static constexpr uint32_t max_chip_erase_duration{44000};
        while(flash.isBusy() && (xTaskGetTickCount() - start_tick) < max_chip_erase_duration)
        {
            vTaskDelay(200);
        }
        const auto end_tick = xTaskGetTickCount();
        StatusQueueElement response{Command::FORMAT_FS, Status::OK};
        if((end_tick - start_tick) > max_chip_erase_duration)
        {
            NRF_LOG_WARNING("task memory: chip erase timed out");
            response.status = Status::ERROR_BUSY;
        }
        else
        {
            NRF_LOG_INFO("task memory: chip erase took %d ms", end_tick - start_tick);
        }
        const auto init_result = memory::filesystem::init_littlefs(lfs, lfs_configuration);
        if(result::Result::OK != init_result)
        {
            NRF_LOG_ERROR("mem: failed to mount littlefs");
            response.status = Status::ERROR_GENERAL;
        }
        

        xQueueSend(context.status_queue, reinterpret_cast<void*>(&response), 0);
        break;
    }
    }
}

void generate_next_file_name_fallback(char* name)
{
    if(name == nullptr)
    {
        NRF_LOG_ERROR("File name generation error: nullptr");
        return;
    }

    uint32_t tmp_length{0};
    const auto name_read_result = memory::filesystem::get_latest_file_name(lfs, name, tmp_length);
    if(result::Result::OK != name_read_result)
    {
        NRF_LOG_WARNING("Failed to read latest record name. Using the default name");
        memset(name, '0', ble::fts::file_id_size);
        return;
    }

    const auto last_id =
        (name[ble::fts::file_id_size - 2] - '0') * 10 + (name[ble::fts::file_id_size - 1] - '0');
    const auto next_id = last_id + 1;
    name[ble::fts::file_id_size - 2] = ((next_id / 10) % 10) + '0';
    name[ble::fts::file_id_size - 1] = (next_id % 10) + '0';
}

void generate_next_file_name(char* name, const Context& context)
{
    if(nullptr == name)
        return;

    // send request to RTC task to figure out if RTC is available
    rtc::CommandQueueElement cmd{rtc::Command::GET_TIME, {}};
    const auto send_result = xQueueSend(context.commands_to_rtc_queue, &cmd, 0);
    if(pdTRUE != send_result)
    {
        // Queue operation has failed
        NRF_LOG_WARNING("mem: get time request has failed. Using fallback generator");
        generate_next_file_name_fallback(name);
        return;
    }

    static constexpr uint32_t get_time_response_wait_time{100};
    rtc::ResponseQueueElement response;
    const auto receive_result =
        xQueueReceive(context.response_from_rtc_queue, &response, get_time_response_wait_time);

    if(receive_result != pdTRUE || response.status != rtc::Status::OK)
    {
        // communication to RTC module has failed. need to fallback
        NRF_LOG_WARNING("mem: get time request has timed out. Using fallback generator");
        generate_next_file_name_fallback(name);
        return;
    }
    // FIXME: for now use day, hour, minute and second of the record
    snprintf(name,
             ble::fts::file_id_size + 1,
             "0000%02d%02d%02d%02d%02d%02d",
             response.content[5] % 100,
             response.content[4] % 12,
             response.content[3] % 31,
             response.content[2] % 24,
             response.content[1] % 60,
             response.content[0] % 60);
}

} // namespace memory