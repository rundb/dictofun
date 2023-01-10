// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_system.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_ble_qwr.h"
#include "nrf_ble_gatt.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"

namespace ble
{

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
nrf_ble_qwr_t * _qwr_default_handle{nullptr};
constexpr uint8_t APP_BLE_OBSERVER_PRIO{3};
constexpr uint8_t APP_BLE_CONN_CFG_TAG{1};

// TODO: move this declaration into configuration memory
static const char device_name[] = "dictofun";

// GAP parameters 
constexpr uint32_t MIN_CONN_INTERVAL{MSEC_TO_UNITS(100, UNIT_1_25_MS)};
constexpr uint32_t MAX_CONN_INTERVAL{MSEC_TO_UNITS(200, UNIT_1_25_MS)};
constexpr uint32_t SLAVE_LATENCY{0};
constexpr uint32_t CONN_SUP_TIMEOUT{MSEC_TO_UNITS(4000, UNIT_10_MS)};

// GATT parameters
static nrf_ble_gatt_t m_gatt;

// Bonding parameters
constexpr uint8_t SEC_PARAM_BOND{1};
constexpr uint8_t SEC_PARAM_MITM{0};
constexpr uint8_t SEC_PARAM_LESC{0};
constexpr uint8_t SEC_PARAM_KEYPRESS{0};
constexpr uint8_t SEC_PARAM_IO_CAPABILITIES{BLE_GAP_IO_CAPS_NONE};
constexpr uint8_t SEC_PARAM_OOB{0};
constexpr uint8_t SEC_PARAM_MIN_KEY_SIZE{7};
constexpr uint8_t SEC_PARAM_MAX_KEY_SIZE{16};

result::Result BleSystem::configure()
{
    const auto sdh_init_result = init_sdh();
    if (result::Result::OK != sdh_init_result)
    {
        return sdh_init_result;
    }

    const auto gap_init_result = init_gap();
    if (result::Result::OK != gap_init_result)
    {
        return sdh_init_result;
    }

    const auto gatt_init_result = init_gatt();
    if (result::Result::OK != gap_init_result)
    {
        return gatt_init_result;
    }

    return result::Result::OK;
}

result::Result BleSystem::start()
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result BleSystem::stop()
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

/// ============ Private API implementation
void BleSystem::ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;
   
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(_qwr_default_handle, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            //bsp_board_led_off(CONNECTED_LED);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            break;
        
        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;
       case BLE_GAP_EVT_AUTH_STATUS:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x",
                          p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
                          p_ble_evt->evt.gap_evt.params.auth_status.bonded,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .tx_phys = BLE_GAP_PHY_AUTO,
                .rx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}

result::Result BleSystem::init_sdh()
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }

    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }

    err_code = nrf_sdh_ble_enable(&ram_start);
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }

    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, BleSystem::ble_evt_handler, NULL);  

    return result::Result::OK;
}

result::Result BleSystem::init_gap()
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)device_name,
                                          strlen(device_name));
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }

    return result::Result::OK;
}
result::Result BleSystem::init_gatt()
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}
result::Result BleSystem::init_bonding()
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

void BleSystem::bonded_client_add(pm_evt_t const * p_evt)
{
    uint16_t   conn_handle = p_evt->conn_handle;
    uint16_t   peer_id     = p_evt->peer_id;
    // TODO: implement
}

void BleSystem::bonded_client_remove_all()
{
    // TODO: implement
}

void BleSystem::pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
            bonded_client_add(p_evt);
            on_bonded_peer_reconnection_lvl_notify(p_evt);
            break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
            bonded_client_add(p_evt);
            break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            bonded_client_remove_all();

            // Bonds are deleted. Start scanning.
            break;

        default:
            break;
    }
}

void BleSystem::on_bonded_peer_reconnection_lvl_notify(pm_evt_t const * p_evt)
{
    ret_code_t        err_code;
    static uint16_t   peer_id   = PM_PEER_ID_INVALID;

    peer_id = p_evt->peer_id;
}


result::Result BleSystem::init_advertising()
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

}