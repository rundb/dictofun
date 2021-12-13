#include "BleServices.h"

#include <ble_services/ble_lbs/ble_lbs.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include <ble_file_transfer_service.h>
#include <boards/boards.h>
#include <nrf_log.h>
#include "spi_access.h"
#include "drv_audio.h"

NRF_BLE_QWR_DEF(m_qwr);
BLE_LBS_DEF(m_lbs);
BLE_FTS_DEF(m_fts, NRF_SDH_BLE_TOTAL_LINK_COUNT);

namespace ble
{

BleServices * BleServices::_instance{nullptr};

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void led_write_handler(uint16_t conn_handle, ble_lbs_t * p_lbs, uint8_t led_state);

static void fts_data_handler(ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length)
{
    BleServices::getInstance().handleFtsData(p_fts, p_data, length);
}

void BleServices::handleFtsData(ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length)
{
    switch(p_data[0])
    {
        case CMD_GET_FILE:
            NRF_LOG_INFO("CMD_GET_FILE");
            _ble_cmd = (ble::BleCommands)p_data[0];
            break;
        case CMD_GET_FILE_INFO:
            NRF_LOG_INFO("CMD_GET_FILE_INFO");
            _ble_cmd = (ble::BleCommands)p_data[0];
            break;
        default:
            NRF_LOG_ERROR("Unknown command: %02x", p_data[0]);
            break;
    }
}

uint32_t BleServices::send_data(const uint8_t *data, uint32_t data_size)
{
    if (data_size > 0)
    {
        unsigned i = 0;

        uint32_t size_left = data_size;
        uint8_t *send_buffer = (uint8_t *)data;

        while (size_left)
        {
            uint8_t send_size = MIN(size_left, BLE_ITS_MAX_DATA_LEN);
            size_left -= send_size;

            uint32_t err_code = NRF_SUCCESS;
            while (true)
            {
                err_code = ble_fts_send_file_fragment(&m_fts, send_buffer, send_size);
                if (err_code == NRF_SUCCESS)
                {
                    break;
                }
                else if (err_code != NRF_ERROR_RESOURCES)
                {
                    NRF_LOG_ERROR("Failed to send file, err = %d", err_code);
                    return err_code;
                }
            }

            send_buffer += send_size;
        }
    }

    return NRF_SUCCESS;
}

BleServices::BleServices() 
{
    _instance = this;
}

void BleServices::init()
{
    ret_code_t         err_code;
    ble_lbs_init_t     lbs_init = {0};
    ble_fts_init_t     fts_init = {0};
    nrf_ble_qwr_init_t qwr_init = {0};

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

    err_code = ble_fts_init(&m_fts, &fts_init);
    APP_ERROR_CHECK(err_code);

}

nrf_ble_qwr_t * BleServices::getQwrHandle()
{
    return &m_qwr;
}

// TODO: assert nullptr on uuids, assert when max_uuids is not enough
size_t BleServices::setAdvUuids(ble_uuid_t * uuids, size_t max_uuids)
{
    uuids[0] = {LBS_UUID_SERVICE, m_lbs.uuid_type};
    return 1U;
}

#define LEDBUTTON_LED                   BSP_BOARD_LED_2

static void led_write_handler(uint16_t conn_handle, ble_lbs_t * p_lbs, uint8_t led_state)
{
    if (led_state)
    {
        NRF_LOG_INFO("Received LED ON!");
    }
    else
    {
        NRF_LOG_INFO("Received LED OFF!");
    }
}

void BleServices::cyclic()
{
    switch(_ble_cmd)
    {
        case CMD_GET_FILE:
        {
            if (_read_pointer == 0) // TODO change to file start
            {
                NRF_LOG_INFO("Starting file sending...");
            }

            if (!_file_size || _read_pointer >= _file_size)
            {
                _is_file_transmission_done = true;
                _ble_cmd = CMD_EMPTY;
                NRF_LOG_INFO("File have been sent.");
                break;
            }

            uint8_t buffer[SPI_READ_SIZE];
            spi_flash_trigger_read(_read_pointer, sizeof(buffer));
            while (spi_flash_is_spi_bus_busy());
            spi_flash_copy_received_data(buffer, sizeof(buffer));
            
            if(_read_pointer == 0)
            {
              drv_audio_wav_header_apply(buffer, _file_size / 2);
            }

            send_data(buffer, sizeof(buffer));

            _read_pointer += 2 * SPI_READ_SIZE;
            break;
        }
        case CMD_GET_FILE_INFO:
        {
            NRF_LOG_INFO("Sending file info, size %d (%X)", _file_size, _file_size);

            ble_fts_file_info_t file_info;
            file_info.file_size_bytes = _file_size / 2;

            ble_fts_file_info_send(&m_fts, &file_info);
            _ble_cmd = CMD_EMPTY;

            break;
        }
        default:
            _ble_cmd = CMD_EMPTY;
    }
}


}