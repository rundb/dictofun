// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_services.h"
#include "ble_lbs.h"
#include "nrf_log.h"

namespace ble
{

BLE_LBS_DEF(m_lbs);
NRF_BLE_QWR_DEF(m_qwr);

ble_uuid_t adv_uuids[] = {
    {
        LBS_UUID_SERVICE, 
        m_lbs.uuid_type
    },
};

constexpr size_t uuids_count{sizeof(adv_uuids) / sizeof(adv_uuids[0])};

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    NRF_LOG_ERROR("ble: QWR error %d", nrf_error);
}

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
    return result::Result::OK;
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

    memcpy(service_uuids, adv_uuids, sizeof(adv_uuids));
    return uuids_count;
}

nrf_ble_qwr_t * get_qwr_handle()
{
    return &m_qwr;
}

}
