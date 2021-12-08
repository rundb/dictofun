#include "BleServices.h"

#include <ble_services/ble_lbs/ble_lbs.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include <ble_file_transfer_service.h>
#include <boards/boards.h>
#include <nrf_log.h>

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
//     switch(p_data[0])
//     {
//         case CMD_GET_FILE:
//             NRF_LOG_INFO("CMD_GET_FILE");
//             m_cmd = p_data[0];
//             m_file_size = write_pointer / 2;
//             break;
//         case CMD_GET_FILE_INFO:
//             NRF_LOG_INFO("CMD_GET_FILE_INFO");
//             m_cmd = p_data[0];
//             m_file_size = write_pointer / 2;
//             break;
//         default:
//             NRF_LOG_ERROR("Unknown command: %02x", p_data[0]);
//             break;
//     }
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
        bsp_board_led_on(LEDBUTTON_LED);
        NRF_LOG_INFO("Received LED ON!");
    }
    else
    {
        bsp_board_led_off(LEDBUTTON_LED);
        NRF_LOG_INFO("Received LED OFF!");
    }
}


}