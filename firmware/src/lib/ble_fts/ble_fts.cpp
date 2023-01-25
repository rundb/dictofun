// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts.h"

#include "ble_srv_common.h"
#include "ble.h"
#include "nrf_sdh_ble.h"
#include "nrf_log.h"

namespace ble
{
namespace fts
{

uint32_t FtsService::_ctx_data_pool[_max_clients * _link_ctx_size]{0};

blcm_link_ctx_storage_t FtsService::_link_ctx_storage =
{
    .p_ctx_data_pool = _ctx_data_pool,
    .max_links_cnt   = _max_clients,
    .link_ctx_size   = sizeof(_ctx_data_pool)/_max_clients
};

FtsService::Context FtsService::_context {
    0, 0, 
    {0, 0, 0, 0,}, {0,0,0,0,},
    0, false,
    &_link_ctx_storage
};

void on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

__attribute__ ((section("." STRINGIFY(sdh_ble_observers4)))) __attribute__((used))
nrf_sdh_ble_evt_observer_t observer = {
    .handler = on_ble_evt,
    .p_context = reinterpret_cast<void *>(&FtsService::_context),
};

FtsService::FtsService(FileSystemInterface& fs_if)
: _fs_if(fs_if)
{

}

ble_uuid128_t FtsService::get_service_uuid128()
{
    ble_uuid128_t result;
    memcpy(&result.uuid128, _service_uuid_base, _uuid_size);
    return result;
}

result::Result FtsService::add_characteristic(
    const uint8_t type, 
    const uint32_t uuid, 
    const uint32_t max_len, 
    ble_gatts_char_handles_t * handle)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write         = 1;
    char_md.char_props.write_wo_resp = 1;
    char_md.p_char_user_desc         = NULL;
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = NULL;
    char_md.p_sccd_md                = NULL;

    ble_uuid.type = type;
    ble_uuid.uuid = uuid;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen    = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = max_len;

    const auto char_add_result =  sd_ble_gatts_characteristic_add(_context.service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           handle);

    return (char_add_result == NRF_SUCCESS) ? result::Result::OK : result::Result::ERROR_GENERAL;
}

result::Result FtsService::init()
{
    ble_uuid_t    ble_uuid;


    const auto service_base_uuid = get_service_uuid128();
    const auto uuid_add_result = sd_ble_uuid_vs_add(&service_base_uuid, &_context.uuid_type);
    if (NRF_SUCCESS != uuid_add_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    ble_uuid.type = _context.uuid_type;
    ble_uuid.uuid = get_service_uuid();

    // Add the service.
    const auto service_add_result = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &_context.service_handle);
    if (NRF_SUCCESS != service_add_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    const auto cp_char_add_result = add_characteristic(
        BLE_UUID_TYPE_VENDOR_BEGIN,
        control_point_char_uuid, 
        cp_char_max_len,
        &_context.control_point_handles
    );

    if (result::Result::OK != cp_char_add_result)
    {
        return cp_char_add_result;
    }

    return result::Result::OK;
}

void on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    NRF_LOG_INFO("ble::fts::on_ble_evt");
}

}
}