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

__attribute__ ((section("." STRINGIFY(sdh_ble_observers2)))) __attribute__((used))
nrf_sdh_ble_evt_observer_t observer = {
    .handler = FtsService::on_ble_evt,
    .p_context = reinterpret_cast<void *>(&FtsService::_context),
};

FtsService * FtsService::_instance{nullptr};

FtsService::FtsService(FileSystemInterface& fs_if)
: _fs_if(fs_if)
{
    if (_instance == nullptr)
    {
        _instance = this;
    }
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
    ble_gatts_char_handles_t * handle,
    CharacteristicInUseType char_type)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write         = (char_type == CharacteristicInUseType::WRITE) ? 1 : 0;
    char_md.char_props.read          = (char_type == CharacteristicInUseType::READ_NOTIFY) ? 1 : 0;
    char_md.char_props.notify        = (char_type == CharacteristicInUseType::READ_NOTIFY) ? 1 : 0;
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
        BLE_UUID_TYPE_BLE,
        control_point_char_uuid, 
        cp_char_max_len,
        &_context.control_point_handles,
        CharacteristicInUseType::WRITE
    );

    if (result::Result::OK != cp_char_add_result)
    {
        return cp_char_add_result;
    }

    const auto file_list_char_add_result = add_characteristic(
        BLE_UUID_TYPE_BLE,
        file_list_char_uuid, 
        file_list_char_max_len,
        &_context.files_list,
        CharacteristicInUseType::READ_NOTIFY
    );

    if (result::Result::OK != file_list_char_add_result)
    {
        return file_list_char_add_result;
    }

    return result::Result::OK;
}

void FtsService::on_write(ble_evt_t const * p_ble_evt, ClientContext& client_context)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    if ((p_evt_write->handle == _context.control_point_handles.value_handle))
    {
        on_control_point_write(p_evt_write->len, p_evt_write->data);
    }
    else 
    {
        NRF_LOG_WARNING("write to unknown char with len %d", p_evt_write->len);   
    }
}

void FtsService::on_control_point_write(uint32_t len, const uint8_t * data)
{
    if (len == 0)
    {
        NRF_LOG_ERROR("cp.write: wrong write length");
        return;
    }
    switch (data[0])
    {
        case static_cast<int>(ControlPointOpcode::REQ_FILES_LIST):
        {
            on_req_files_list(len-1);
            break;
        }
        case static_cast<int>(ControlPointOpcode::REQ_FILE_INFO):
        {
            on_req_file_info(len - 1, &data[1]);
            break;
        }
        case static_cast<int>(ControlPointOpcode::REQ_FILE_DATA):
        {
            on_req_file_data(len - 1, &data[1]);
            break;
        }
        case static_cast<int>(ControlPointOpcode::REQ_FS_STATUS):
        {
            on_req_fs_status(len-1);
            break;
        }
        default:
        {
            NRF_LOG_ERROR("cp.write: wrong opcode");
            break;
        }
    }
}

void FtsService::on_req_files_list(uint32_t size)
{
    if (size != 0)
    {
        NRF_LOG_ERROR("cp.write: file list request wrong size");
        return;
    }
    NRF_LOG_INFO("cp.write: file list request");
}

uint32_t FtsService::get_file_id_from_raw(const uint8_t * data)
{
    return (data[0]) |  (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

void FtsService::on_req_file_info(uint32_t data_size, const uint8_t * file_id_data)
{
    if (data_size != file_id_size || file_id_data == nullptr)
    {
        NRF_LOG_ERROR("cp.write: wrong file id size");
        return;
    }
    const uint32_t file_id = get_file_id_from_raw(file_id_data);
    
    NRF_LOG_INFO("cp.write: file info request, id %d", file_id);
}

void FtsService::on_req_file_data(uint32_t data_size, const uint8_t * file_id_data)
{
    if (data_size != file_id_size || file_id_data == nullptr)
    {
        NRF_LOG_ERROR("cp.write: wrong file id size");
        return;
    }

    const uint32_t file_id = get_file_id_from_raw(file_id_data);
    
    NRF_LOG_INFO("cp.write: file data request, id %d", file_id);
}

void FtsService::on_req_fs_status(uint32_t size)
{
    if (size != 0)
    {
        NRF_LOG_ERROR("cp.write: fs stat request wrong size");
        return;
    }
    NRF_LOG_INFO("cp.write: fs status req");
}

void FtsService::on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }
    
    ClientContext& client_context = *reinterpret_cast<ClientContext*>(p_context);
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
        {   
            NRF_LOG_INFO("on_conn");
            break;
        }

        case BLE_GAP_EVT_DISCONNECTED:
        {
            NRF_LOG_INFO("on_disconn");
            break;
        }

        case BLE_GATTS_EVT_WRITE:
        {
            instance().on_write(p_ble_evt, client_context);
            break;
        }
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
        {
            NRF_LOG_INFO("on_evt_data_len_upd");
            break;
        }
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
        {
            NRF_LOG_INFO("on_gatts_mtu_req_chng");
            break;   
        }
        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        {
            NRF_LOG_INFO("on_gap_evt_param_upd");
            break;
        }
        default:
        {
            NRF_LOG_INFO("ble::fts::on_unk_evt::evt=%d", p_ble_evt->header.evt_id);
            break;
        }
    }
}

}
}