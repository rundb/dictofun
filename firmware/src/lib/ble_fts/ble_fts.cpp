// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts.h"

#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include "nrf_sdh_ble.h"

#include "noinit_mem.h"

namespace ble
{
namespace fts
{

bool operator==(const file_id_type& lhs, const file_id_type& rhs)
{
    for(auto i = 0U; i < file_id_size; ++i)
    {
        if(lhs.data[i] != rhs.data[i])
        {
            return false;
        }
    }
    return true;
}

uint32_t FtsService::_ctx_data_pool[_max_clients * _link_ctx_size]{0};

blcm_link_ctx_storage_t FtsService::_link_ctx_storage = {.p_ctx_data_pool = _ctx_data_pool,
                                                         .max_links_cnt = _max_clients,
                                                         .link_ctx_size =
                                                             sizeof(_ctx_data_pool) / _max_clients};

// clang-format off
FtsService::Context FtsService::_context{0xDEADBEEF,
                                         0xFEEBDAED,
                                         0xEFCCAA55,
                                         0,
                                         0,
                                         {0, 0, 0, 0,},
                                         {0, 0, 0, 0,},
                                         {0, 0, 0, 0,},
                                         {0, 0, 0, 0,},
                                         {0, 0, 0, 0,},
                                         {0, 0, 0, 0,},
                                         {0, 0, 0, 0,},
                                         {0, 0, 0, 0,},
                                         0,
                                         false,
                                         &_link_ctx_storage,
                                         0xBEEFDEAD};

__attribute__((section("." STRINGIFY(sdh_ble_observers2)))) __attribute__((used))
nrf_sdh_ble_evt_observer_t observer = {
    .handler = FtsService::on_ble_evt,
    .p_context = reinterpret_cast<void*>(&FtsService::_context),
};

// clang-format on

FtsService* FtsService::_instance{nullptr};

FtsService::FtsService(FileSystemInterface& fs_if)
    : _fs_if(fs_if)
{
    if(_instance == nullptr)
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

result::Result FtsService::add_characteristic(const uint8_t type,
                                              const uint32_t uuid,
                                              const uint32_t max_len,
                                              ble_gatts_char_handles_t* handle,
                                              CharacteristicInUseType char_type,
                                              bool should_be_secured)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t attr_char_value;
    ble_uuid_t ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write = (char_type == CharacteristicInUseType::WRITE) ? 1 : 0;
    char_md.char_props.read = (char_type == CharacteristicInUseType::READ_NOTIFY) ? 1 : 0;
    char_md.char_props.notify = (char_type == CharacteristicInUseType::READ_NOTIFY ||
                                 char_type == CharacteristicInUseType::WRITE)
                                    ? 1
                                    : 0;
    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf = NULL;
    char_md.p_user_desc_md = NULL;
    char_md.p_cccd_md = NULL;
    char_md.p_sccd_md = NULL;

    ble_uuid.type = type;
    ble_uuid.uuid = uuid;

    memset(&attr_md, 0, sizeof(attr_md));

    if(should_be_secured)
    {
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&attr_md.write_perm);
    }
    else
    {
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    }

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = 1;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = max_len;

    const auto char_add_result = sd_ble_gatts_characteristic_add(
        _context.service_handle, &char_md, &attr_char_value, handle);
    if(char_add_result != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("add char failed: %x : %x", uuid, char_add_result);
    }

    return (char_add_result == NRF_SUCCESS) ? result::Result::OK : result::Result::ERROR_GENERAL;
}

result::Result FtsService::init()
{
    ble_uuid_t ble_uuid;
    const auto service_base_uuid = get_service_uuid128();
    const auto uuid_add_result = sd_ble_uuid_vs_add(&service_base_uuid, &_context.uuid_type);
    if(NRF_SUCCESS != uuid_add_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    ble_uuid.type = _context.uuid_type;
    ble_uuid.uuid = get_service_uuid();

    // Add the service.
    const auto service_add_result =
        sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &_context.service_handle);
    if(NRF_SUCCESS != service_add_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    const auto cp_char_add_result = add_characteristic(BLE_UUID_TYPE_BLE,
                                                       control_point_char_uuid,
                                                       cp_char_max_len,
                                                       &_context.control_point_handles,
                                                       CharacteristicInUseType::WRITE,
                                                       true);

    if(result::Result::OK != cp_char_add_result)
    {
        return cp_char_add_result;
    }

    const auto file_list_char_add_result = add_characteristic(BLE_UUID_TYPE_BLE,
                                                              file_list_char_uuid,
                                                              file_list_char_max_len,
                                                              &_context.files_list,
                                                              CharacteristicInUseType::READ_NOTIFY,
                                                              true);

    if(result::Result::OK != file_list_char_add_result)
    {
        return file_list_char_add_result;
    }

    const auto file_list_next_char_add_result =
        add_characteristic(BLE_UUID_TYPE_BLE,
                           file_list_next_char_uuid,
                           file_list_next_char_max_len,
                           &_context.files_list_next,
                           CharacteristicInUseType::READ_NOTIFY,
                           true);

    if(result::Result::OK != file_list_next_char_add_result)
    {
        return file_list_next_char_add_result;
    }

    const auto file_info_char_add_result = add_characteristic(BLE_UUID_TYPE_BLE,
                                                              file_info_char_uuid,
                                                              file_info_char_max_len,
                                                              &_context.file_info,
                                                              CharacteristicInUseType::READ_NOTIFY,
                                                              true);

    if(result::Result::OK != file_info_char_add_result)
    {
        return file_info_char_add_result;
    }

    const auto file_data_char_add_result = add_characteristic(BLE_UUID_TYPE_BLE,
                                                              file_data_char_uuid,
                                                              file_data_char_max_len,
                                                              &_context.file_data,
                                                              CharacteristicInUseType::READ_NOTIFY,
                                                              true);

    if(result::Result::OK != file_data_char_add_result)
    {
        return file_data_char_add_result;
    }

    const auto fs_status_char_add_result = add_characteristic(BLE_UUID_TYPE_BLE,
                                                              fs_status_char_uuid,
                                                              fs_status_char_max_len,
                                                              &_context.fs_status,
                                                              CharacteristicInUseType::READ_NOTIFY,
                                                              true);

    if(result::Result::OK != fs_status_char_add_result)
    {
        return fs_status_char_add_result;
    }

    const auto status_char_add_result = add_characteristic(BLE_UUID_TYPE_BLE,
                                                           status_char_uuid,
                                                           status_char_max_len,
                                                           &_context.status,
                                                           CharacteristicInUseType::READ_NOTIFY,
                                                           true);

    if(result::Result::OK != status_char_add_result)
    {
        return status_char_add_result;
    }

    const auto pairing_char_add_result = add_characteristic(BLE_UUID_TYPE_BLE,
                                                            pairing_char_uuid,
                                                            pairing_char_max_len,
                                                            &_context.pairer,
                                                            CharacteristicInUseType::WRITE,
                                                            true);

    if(result::Result::OK != pairing_char_add_result)
    {
        return pairing_char_add_result;
    }

    _context.conn_handle = BLE_CONN_HANDLE_INVALID;

    return result::Result::OK;
}

void FtsService::reset_context()
{
    _context.pending_command = ControlPointOpcode::IDLE;
    _context.active_command = ControlPointOpcode::IDLE;
}

void FtsService::print_handles(FtsService::Context& ctx)
{
    NRF_LOG_INFO("%x %x %x %x", ctx.buffer_canary_1, ctx.buffer_canary_2, ctx.buffer_canary_3, ctx.buffer_canary_4);
}

void FtsService::on_write(ble_evt_t const* p_ble_evt, ClientContext& client_context)
{
    print_handles(_context);
    ble_gatts_evt_write_t const* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    _context.client_context = &client_context;
    if(p_evt_write->handle == _context.control_point_handles.value_handle)
    {
        on_control_point_write(p_evt_write->len, p_evt_write->data);
    }
    else if((p_evt_write->handle == _context.files_list.cccd_handle) && (p_evt_write->len == 2))
    {
        client_context.is_file_list_notifications_enabled =
            ble_srv_is_notification_enabled(p_evt_write->data);
    }
    else if((p_evt_write->handle == _context.files_list_next.cccd_handle) &&
            (p_evt_write->len == 2))
    {
        client_context.is_file_list_next_notifications_enabled =
            ble_srv_is_notification_enabled(p_evt_write->data);
    }
    else if((p_evt_write->handle == _context.file_info.cccd_handle) && (p_evt_write->len == 2))
    {
        client_context.is_file_info_notifications_enabled =
            ble_srv_is_notification_enabled(p_evt_write->data);
    }
    else if((p_evt_write->handle == _context.file_data.cccd_handle) && (p_evt_write->len == 2))
    {
        client_context.is_file_data_notifications_enabled =
            ble_srv_is_notification_enabled(p_evt_write->data);
    }
    else if((p_evt_write->handle == _context.fs_status.cccd_handle) && (p_evt_write->len == 2))
    {
        client_context.is_fs_status_notifications_enabled =
            ble_srv_is_notification_enabled(p_evt_write->data);
    }
    else if((p_evt_write->handle == _context.status.cccd_handle) && (p_evt_write->len == 2))
    {
        client_context.is_status_notifications_enabled =
            ble_srv_is_notification_enabled(p_evt_write->data);
    }
    else if (p_evt_write->handle == _context.pairer.value_handle)
    {
        on_pairer_write(p_evt_write->len, p_evt_write->data);
    }
    else
    {
        // Ignore, but it's kept here for the cases of debugging (in particular for porting to other client platforms)
        NRF_LOG_WARNING("write to unknown char 0x%x with len %d", p_evt_write->handle, p_evt_write->len);
    }
}

void FtsService::on_pairer_write(uint32_t len, const uint8_t* data) 
{
    if(len == 0 || data == nullptr)
    {
        NRF_LOG_ERROR("pairer.write: wrong input");

        return;
    }
    if (data[0] == unpair_magic_value) 
    {
        NRF_LOG_ERROR("pairer.write: unpair requested");
        // Initiate an unpairing process
        _context.is_unpair_requested = true;
    }
}

void FtsService::on_control_point_write(const uint32_t len, const uint8_t* data)
{
    if(len == 0 || data == nullptr)
    {
        NRF_LOG_ERROR("cp.write: wrong input");

        return;
    }
    switch(data[0])
    {
    // TODO: add to all requests an immediate response in case of wrong parameters.
    case static_cast<int>(ControlPointOpcode::REQ_FILES_LIST): {
        on_req_files_list(len - 1);
        break;
    }
    case static_cast<int>(ControlPointOpcode::REQ_FILE_INFO): {
        on_req_file_info(len - 1, &data[1]);
        break;
    }
    case static_cast<int>(ControlPointOpcode::REQ_FILE_DATA): {
        on_req_file_data(len - 1, &data[1]);
        break;
    }
    case static_cast<int>(ControlPointOpcode::REQ_FS_STATUS): {
        on_req_fs_status(len - 1);
        break;
    }
    case static_cast<int>(ControlPointOpcode::REQ_RECEIVE_COMPLETE): {
        on_req_receive_complete(len - 1);
        break;
    }
    default: {
        NRF_LOG_ERROR("cp.write: wrong opcode");
        // TODO: send a response to the client in order to notify it about an error.
        break;
    }
    }
}

void FtsService::on_req_files_list(const uint32_t size)
{
    if(size != 0)
    {
        NRF_LOG_ERROR("cp.write: file list request wrong size");
        return;
    }
    _context.pending_command = FtsService::ControlPointOpcode::REQ_FILES_LIST;
}

file_id_type FtsService::get_file_id_from_raw(const uint8_t* data) const
{
    file_id_type result;
    memcpy(result.data, data, ble::fts::file_id_size);
    return result;
}

void FtsService::on_req_file_info(const uint32_t data_size, const uint8_t* file_id_data)
{
    if(data_size != file_id_size || file_id_data == nullptr)
    {
        NRF_LOG_ERROR("cp.write: wrong file id size");
        return;
    }
    const file_id_type file_id = get_file_id_from_raw(file_id_data);
    _transaction_ctx.file_id = file_id;
    _context.pending_command = FtsService::ControlPointOpcode::REQ_FILE_INFO;
}

void FtsService::on_req_file_data(const uint32_t data_size, const uint8_t* file_id_data)
{
    if(data_size != file_id_size || file_id_data == nullptr)
    {
        NRF_LOG_ERROR("cp.write: wrong file id size");
        return;
    }

    const file_id_type file_id = get_file_id_from_raw(file_id_data);
    _transaction_ctx.file_id = file_id;
    _context.pending_command = FtsService::ControlPointOpcode::REQ_FILE_DATA;
}

void FtsService::on_req_fs_status(const uint32_t size)
{
    if(size != 0)
    {
        NRF_LOG_ERROR("cp.write: fs stat request wrong size");
        return;
    }
    _context.pending_command = FtsService::ControlPointOpcode::REQ_FS_STATUS;
}

void FtsService::on_req_receive_complete(uint32_t size)
{
    if(size != 0)
    {
        NRF_LOG_ERROR("cp.write: rx complete requ wrong size");
        return;
    }
    _context.pending_command = FtsService::ControlPointOpcode::REQ_RECEIVE_COMPLETE;
}

void FtsService::on_connect(ble_evt_t const* p_ble_evt, ClientContext& client_context)
{
    _context.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

void FtsService::on_disconnect(ble_evt_t const* p_ble_evt, ClientContext& client_context)
{
    _context.conn_handle = BLE_CONN_HANDLE_INVALID;
}

void FtsService::on_ble_evt(ble_evt_t const* p_ble_evt, void* p_context)
{
    if((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ClientContext& client_context = *reinterpret_cast<ClientContext*>(p_context);
    switch(p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_CONNECTED: {
        instance().on_connect(p_ble_evt, client_context);
        break;
    }

    case BLE_GAP_EVT_DISCONNECTED: {
        instance().on_disconnect(p_ble_evt, client_context);
        break;
    }

    case BLE_GATTS_EVT_WRITE: {
        instance().on_write(p_ble_evt, client_context);
        break;
    }
    case BLE_GAP_EVT_DATA_LENGTH_UPDATE: {
        NRF_LOG_DEBUG("ble::fts::evt_data_len_upd");
        break;
    }
    case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST: {
        NRF_LOG_DEBUG("ble::fts::exch_mtu_req");
        break;
    }
    case BLE_GAP_EVT_CONN_PARAM_UPDATE: {
        // NRF_LOG_DEBUG("ble::fts::evt_conn_param_upd. id=%d, len=%d",
        //               p_ble_evt->header.evt_id,
        //               p_ble_evt->header.evt_len);
        // const auto& conn_param_upd_evt = p_ble_evt->evt.gap_evt.params.conn_param_update;

        // NRF_LOG_DEBUG("conn param event: min=%d, max=%d, lat=%d, timeout=%d",
        //               conn_param_upd_evt.conn_params.min_conn_interval,
        //               conn_param_upd_evt.conn_params.max_conn_interval,
        //               conn_param_upd_evt.conn_params.slave_latency,
        //               conn_param_upd_evt.conn_params.conn_sup_timeout);
        FtsService::_context.is_connection_params_request_needed = true;
        break;
    }
    case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
        instance().process_hvn_tx_callback();
        break;
    }
    default: {
        NRF_LOG_DEBUG("ble::fts::on_unk_evt::evt=%d", p_ble_evt->header.evt_id);
        break;
    }
    }
}

void FtsService::process_hvn_tx_callback()
{
    // 1. update the transaction context
    _transaction_ctx.idx += _transaction_ctx.packet_size;
    _transaction_ctx.update_next_packet_size();
    // 2. if no more data left to send -
    if(_transaction_ctx.packet_size == 0)
    {
        _context.pending_command = ControlPointOpcode::FINALIZE_TRANSACTION;
    }
    else
    {
        _context.pending_command = ControlPointOpcode::PUSH_DATA_PACKETS;
    }
}

void FtsService::process()
{
    if(_context.pending_command == FtsService::ControlPointOpcode::FINALIZE_TRANSACTION)
    {
        if(_context.active_command == FtsService::ControlPointOpcode::REQ_FILE_DATA)
        {
            _transaction_ctx.file_sent_size += _transaction_ctx.size;
            if(_transaction_ctx.file_sent_size == _transaction_ctx.file_size)
            {
                _context.active_command = FtsService::ControlPointOpcode::IDLE;
                _context.pending_command = FtsService::ControlPointOpcode::IDLE;
                const auto close_result = _fs_if.file_close_function(_transaction_ctx.file_id);
                if(result::Result::OK != close_result)
                {
                    NRF_LOG_ERROR("failed to close file after transferring data");
                }
                else
                {
                    NRF_LOG_DEBUG("file transaction completed");
                }
            }
            else
            {
                const auto continue_result = continue_sending_file_data();
                if(result::Result::OK != continue_result)
                {
                    _context.active_command = FtsService::ControlPointOpcode::IDLE;
                    NRF_LOG_ERROR("failed to continue sending file data");
                }
                _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            }
        }
        else if(_context.active_command == FtsService::ControlPointOpcode::REQ_FILES_LIST ||
                _context.active_command == FtsService::ControlPointOpcode::REQ_FILES_LIST_NEXT)
        {
            NRF_LOG_DEBUG("finalize called on req files list. left: %d files",
                          _transaction_ctx.files_count_left);
            if(_transaction_ctx.files_count_left > 0)
            {
                const auto continue_result = continue_sending_files_list();
                if(result::Result::OK != continue_result)
                {
                    _context.active_command = FtsService::ControlPointOpcode::IDLE;
                    NRF_LOG_ERROR("failed to continue sending file list");
                }
                _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            }
            else
            {
                _context.active_command = FtsService::ControlPointOpcode::IDLE;
                _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            }
        }
        else
        {
            _context.active_command = FtsService::ControlPointOpcode::IDLE;
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
        }
        return;
    }
    else if(_context.pending_command == FtsService::ControlPointOpcode::PUSH_DATA_PACKETS)
    {
        const auto push_result = push_data_packets(_context.active_command);
        if(result::Result::OK != push_result)
        {
            NRF_LOG_ERROR("ble::fts: failed to continue data pushing");
            _context.active_command = FtsService::ControlPointOpcode::IDLE;
        }
        _context.pending_command = FtsService::ControlPointOpcode::IDLE;
        return;
    }
    else if(_context.pending_command != FtsService::ControlPointOpcode::IDLE &&
            is_command_a_request(_context.active_command))
    {
        NRF_LOG_ERROR("ble::fts: command requested before previous command has been processed (%d active, %d pending)", _context.active_command, _context.pending_command);
        _context.pending_command = FtsService::ControlPointOpcode::IDLE;
    }
    else
    {
        process_client_request(_context.pending_command);
    }

    if(_context.is_connection_params_request_needed &&
       (_context.current_timestamp - _context.last_connection_params_request_timestamp) >=
           Context::min_connection_params_change_request_time)
    {
        _context.is_connection_params_request_needed = false;
        ble_gap_conn_params_t gap_conn_params;
        gap_conn_params.min_conn_interval = 12;
        gap_conn_params.max_conn_interval = 20;
        gap_conn_params.conn_sup_timeout = 200;
        gap_conn_params.slave_latency = 0;
        const auto res = sd_ble_gap_conn_param_update(_context.conn_handle, &gap_conn_params);
        if(NRF_SUCCESS != res)
        {
            NRF_LOG_ERROR("failed to update connection parameters");
        }
        _context.last_connection_params_request_timestamp = _context.current_timestamp;
    }

    _context.current_timestamp += Context::process_period;
}

void FtsService::process_client_request(ControlPointOpcode client_request)
{
    switch(client_request)
    {
    case FtsService::ControlPointOpcode::REQ_FILES_LIST: {
        NRF_LOG_DEBUG("ble::fts::processing files' list request");
        if(_context.client_context == nullptr ||
           !_context.client_context->is_file_list_notifications_enabled ||
           !_context.client_context->is_file_list_next_notifications_enabled)
        {
            NRF_LOG_ERROR("ble::fts::files_list: notifications are not enabled. Aborting");
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }

        const auto result = send_files_list();
        if(result != result::Result::OK)
        {
            NRF_LOG_ERROR("ble::fts::file_list send failed");
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }
        _context.active_command = _context.pending_command;
        _context.pending_command = FtsService::ControlPointOpcode::IDLE;
        break;
    };
    case FtsService::ControlPointOpcode::REQ_FILE_INFO: {
        NRF_LOG_DEBUG("ble::fts::processing file info request");
        if(_context.client_context == nullptr ||
           !_context.client_context->is_file_info_notifications_enabled)
        {
            NRF_LOG_ERROR("ble::fts::file info: notifications are not enabled. Aborting");
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }

        const auto result = send_file_info();
        if(result != result::Result::OK)
        {
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }
        _context.active_command = _context.pending_command;
        _context.pending_command = FtsService::ControlPointOpcode::IDLE;
        break;
    }
    case FtsService::ControlPointOpcode::REQ_FILE_DATA: {
        NRF_LOG_DEBUG("ble::fts::processing file data request");
        if(_context.client_context == nullptr ||
           !_context.client_context->is_file_data_notifications_enabled)
        {
            NRF_LOG_ERROR("ble::fts::file data: notifications are not enabled. Aborting");
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }

        const auto result = send_file_data();
        if(result != result::Result::OK)
        {
            NRF_LOG_ERROR("ble::fts::file data: failed");
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }
        _context.active_command = _context.pending_command;
        _context.pending_command = FtsService::ControlPointOpcode::IDLE;
        break;
    }
    case FtsService::ControlPointOpcode::REQ_FS_STATUS: {
        NRF_LOG_DEBUG("ble::fts::processing FS status request");
        if(_context.client_context == nullptr ||
           !_context.client_context->is_fs_status_notifications_enabled)
        {
            NRF_LOG_ERROR("ble::fts::fs status: notifications are not enabled. Aborting");
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }

        const auto result = send_fs_status();
        if(result != result::Result::OK)
        {
            NRF_LOG_ERROR("ble::fts::fs status send failed");
            _context.pending_command = FtsService::ControlPointOpcode::IDLE;
            return;
        }
        _context.active_command = FtsService::ControlPointOpcode::IDLE;
        _context.pending_command = FtsService::ControlPointOpcode::IDLE;
        break;
    }
    case FtsService::ControlPointOpcode::REQ_RECEIVE_COMPLETE: 
    {
        _fs_if.receive_completed_function();
        _context.active_command = _context.pending_command;
        _context.pending_command = FtsService::ControlPointOpcode::IDLE;
        _context.is_disconnect_requested = true;
        break;
    }
    default:
        break;
    }
}

result::Result FtsService::send_files_list()
{
    uint32_t count{0};
    file_id_type files_list[files_list_max_count * file_id_size]{{0}};
    const auto fs_call = _fs_if.file_list_get_function(count, files_list);
    if(result::Result::OK != fs_call)
    {
        NRF_LOG_ERROR("ble::fts: FS files' list getter has failed");
        (void)update_general_status(GeneralStatus::FS_CORRUPT, file_id_type());
        return result::Result::ERROR_GENERAL;
    }
    NRF_LOG_DEBUG("files list: count=%d, max_count=%d", count, files_list_max_count);

    _transaction_ctx.files_count_left =
        (count > files_list_max_count) ? count - files_list_max_count : 0;

    const auto count_per_this_transaction = std::min(count, files_list_max_count);

    // Serialize the files list.
    // First 8 bytes -> files' count (N)
    _transaction_ctx.idx = 0;
    memcpy(_transaction_ctx.buffer, &count, sizeof(count));
    _transaction_ctx.idx += sizeof(count);
    memset(&_transaction_ctx.buffer[_transaction_ctx.idx], 0, sizeof(uint32_t));
    _transaction_ctx.idx += sizeof(uint32_t);
    // After that N elements, 8 bytes each.
    for(auto i = 0U; i < count_per_this_transaction; ++i)
    {
        memcpy(
            &_transaction_ctx.buffer[_transaction_ctx.idx], &files_list[i], sizeof(file_id_type));
        _transaction_ctx.idx += sizeof(file_id_type);
    }

    _transaction_ctx.size = _transaction_ctx.idx;
    _transaction_ctx.idx = 0;

    const auto push_result = push_data_packets(ControlPointOpcode::REQ_FILES_LIST);
    if(push_result != result::Result::OK)
    {
        NRF_LOG_ERROR("ble::fts::send: push has failed");
    }

    return result::Result::OK;
}

result::Result FtsService::continue_sending_files_list()
{
    uint32_t count{0};

    file_id_type files_list[files_list_max_count * sizeof(file_id_type)]{{0}};
    const auto fs_call = _fs_if.file_list_get_next_function(count, files_list);
    if(result::Result::OK != fs_call)
    {
        NRF_LOG_ERROR("ble::fts: FS files' list next getter has failed");
        (void)update_general_status(GeneralStatus::FS_CORRUPT, file_id_type());
        return result::Result::ERROR_GENERAL;
    }

    _transaction_ctx.files_count_left = (_transaction_ctx.files_count_left > count)
                                            ? (_transaction_ctx.files_count_left - count)
                                            : 0;

    NRF_LOG_DEBUG("continue sending files list is called. left: %d, count: %d",
                  _transaction_ctx.files_count_left,
                  count);

    _transaction_ctx.idx = 0;
    for(auto i = 0U; i < count; ++i)
    {
        memcpy(
            &_transaction_ctx.buffer[_transaction_ctx.idx], &files_list[i], sizeof(file_id_type));
        _transaction_ctx.idx += sizeof(file_id_type);
    }

    _transaction_ctx.size = _transaction_ctx.idx;
    _transaction_ctx.idx = 0;

    const auto push_result = push_data_packets(ControlPointOpcode::REQ_FILES_LIST_NEXT);
    if(push_result != result::Result::OK)
    {
        NRF_LOG_ERROR("ble::fts::send: push has failed");
    }

    return result::Result::OK;
}

result::Result FtsService::send_file_info()
{
    static constexpr size_t json_size_field_size{2};

    // TODO: add a signalling in case if the file doesn't exist.
    const auto file_info_call_result =
        _fs_if.file_info_get_function(_transaction_ctx.file_id,
                                      &_transaction_ctx.buffer[json_size_field_size],
                                      _transaction_ctx.size,
                                      TransactionContext::buffer_size);

    if (result::Result::ERROR_NOT_FOUND == file_info_call_result)
    {
        NRF_LOG_ERROR("ble::fts::send_info: file not found");
        (void)update_general_status(GeneralStatus::FILE_NOT_FOUND, _transaction_ctx.file_id);
        return file_info_call_result;
    }
    else if(result::Result::OK != file_info_call_result)
    {
        NRF_LOG_ERROR("ble::fts::send_info: error (id %x%x%x%x)", 
            _transaction_ctx.file_id.data[4], _transaction_ctx.file_id.data[5], 
            _transaction_ctx.file_id.data[6], _transaction_ctx.file_id.data[7]);
        (void)update_general_status(GeneralStatus::FS_CORRUPT, file_id_type());
        return file_info_call_result;
    }

    _transaction_ctx.idx = 0;
    _transaction_ctx.buffer[_transaction_ctx.size + json_size_field_size] = 0;
    _transaction_ctx.buffer[0] = static_cast<uint8_t>(_transaction_ctx.size & 0xFF);
    _transaction_ctx.buffer[1] = static_cast<uint8_t>((_transaction_ctx.size >> 8) & 0xFF);

    _transaction_ctx.size += json_size_field_size;

    const auto push_result = push_data_packets(ControlPointOpcode::REQ_FILE_INFO);
    if(push_result == result::Result::OK)
    {
        NRF_LOG_DEBUG("ble::fts::send_info: sending %d bytes", _transaction_ctx.size);
    }
    else
    {
        NRF_LOG_ERROR("ble::fts::send_info: push has failed");
    }
    return result::Result::OK;
}

result::Result FtsService::send_file_data()
{
    // 1. Open the requested file
    uint32_t file_size{0};
    const auto open_result = _fs_if.file_open_function(_transaction_ctx.file_id, file_size);
    if(result::Result::OK != open_result)
    {
        (void)update_general_status(GeneralStatus::FILE_NOT_FOUND, _transaction_ctx.file_id);
        return open_result;
    }

    NRF_LOG_DEBUG("ble::fts::file size %d bytes", file_size);

    // 2. Fill in the file size data to the transaction context
    _transaction_ctx.idx = 0;
    _transaction_ctx.file_size = file_size;

    // 3. Read out first buffer from the file to the buffer
    const auto read_result = _fs_if.file_data_get_function(_transaction_ctx.file_id,
                                                           _transaction_ctx.buffer,
                                                           _transaction_ctx.size,
                                                           TransactionContext::buffer_size);
    if(result::Result::OK != read_result)
    {
        NRF_LOG_ERROR("ble::fts::send_data: FS read failed");
        (void)update_general_status(GeneralStatus::FS_CORRUPT, file_id_type());
        return read_result;
    }

    _transaction_ctx.idx = 0;
    _transaction_ctx.file_sent_size = 0;

    // 4. Kick off data packets push
    const auto push_result = push_data_packets(ControlPointOpcode::REQ_FILE_DATA);
    if(push_result != result::Result::OK)
    {
        NRF_LOG_ERROR("ble::fts::send_data: push has failed");
    }

    return push_result;
}

result::Result FtsService::continue_sending_file_data()
{
    const auto read_result = _fs_if.file_data_get_function(_transaction_ctx.file_id,
                                                           _transaction_ctx.buffer,
                                                           _transaction_ctx.size,
                                                           TransactionContext::buffer_size);
    if(result::Result::OK != read_result)
    {
        NRF_LOG_ERROR("ble::fts::fs read has failed");
        return read_result;
    }

    // FIXME: investigate why file system now hands back chunks of size 0
    if(_transaction_ctx.size == 0)
    {
        // we allow this issue to only happen several times, after which a reset should be issued.
        static uint32_t fs_issue_max_counter{5};
        NRF_LOG_WARNING("FS issue has happened again. Stuffing the end of the file with zeros");
        const auto diff = std::min(_transaction_ctx.file_size - _transaction_ctx.file_sent_size,
                                   static_cast<uint32_t>(TransactionContext::buffer_size));
        memset(_transaction_ctx.buffer, 0, diff);
        _transaction_ctx.size = diff;

        --fs_issue_max_counter;
        if (fs_issue_max_counter == 0)
        {
            // TODO: move this dependency to app-specific noinit module somewhere else.
            using namespace noinit;
            auto& nim = NoInitMemory::instance();
            nim.reset_reason = ResetReason::FILE_SYSTEM_ERROR_DURING_BLE;
            nim.reset_count++;
            NoInitMemory::store();
            NVIC_SystemReset();
        }
    }
    _transaction_ctx.idx = 0;
    _transaction_ctx.update_next_packet_size();
    auto push_result = push_data_packets(ControlPointOpcode::REQ_FILE_DATA);

    if(push_result != result::Result::OK)
    {
        NRF_LOG_ERROR("ble::fts::cont_send_data: push has failed");
        return result::Result::ERROR_GENERAL;
    }

    return result::Result::OK;
}

result::Result FtsService::send_fs_status()
{
    // TODO: add a signalling in case if the file doesn't exist.
    FileSystemInterface::FSStatus fs_status{};
    const auto fs_status_result = _fs_if.fs_status_function(fs_status);

    if(result::Result::OK != fs_status_result)
    {
        NRF_LOG_ERROR("ble::fts::send fs status failed");
        (void)update_general_status(GeneralStatus::FS_CORRUPT, file_id_type());
        return fs_status_result;
    }

    _transaction_ctx.idx = 0;
    // NB: this is the place where problems can arise, if status structure is modified
    constexpr uint32_t transaction_size{2 + sizeof(FileSystemInterface::FSStatus)};
    _transaction_ctx.buffer[0] = static_cast<uint8_t>(transaction_size);
    _transaction_ctx.buffer[1] = static_cast<uint8_t>(transaction_size >> 8);
    memcpy(&_transaction_ctx.buffer[2], &fs_status, sizeof(fs_status));

    _transaction_ctx.size = transaction_size;

    const auto push_result = push_data_packets(ControlPointOpcode::REQ_FS_STATUS);
    if(push_result == result::Result::OK)
    {
        NRF_LOG_DEBUG("ble::fts::fs_status: sending %d bytes", _transaction_ctx.size);
    }
    else
    {
        NRF_LOG_ERROR("ble::fts::fs_status: push has failed");
    }
    return result::Result::OK;
}

result::Result FtsService::update_general_status(GeneralStatus status,
                                                 ble::fts::file_id_type parameter)
{
    _transaction_ctx.buffer[0] = static_cast<uint8_t>(status);
    if(status == GeneralStatus::TRANSACTION_ABORTED)
    {
        _transaction_ctx.buffer[1] = parameter.data[0];
        _transaction_ctx.size = 2;
    }
    else if(status == GeneralStatus::FILE_NOT_FOUND)
    {
        memcpy(&_transaction_ctx.buffer[1], &parameter, sizeof(parameter));
        _transaction_ctx.size = 1 + sizeof(parameter);
    }
    else
    {
        _transaction_ctx.size = 1;
    }
    _transaction_ctx.idx = 0;
    _transaction_ctx.update_next_packet_size();

    const auto push_result = push_data_packets(ControlPointOpcode::GENERAL_STATUS);
    if(push_result == result::Result::OK)
    {
        NRF_LOG_DEBUG("ble::fts::status: sending %d bytes", _transaction_ctx.size);
    }
    else
    {
        NRF_LOG_ERROR("ble::fts::status: push has failed");
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

struct HvxError
{
    uint16_t code;
    char* description;
};

static const HvxError hvx_error_decoder[] = {
    {0xFFFE, "unknown"},
    {NRF_SUCCESS, "SUCCESS"},
    {BLE_ERROR_INVALID_CONN_HANDLE, "BLE_ERROR_INVALID_CONN_HANDLE"},
    {NRF_ERROR_INVALID_STATE, "NRF_ERROR_INVALID_STATE"},
    {NRF_ERROR_INVALID_ADDR, "NRF_ERROR_INVALID_ADDR"},
    {NRF_ERROR_INVALID_PARAM, "NRF_ERROR_INVALID_PARAM"},
    {BLE_ERROR_INVALID_ATTR_HANDLE, "BLE_ERROR_INVALID_ATTR_HANDLE"},
    {BLE_ERROR_GATTS_INVALID_ATTR_TYPE, "BLE_ERROR_GATTS_INVALID_ATTR_TYPE"},
    {NRF_ERROR_NOT_FOUND, "NRF_ERROR_NOT_FOUND"},
    {NRF_ERROR_FORBIDDEN, "NRF_ERROR_FORBIDDEN"},
    {NRF_ERROR_DATA_SIZE, "NRF_ERROR_DATA_SIZE"},
    {NRF_ERROR_BUSY, "NRF_ERROR_BUSY"},
    {BLE_ERROR_GATTS_SYS_ATTR_MISSING, "BLE_ERROR_GATTS_SYS_ATTR_MISSING"},
    {NRF_ERROR_RESOURCES, "NRF_ERROR_RESOURCES"},
    {NRF_ERROR_TIMEOUT, "NRF_ERROR_TIMEOUT"},
};

char* get_hvx_error_description(uint16_t code)
{
    int i = 0;
    for(i = 0; i < sizeof(hvx_error_decoder) / sizeof(hvx_error_decoder[0]); ++i)
    {
        if(code == hvx_error_decoder[i].code)
        {
            return hvx_error_decoder[i].description;
        }
    }
    return hvx_error_decoder[0].description;
}

result::Result FtsService::push_data_packets(ControlPointOpcode opcode)
{
    _transaction_ctx.update_next_packet_size();

    ble_gatts_hvx_params_t hvx_params;

    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle =
        (opcode == ControlPointOpcode::REQ_FILES_LIST)  ? _context.files_list.value_handle
        : (opcode == ControlPointOpcode::REQ_FILE_INFO) ? _context.file_info.value_handle
        : (opcode == ControlPointOpcode::REQ_FILE_DATA) ? _context.file_data.value_handle
        : (opcode == ControlPointOpcode::REQ_FS_STATUS) ? _context.fs_status.value_handle
        : (opcode == ControlPointOpcode::REQ_FILES_LIST_NEXT)
            ? _context.files_list_next.value_handle
        : (opcode == ControlPointOpcode::GENERAL_STATUS) ? _context.status.value_handle
                                                         : BLE_CONN_HANDLE_INVALID;

    hvx_params.p_data = &_transaction_ctx.buffer[_transaction_ctx.idx];
    hvx_params.p_len = &_transaction_ctx.packet_size;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    const auto send_result = sd_ble_gatts_hvx(_context.conn_handle, &hvx_params);
    if(NRF_SUCCESS != send_result)
    {
        const auto error_description = get_hvx_error_description(send_result);
        NRF_LOG_ERROR(
            "sd_ble_gatts_hvx: %x (%s)", static_cast<int>(send_result), error_description);
        return send_result == NRF_ERROR_RESOURCES ? result::Result::ERROR_BUSY
                                                  : result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

} // namespace fts
} // namespace ble
