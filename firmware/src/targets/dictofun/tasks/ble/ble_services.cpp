// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_services.h"
#include "ble_lbs.h"
#include "nrf_log.h"
#include "ble_fts.h"
#include "ble_fts_glue.h"
namespace ble
{

BLE_LBS_DEF(m_lbs);
NRF_BLE_QWR_DEF(m_qwr);

ble_uuid_t adv_uuids[] = {
    {
        LBS_UUID_SERVICE, 
        m_lbs.uuid_type
    },
    {
        0, BLE_UUID_TYPE_VENDOR_BEGIN,
    },
};

constexpr size_t uuids_count{sizeof(adv_uuids) / sizeof(adv_uuids[0])};

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    NRF_LOG_ERROR("ble: QWR error %d", nrf_error);
}

ble::fts::FtsService fts_service{integration::test::dictofun_test_fs_if};

result::Result init_services(ble_lbs_led_write_handler_t led_write_handler)
{
    // Initialize Queued Write Module.
    nrf_ble_qwr_init_t qwr_init = {0};
    qwr_init.error_handler = nrf_qwr_error_handler;
    const auto qwr_init_result = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    if (NRF_SUCCESS != qwr_init_result)
    {
        return result::Result::ERROR_GENERAL;
    }    

    // Initialize Led Button Service
    ble_lbs_init_t init{};
    init.led_write_handler = led_write_handler;
    const auto lbs_init_result = ble_lbs_init(&m_lbs, &init);
    if (NRF_SUCCESS != lbs_init_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    // Initialize File Transfer Service
    const auto fts_init_result = fts_service.init();
    if (result::Result::OK != fts_init_result)
    {
        return fts_init_result;
    }
    return result::Result::OK;
}

void set_fts_fs_handler(ble::fts::FileSystemInterface& fs_if)
{
    fts_service.set_fs_interface(fs_if);
}

void services::register_fs_communication_queues(QueueHandle_t * commands_queue, QueueHandle_t * status_queue, QueueHandle_t * data_queue)
{
    integration::target::register_filesystem_queues(commands_queue, status_queue, data_queue);
}

size_t get_services_uuids(ble_uuid_t * service_uuids, const size_t max_uuids)
{
    if (nullptr == service_uuids)
    {
        NRF_LOG_ERROR("ble: service uuid is nullptr");
        return 0;
    }
    if (uuids_count > max_uuids)
    {
        NRF_LOG_ERROR("ble: not enough space for adv uuids");
        return 0;
    }
    adv_uuids[0].type = m_lbs.uuid_type;

    adv_uuids[1].uuid = fts_service.get_service_uuid();
    adv_uuids[1].type = fts_service.get_service_uuid_type();

    memcpy(service_uuids, adv_uuids, sizeof(adv_uuids));
    return uuids_count;
}

nrf_ble_qwr_t * get_qwr_handle()
{
    return &m_qwr;
}

void services_process()
{
    fts_service.process();
}

}
