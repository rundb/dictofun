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
#include "ble_advertising.h"
#include "ble_services.h"
#include "nrf_error_decoder.h"
#include "ble_conn_params.h"
#include "nrf_sdh_freertos.h"
#include "boards.h"
#include "ble_fts_glue.h"

namespace ble
{

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
nrf_ble_qwr_t * _qwr_default_handle{nullptr};
// define has to be used here, it's a link-time feature from Nordic
#define APP_BLE_OBSERVER_PRIO 3
constexpr uint8_t APP_BLE_CONN_CFG_TAG{1};

// TODO: move this declaration into configuration memory
static const char device_name[] = BOARD_NAME;

// GAP parameters 
constexpr uint32_t MIN_CONN_INTERVAL{MSEC_TO_UNITS(100, UNIT_1_25_MS)};
constexpr uint32_t MAX_CONN_INTERVAL{MSEC_TO_UNITS(200, UNIT_1_25_MS)};
constexpr uint32_t SLAVE_LATENCY{0};
constexpr uint32_t CONN_SUP_TIMEOUT{MSEC_TO_UNITS(4000, UNIT_10_MS)};

// GATT parameters
NRF_BLE_GATT_DEF(m_gatt);

// Bonding parameters
constexpr uint8_t SEC_PARAM_BOND{1};
constexpr uint8_t SEC_PARAM_MITM{0};
constexpr uint8_t SEC_PARAM_LESC{0};
constexpr uint8_t SEC_PARAM_KEYPRESS{0};
constexpr uint8_t SEC_PARAM_IO_CAPABILITIES{BLE_GAP_IO_CAPS_NONE};
constexpr uint8_t SEC_PARAM_OOB{0};
constexpr uint8_t SEC_PARAM_MIN_KEY_SIZE{7};
constexpr uint8_t SEC_PARAM_MAX_KEY_SIZE{16};

// Advertising parameters

BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */

constexpr uint32_t APP_ADV_INTERVAL{200};                                      //  The advertising interval (in units of 0.625 ms)
constexpr uint32_t APP_ADV_DURATION{BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED};    //< The advertising time-out (in units of seconds). When set to 0, we will never time out.

// Connection parameters
constexpr uint32_t FIRST_CONN_PARAMS_UPDATE_DELAY{20000};
constexpr uint32_t NEXT_CONN_PARAMS_UPDATE_DELAY{5000};
constexpr uint32_t MAX_CONN_PARAMS_UPDATE_COUNT{3};

BleSystem * BleSystem::_instance{nullptr};

result::Result BleSystem::configure(ble_lbs_led_write_handler_t led_write_handler)
{
    const auto sdh_init_result = init_sdh();
    if (result::Result::OK != sdh_init_result)
    {
        NRF_LOG_ERROR("ble: sdh init failed");
        return sdh_init_result;
    }

    const auto gap_init_result = init_gap();
    if (result::Result::OK != gap_init_result)
    {
        NRF_LOG_ERROR("ble: gap init failed");
        return gap_init_result;
    }

    const auto gatt_init_result = init_gatt();
    if (result::Result::OK != gatt_init_result)
    {
        NRF_LOG_ERROR("ble: gatt init failed");
        return gatt_init_result;
    }

    const auto bonding_init_result = init_bonding();
    if (result::Result::OK != bonding_init_result)
    {
        NRF_LOG_ERROR("ble: bonding init failed");
        return gatt_init_result;
    }

    const auto services_init_result = init_services(led_write_handler);
    if (result::Result::OK != services_init_result)
    {
        NRF_LOG_ERROR("ble: services init failed");
        return services_init_result;
    }

    const auto adv_init_result = init_advertising();
    if (result::Result::OK != adv_init_result)
    {
        NRF_LOG_ERROR("ble: adv init failed");
        return adv_init_result;
    }

    const auto conn_param_init_result = init_conn_params();
    if (result::Result::OK != conn_param_init_result)
    {
        NRF_LOG_ERROR("ble: conn parameters setup failed");
        return conn_param_init_result;
    }

    return result::Result::OK;
}

result::Result BleSystem::start()
{
    if (_is_cold_start_required)
    {
        nrf_sdh_freertos_init(start_advertising, nullptr);
        _is_cold_start_required = false;
    }
    else 
    {
        nrf_sdh_freertos_task_resume();
    }
    _is_active = true;

    return result::Result::OK;
}

result::Result BleSystem::stop()
{
    if (BLE_CONN_HANDLE_INVALID != m_conn_handle)
    {
        const auto disconnect_result = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (NRF_SUCCESS != disconnect_result) 
        {
            NRF_LOG_ERROR("ble: failed to disconnect current connection");
        }
        else 
        {
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
        }
    }

    _is_active = false;
    nrf_sdh_freertos_task_pause();

    return result::Result::OK;
}

void BleSystem::process()
{
    if (_is_active)
    {
        services_process();
    }
}

void BleSystem::connect_fts_to_target_fs()
{
    set_fts_fs_handler(integration::target::dictofun_fs_if);
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
            err_code = nrf_ble_qwr_conn_handle_assign(get_qwr_handle(), m_conn_handle);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_ERROR("ble: evt conn error (%s)", helpers::decode_error(err_code));
            }
            BleSystem::instance()._has_connect_happened = true;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            //bsp_board_led_off(CONNECTED_LED);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            BleSystem::instance()._has_disconnect_happened = true;
            if (BleSystem::instance()._has_pairing_reset_been_requested)
            {
                const auto peer_delete_result = pm_peer_delete(BleSystem::default_peer_id);
                if (NRF_SUCCESS != peer_delete_result)
                {
                    NRF_LOG_ERROR("ble: peer delete has failed (%d)", peer_delete_result);
                }
                BleSystem::instance()._has_pairing_reset_been_requested = false;
            }
            BleSystem::instance()._has_connect_happened = false;
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
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
            NRF_LOG_INFO("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .tx_phys = BLE_GAP_PHY_AUTO,
                .rx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_ERROR("ble: evt phy upd error (%s)", helpers::decode_error(err_code));
            }
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_ERROR("ble: gatts sys attr error (%s)", helpers::decode_error(err_code));
            }
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_INFO("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_ERROR("ble: gap disconn error (%s)", helpers::decode_error(err_code));
            }
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_INFO("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (NRF_SUCCESS != err_code)
            {
                NRF_LOG_ERROR("ble: gap disconn error (%s)", helpers::decode_error(err_code));
            }
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
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }

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
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }

    err_code = pm_register(pm_evt_handler);
    if (NRF_SUCCESS != err_code)
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

void BleSystem::bonded_client_add(pm_evt_t const * /*p_evt*/)
{
    // uint16_t   conn_handle = p_evt->conn_handle;
    // uint16_t   peer_id     = p_evt->peer_id;
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

void BleSystem::on_bonded_peer_reconnection_lvl_notify(pm_evt_t const * /*p_evt*/)
{
    // static uint16_t   peer_id   = PM_PEER_ID_INVALID;
    // peer_id = p_evt->peer_id;
}

static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    // uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("adv: Fast advertising.");
            // APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            NRF_LOG_INFO("adv: idle");
            // sleep_mode_enter();
            break;

        default:
            break;
    }
}

result::Result BleSystem::init_advertising()
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    static constexpr size_t MAX_UUIDS_COUNT{2U};
    ble_uuid_t adv_uuids[MAX_UUIDS_COUNT]{0};
    
    [[maybe_unused]] const auto uuids_count = get_services_uuids(adv_uuids, MAX_UUIDS_COUNT); 

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = 1;
    init.advdata.uuids_complete.p_uuids  = &adv_uuids[0];
    init.srdata.uuids_complete.uuid_cnt = 1;//uuids_count;
    init.srdata.uuids_complete.p_uuids  = &adv_uuids[1];

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.config.ble_adv_extended_enabled = false;
    init.config.ble_adv_primary_phy      = BLE_GAP_PHY_1MBPS;
    init.config.ble_adv_secondary_phy    = BLE_GAP_PHY_2MBPS;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_ERROR("ble: adv init error (%s)", helpers::decode_error(err_code));
        return result::Result::ERROR_GENERAL;
    }

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);

    return result::Result::OK;
}

void BleSystem::start_advertising(void * /*context_ptr*/)
{
    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);

    if (err_code != 0)
    {
        NRF_LOG_ERROR("ble: adv start error (%s)", helpers::decode_error(err_code));
    }
}

void BleSystem::stop_advertising(void * /*context_ptr*/)
{
    const auto err_code = sd_ble_gap_adv_stop(m_advertising.adv_handle);
    if (NRF_SUCCESS != err_code) 
    {
        NRF_LOG_ERROR("ble: adv stop error (%d)", err_code);
    }
}

void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;
    NRF_LOG_DEBUG("ble: on conn param evt");

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        if (NRF_SUCCESS != err_code)
        {
            NRF_LOG_ERROR("ble: sd gap disconn (%s)", helpers::decode_error(err_code));
        }
    }
}

void conn_params_error_handler(uint32_t nrf_error)
{
    NRF_LOG_ERROR("ble: conn params error (%s)", helpers::decode_error(nrf_error));
}

result::Result BleSystem::init_conn_params()
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_ERROR("ble: conn params init (%s)", helpers::decode_error(err_code));
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

void BleSystem::register_fs_communication_queues(
    QueueHandle_t commands_queue, 
    QueueHandle_t status_queue, 
    QueueHandle_t data_queue)
{
    services::register_fs_communication_queues(commands_queue, status_queue, data_queue);
}

void BleSystem::register_keepalive_queue(QueueHandle_t keepalive_queue)
{
    services::register_keepalive_queue(keepalive_queue);
}

bool BleSystem::is_fts_active()
{
    return services::is_fts_active();
}

result::Result BleSystem::reset_pairing()
{
    // If connection is not established, peer can be removed straight away
    if (!_has_connect_happened || _has_disconnect_happened)
    {
        const auto peer_delete_result = pm_peer_delete(BleSystem::default_peer_id);
        if (NRF_SUCCESS != peer_delete_result)
        {
            NRF_LOG_ERROR("ble: peer delete has failed (%d)", peer_delete_result);
        }
        else
        {
            NRF_LOG_INFO("ble: peer delete succeeded");
        }
    }
    else
    {
        // If device is connected, one has to wait until disconnect.
        _has_pairing_reset_been_requested = true;
    }
    return result::Result::OK;
}

}
