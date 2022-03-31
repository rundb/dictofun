// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "BleServices.h"

#include "drv_audio.h"
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include <ble_file_transfer_service.h>
#include <ble_services/ble_lbs/ble_lbs.h>
#include <boards/boards.h>
#include <nrf_log.h>

#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"

#include "ble_advertising.h"
#include "ble_dfu.h"
#include "nrf_bootloader_info.h"
#include "nrf_power.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"

#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"

#include "simple_fs.h"

NRF_BLE_QWR_DEF(m_qwr);
BLE_LBS_DEF(m_lbs);

#define APP_ADV_INTERVAL 300
#define APP_ADV_DURATION 18000

// TODO: move this buffer to class instance
static const size_t READ_BUFFER_SIZE = 256;
uint8_t readBuffer[READ_BUFFER_SIZE];

BLE_ADVERTISING_DEF(m_advertising);

namespace ble
{

BleServices* BleServices::_instance{nullptr};

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void led_write_handler(uint16_t conn_handle, ble_lbs_t* p_lbs, uint8_t led_state);

static void fts_data_handler(ble_fts_t* p_fts, uint8_t const* p_data, uint16_t length)
{
    BleServices::getInstance().handleFtsData(p_fts, p_data, length);
}

void BleServices::handleFtsData(ble_fts_t* p_fts, uint8_t const* p_data, uint16_t length)
{
    switch(p_data[0])
    {
    case CMD_GET_FILE: {
        NRF_LOG_DEBUG("cmd: get_file");
        _ble_cmd = (BleCommands)p_data[0];
        break;
    }
    case CMD_GET_FILE_INFO: {
        NRF_LOG_DEBUG("cmd: file_info");
        _ble_cmd = (BleCommands)p_data[0];
        break;
    }
    case CMD_GET_VALID_FILES_COUNT: {
        NRF_LOG_DEBUG("cmd: files_count");
        _ble_cmd = (BleCommands)p_data[0];
        break;
    }
    default: {
        NRF_LOG_ERROR("Unknown command: %02x", p_data[0]);
        break;
    }
    }
}

uint32_t BleServices::send_data(const uint8_t* data, uint32_t data_size)
{
    // if(data_size > 0)
    // {
    //     unsigned i = 0;

    //     uint32_t size_left = data_size;
    //     uint8_t* send_buffer = (uint8_t*)data;

    //     while(size_left)
    //     {
    //         uint8_t send_size = MIN(size_left, BLE_ITS_MAX_DATA_LEN);
    //         size_left -= send_size;

    //         uint32_t err_code = NRF_SUCCESS;
    //         while(true)
    //         {
    //             // err_code = ble_fts_send_file_fragment(&m_fts, send_buffer, send_size);
    //             if(err_code == NRF_SUCCESS)
    //             {
    //                 break;
    //             }
    //             else if(err_code != NRF_ERROR_RESOURCES)
    //             {
    //                 NRF_LOG_ERROR("Failed to send file, err = %d", err_code);
    //                 return err_code;
    //             }
    //         }

    //         send_buffer += send_size;
    //     }
    // }

    return NRF_SUCCESS;
}

// DFU-related stuff
static void buttonless_dfu_sdh_state_observer(nrf_sdh_state_evt_t state, void* p_context)
{
    if(state == NRF_SDH_EVT_STATE_DISABLED)
    {
        // Softdevice was disabled before going into reset. Inform bootloader to skip CRC on next boot.
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        //Go to system off.
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
    }
}

/* nrf_sdh state observer. */
NRF_SDH_STATE_OBSERVER(m_buttonless_dfu_state_obs, 0) = {
    .handler = buttonless_dfu_sdh_state_observer,
};

static void advertising_config_get(ble_adv_modes_config_t* p_config)
{
    memset(p_config, 0, sizeof(ble_adv_modes_config_t));

    p_config->ble_adv_fast_enabled = true;
    p_config->ble_adv_fast_interval = APP_ADV_INTERVAL;
    p_config->ble_adv_fast_timeout = APP_ADV_DURATION;
}

void BleServices::disconnect(uint16_t conn_handle, void* p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code =
        sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if(err_code != NRF_SUCCESS)
    {
        NRF_LOG_WARNING("Failed to disconnect connection. Connection handle: %d Error: %d",
                        conn_handle,
                        err_code);
    }
    else
    {
        NRF_LOG_DEBUG("Disconnected connection handle %d", conn_handle);
    }
}

static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch(event)
    {
    case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE: {
        NRF_LOG_INFO("Device is preparing to enter bootloader mode.");

        // Prevent device from advertising on disconnect.
        ble_adv_modes_config_t config;
        advertising_config_get(&config);
        config.ble_adv_on_disconnect_disabled = true;
        ble_advertising_modes_config_set(&m_advertising, &config);

        // Disconnect all other bonded devices that currently are connected.
        // This is required to receive a service changed indication
        // on bootup after a successful (or aborted) Device Firmware Update.
        uint32_t conn_count = ble_conn_state_for_each_connected(BleServices::disconnect, NULL);
        NRF_LOG_INFO("Disconnected %d links.", conn_count);
        break;
    }

    case BLE_DFU_EVT_BOOTLOADER_ENTER:
        // YOUR_JOB: Write app-specific unwritten data to FLASH, control finalization of this
        //           by delaying reset by reporting false in app_shutdown_handler
        NRF_LOG_INFO("Device will enter bootloader mode.");
        break;

    case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
        NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
        // YOUR_JOB: Take corrective measures to resolve the issue
        //           like calling APP_ERROR_CHECK to reset the device.
        break;

    case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
        NRF_LOG_ERROR("Request to send a response to client failed.");
        // YOUR_JOB: Take corrective measures to resolve the issue
        //           like calling APP_ERROR_CHECK to reset the device.
        APP_ERROR_CHECK(false);
        break;

    default:
        NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless.");
        break;
    }
}

BleServices::BleServices()
{
    _instance = this;
}

void BleServices::init()
{
    ret_code_t err_code;
    ble_lbs_init_t lbs_init = {0};
    ble_fts_init_t fts_init = {0};
    nrf_ble_qwr_init_t qwr_init = {0};
    ble_dfu_buttonless_init_t dfus_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize LBS.
    lbs_init.led_write_handler = led_write_handler;

    err_code = ble_lbs_init(&m_lbs, &lbs_init);
    APP_ERROR_CHECK(err_code);

    // Initialize FTS.
    fts_init.data_handler = fts_data_handler;

    err_code = ble_fts_init(&ble::fts::get_fts_instance(), &fts_init);
    APP_ERROR_CHECK(err_code);

    // Initiaize DFU service
    dfus_init.evt_handler = ble_dfu_evt_handler;
    err_code = ble_dfu_buttonless_init(&dfus_init);
    APP_ERROR_CHECK(err_code);
}

void BleServices::start()
{
    // Figure out how many files we have to be transmitted
    _files_count = filesystem::get_files_count();

    int size = 0;
    do {
        const auto open_result = filesystem::open(_file, filesystem::FileMode::RDONLY);
        if(open_result != result::Result::OK)
        {
            NRF_LOG_ERROR("BleServices::start(): file opening failure!");
            _is_file_transmission_done = true;
        }
        _file_size = _file.rom.size;
        if (_file_size == 0)
        {
            filesystem::close(_file);
        }
    } while (_file_size == 0);
    _fsm.start();
}

nrf_ble_qwr_t* BleServices::getQwrHandle()
{
    return &m_qwr;
}

// TODO: assert nullptr on uuids, assert when max_uuids is not enough
size_t BleServices::setAdvUuids(ble_uuid_t* uuids, size_t max_uuids)
{
    uuids[0] = {LBS_UUID_SERVICE, m_lbs.uuid_type};
    return 1U;
}

static void led_write_handler(uint16_t conn_handle, ble_lbs_t* p_lbs, uint8_t led_state)
{
    // TODO: get rid or fix to use the write LED
}

/**
 * TODO: implement a more consistent logic on several files transmission. 
 * TODO: implement consistency check for the commands' sequence from the application 
 *       expectation: GET_FILES_COUNT -> (M times) x {GET_FILE_SIZE -> GET_FILE (N times)},
 *       where M - valid files' count, N - number of chunks in single file
 * TODO: refactor to a more consistent structure (currently CMD_GET_FILE also does files' closing and opening)
 */
void BleServices::cyclic()
{
    _fsm.process_command(_ble_cmd);
    _ble_cmd = CMD_EMPTY;
    // switch(_ble_cmd)
    // {
    // case CMD_GET_FILE: {
    //     size_t read_size = 0;
    //     const auto read_result = filesystem::read(_file, readBuffer, READ_BUFFER_SIZE, read_size);
    //     if(read_result != result::Result::OK)
    //     {
    //         // ERROR!
    //         NRF_LOG_ERROR("BleSystem::cyclic(): file reading failure!");
    //         _is_file_transmission_done = true;
    //     }
    //     else 
    //     if(read_size != READ_BUFFER_SIZE)
    //     {
    //         const auto close_res = filesystem::close(_file);
    //         if (close_res != result::Result::OK)
    //         {
    //             NRF_LOG_ERROR("File closing failure! Might require an invalidation of FS");
    //             _is_file_transmission_done = true;
    //         }
    //         _files_count = filesystem::get_files_count();
    //         NRF_LOG_INFO("files count: valid=%d, invalid=%d", _files_count.valid, _files_count.invalid);
    //         if (_files_count.valid == 0)
    //         {
    //             _is_file_transmission_done = true;
    //             NRF_LOG_DEBUG("All files have been sent");
    //         }
    //         else
    //         {
    //             _is_file_transmission_started = false;
    //             const auto open_result = filesystem::open(_file, filesystem::FileMode::RDONLY);
    //             if(open_result != result::Result::OK)
    //             {
    //                 NRF_LOG_ERROR("File opening failure!");
    //                 _is_file_transmission_done = true;
    //             }
    //             else
    //             {
    //                 _file_size = _file.rom.size;
    //                 NRF_LOG_DEBUG("File has been sent, waiting for the next request");
    //             }
    //         }
            
    //         _ble_cmd = CMD_EMPTY;
    //     }
    //     else if(!_is_file_transmission_started)
    //     {
    //         drv_audio_wav_header_apply(readBuffer, _file_size);
    //         _is_file_transmission_started = true;
    //     }

    //     send_data(readBuffer, read_size);

    //     break;
    // }
    // case CMD_GET_FILE_INFO: {
    //     NRF_LOG_DEBUG("Sending file info, size %d", _file_size);

    //     ble_fts_file_info_t file_info;
    //     file_info.file_size_bytes = _file_size;

    //     ble_fts_file_info_send(&m_fts, &file_info);
    //     _ble_cmd = CMD_EMPTY;

    //     break;
    // }
    // case CMD_GET_VALID_FILES_COUNT: {
    //     _files_count = filesystem::get_files_count();
    //     NRF_LOG_DEBUG("Sending files' count: %d", _files_count.valid);

    //     ble_fts_filesystem_info_t fs_info;
    //     fs_info.valid_files_count = _files_count.valid;
    //     const auto res = ble_fts_filesystem_info_send(&m_fts, &fs_info);
    //     if (res != 0)
    //     {
    //         NRF_LOG_ERROR("fts filesystem info send has failed, error code %d", res);
    //     }

    //     _ble_cmd = CMD_EMPTY;
    //     break;
    // }
    // default:
    //     _ble_cmd = CMD_EMPTY;
    // }
}

} // namespace ble