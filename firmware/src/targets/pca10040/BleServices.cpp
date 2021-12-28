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

#include "BleServices.h"

#include <ble_services/ble_lbs/ble_lbs.h>
#include <ble/nrf_ble_qwr/nrf_ble_qwr.h>
#include <boards/boards.h>
#include <nrf_log.h>

NRF_BLE_QWR_DEF(m_qwr);
BLE_LBS_DEF(m_lbs);


namespace ble
{

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void led_write_handler(uint16_t conn_handle, ble_lbs_t * p_lbs, uint8_t led_state);


BleServices::BleServices() 
{}

void BleServices::init()
{
    ret_code_t         err_code;
    ble_lbs_init_t     lbs_init = {0};
    //ble_fts_init_t     fts_init = {0};
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
    // fts_init.data_handler = fts_data_handler;

    // err_code = ble_fts_init(&m_fts, &fts_init);
    // APP_ERROR_CHECK(err_code);

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