// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_services.h"
#include "ble_bas.h"
#include "ble_cts_c.h"
#include "ble_db_discovery.h"
#include "ble_fts.h"
#include "ble_fts_glue.h"
#include "nrf_ble_gq.h"
#include "nrf_log.h"
#include "time.h"
#include "nrf_sdh.h"
#include "peer_manager.h"

namespace ble
{

NRF_BLE_QWR_DEF(m_qwr);
BLE_DB_DISCOVERY_DEF(m_ble_db_discovery); /**< DB discovery module instance. */
NRF_BLE_GQ_DEF(m_ble_gatt_queue, /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_PERIPHERAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);

BLE_CTS_C_DEF(m_cts_c);
BLE_BAS_DEF(m_bas);

ble_uuid_t adv_uuids[] = {
    {
        0,
        BLE_UUID_TYPE_VENDOR_BEGIN,
    },
    {BLE_UUID_CURRENT_TIME_SERVICE, BLE_UUID_TYPE_BLE},
};

volatile bool _is_current_time_update_pending{false};
time::DateTime _current_time;
uint8_t _batt_level{100};

constexpr size_t uuids_count{sizeof(adv_uuids) / sizeof(adv_uuids[0])};
std::function<void()> _disconnect_delegate{nullptr};

static void db_discovery_init();
static void battery_level_update();

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    NRF_LOG_ERROR("ble: QWR error %d", nrf_error);
}

void services::set_bas_battery_level(const uint8_t batt_level)
{
    _batt_level = batt_level;
    battery_level_update();
}

static void current_time_set(ble_cts_c_evt_t* p_evt)
{
    NRF_LOG_DEBUG("Received current time from CTS");
    _current_time.year = p_evt->params.current_time.exact_time_256.day_date_time.date_time.year;
    _current_time.month = p_evt->params.current_time.exact_time_256.day_date_time.date_time.month;
    _current_time.day = p_evt->params.current_time.exact_time_256.day_date_time.date_time.day;
    _current_time.weekday = p_evt->params.current_time.exact_time_256.day_date_time.day_of_week;
    _current_time.hour = p_evt->params.current_time.exact_time_256.day_date_time.date_time.hours;
    _current_time.minute =
        p_evt->params.current_time.exact_time_256.day_date_time.date_time.minutes;
    _current_time.second =
        p_evt->params.current_time.exact_time_256.day_date_time.date_time.seconds;

    _is_current_time_update_pending = true;
}

result::Result services::get_current_time_update(time::DateTime& datetime)
{
    if(_is_current_time_update_pending)
    {
        _is_current_time_update_pending = false;
        datetime = _current_time;
        return result::Result::OK;
    }
    return result::Result::OK;
}

static void on_cts_c_evt(ble_cts_c_t* p_cts, ble_cts_c_evt_t* p_evt)
{
    ret_code_t err_code;

    switch(p_evt->evt_type)
    {
    case BLE_CTS_C_EVT_DISCOVERY_COMPLETE:
        // NRF_LOG_INFO("Current Time Service discovered on server.");
        err_code =
            ble_cts_c_handles_assign(&m_cts_c, p_evt->conn_handle, &p_evt->params.char_handles);
        if(err_code != NRF_SUCCESS)
        {
            NRF_LOG_ERROR("Failed to assign CTS handles")
        }
        else
        {
            services::request_current_time();
        }
        break;

    case BLE_CTS_C_EVT_DISCOVERY_FAILED:
        break;

    case BLE_CTS_C_EVT_DISCONN_COMPLETE:
        // NRF_LOG_INFO("Disconnect Complete.");
        break;

    case BLE_CTS_C_EVT_CURRENT_TIME:
        current_time_set(p_evt);
        break;

    case BLE_CTS_C_EVT_INVALID_TIME:
        // NRF_LOG_INFO("Invalid Time received.");
        break;

    default:
        break;
    }
}

static void current_time_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

ble::fts::FtsService fts_service{integration::test::dictofun_test_fs_if};

result::Result init_services()
{
    db_discovery_init();

    // Initialize Queued Write Module.
    nrf_ble_qwr_init_t qwr_init = {0};
    qwr_init.error_handler = nrf_qwr_error_handler;
    const auto qwr_init_result = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    if(NRF_SUCCESS != qwr_init_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    // Initialize File Transfer Service
    const auto fts_init_result = fts_service.init();
    if(result::Result::OK != fts_init_result)
    {
        return fts_init_result;
    }

    ble_cts_c_init_t cts_init = {0};

    // Initialize CTS.
    cts_init.evt_handler = on_cts_c_evt;
    cts_init.error_handler = current_time_error_handler;
    cts_init.p_gatt_queue = &m_ble_gatt_queue;
    const auto cts_init_err_code = ble_cts_c_init(&m_cts_c, &cts_init);
    if(NRF_SUCCESS != cts_init_err_code)
    {
        NRF_LOG_ERROR("CTS: initialization has failed (err code: %d)", cts_init_err_code);
        return result::Result::ERROR_GENERAL;
    }

    // Initialize Battery Service.
    ble_bas_init_t bas_init;
    memset(&bas_init, 0, sizeof(bas_init));

    bas_init.evt_handler = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref = NULL;
    bas_init.initial_batt_level = _batt_level;

    // Here the sec level for the Battery Service can be changed/increased.
    bas_init.bl_rd_sec = SEC_OPEN;
    bas_init.bl_cccd_wr_sec = SEC_OPEN;
    bas_init.bl_report_rd_sec = SEC_OPEN;

    const auto bas_init_err_code = ble_bas_init(&m_bas, &bas_init);
    if(bas_init_err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("BAS: initialization has failed (err code: %d)", bas_init_err_code);
        return result::Result::ERROR_GENERAL;
    }

    return result::Result::OK;
}

static void db_disc_handler(ble_db_discovery_evt_t* p_evt)
{
    ble_cts_c_on_db_disc_evt(&m_cts_c, p_evt);
}

void db_discovery_init()
{
    ble_db_discovery_init_t db_init;

    memset(&db_init, 0, sizeof(ble_db_discovery_init_t));

    db_init.evt_handler = db_disc_handler;
    db_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_init);
    APP_ERROR_CHECK(err_code);
}

void battery_level_update()
{
    const auto err_code = ble_bas_battery_level_update(&m_bas, _batt_level, BLE_CONN_HANDLE_ALL);
    if((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_INVALID_STATE) &&
       (err_code != NRF_ERROR_RESOURCES) && (err_code != NRF_ERROR_BUSY) &&
       (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
    {
        NRF_LOG_ERROR("BAS: failed to update batt level (code %d)", err_code);
    }
}

void services::start_db_discovery(uint16_t conn_handle)
{
    const auto err_code = ble_db_discovery_start(&m_ble_db_discovery, conn_handle);
    if(NRF_SUCCESS != err_code)
    {
        NRF_LOG_ERROR("db discovery start has failed");
    }
}

void services::restart()
{
    fts_service.reset_context();
}

void set_fts_fs_handler(ble::fts::FileSystemInterface& fs_if)
{
    fts_service.set_fs_interface(fs_if);
}

void services::register_fs_communication_queues(QueueHandle_t commands_queue,
                                                QueueHandle_t status_queue,
                                                QueueHandle_t data_queue)
{
    integration::target::register_filesystem_queues(commands_queue, status_queue, data_queue);
}

void services::register_keepalive_queue(QueueHandle_t keepalive_queue)
{
    integration::target::register_keepalive_queue(keepalive_queue);
}

void services::register_fts_to_state_status_queue(QueueHandle_t status_queue)
{
    integration::target::register_fts_to_state_status_queue(status_queue);
}

bool services::is_fts_active()
{
    return fts_service.is_file_transmission_running();
}

bool services::is_current_time_update_pending()
{
    return _is_current_time_update_pending;
}

void services::request_current_time()
{
    if(m_cts_c.conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        const auto err_code = ble_cts_c_current_time_read(&m_cts_c);
        if(err_code == NRF_ERROR_NOT_FOUND)
        {
            NRF_LOG_INFO("Current Time Service is not discovered.");
        }
    }
    else
    {
        NRF_LOG_DEBUG("CTS handle is invalid");
    }
}

void services::register_disconnect_delegate(std::function<void()> disconnect_delegate)
{
    _disconnect_delegate = disconnect_delegate;
}

size_t get_services_uuids(ble_uuid_t* service_uuids, const size_t max_uuids)
{
    if(nullptr == service_uuids)
    {
        NRF_LOG_ERROR("ble: service uuid is nullptr");
        return 0;
    }
    if(uuids_count > max_uuids)
    {
        NRF_LOG_ERROR("ble: not enough space for adv uuids");
        return 0;
    }

    adv_uuids[0].uuid = fts_service.get_service_uuid();
    adv_uuids[0].type = fts_service.get_service_uuid_type();

    memcpy(service_uuids, adv_uuids, sizeof(adv_uuids));
    return uuids_count;
}

nrf_ble_qwr_t* get_qwr_handle()
{
    return &m_qwr;
}

void services_process()
{
    fts_service.process();
    if (fts_service.is_unpair_requested())
    {
        // TODO: move it to system info provider, when it's available
        const auto err_code = pm_peers_delete();
        if (NRF_SUCCESS != err_code)
        {
            NRF_LOG_ERROR("pm peers deleted has failed");
        }
    }
    if (fts_service.is_disconnect_requested())
    {
        if (_disconnect_delegate != nullptr)
        {
            NRF_LOG_INFO("Performing a disconnect upon transaction completion");
            _disconnect_delegate();
        }
    }
}

} // namespace ble
