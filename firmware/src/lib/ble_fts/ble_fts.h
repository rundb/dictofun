// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include <stdint.h>
#include <functional>
#include "result.h"
#include "ble_link_ctx_manager.h"

namespace ble
{
namespace fts
{

using file_id_type = uint64_t;

/// @brief This structure provides a glue to the file system
struct FileSystemInterface
{
    using file_list_get_function_type = std::function<result::Result (uint32_t&, file_id_type *)>;
    // file data is a minimal json string describing the contents of a particular file
    using file_info_get_function_type = std::function<result::Result (file_id_type, uint8_t *, uint32_t&, uint32_t)>;
    using file_open_function_type = std::function<result::Result (file_id_type, uint32_t&)>;
    using file_close_function_type = std::function<result::Result (file_id_type)>;
    using file_data_get_function_type = std::function<result::Result (file_id_type, uint8_t *, uint32_t&, uint32_t)>;

    file_list_get_function_type file_list_get_function;
    file_info_get_function_type file_info_get_function;
    file_open_function_type file_open_function;
    file_close_function_type file_close_function;
    file_data_get_function_type file_data_get_function;
};

// TODO: consider replacing the glue structures above with a template
class FtsService
{
public:
    explicit FtsService(FileSystemInterface& fs_if);
    FtsService() = delete;
    FtsService(const FtsService&) = delete;
    FtsService(FtsService&&) = delete;
    FtsService& operator=(const FtsService&) = delete;
    FtsService& operator=(FtsService&&) = delete;
    ~FtsService() = default;

    result::Result init();
    constexpr uint32_t get_service_uuid() { return service_uuid;}
    uint8_t get_service_uuid_type() { return _context.uuid_type; }
    static FtsService& instance() { return *_instance;}
    static void on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

    void process();
private:
    static FtsService * _instance;
    FileSystemInterface& _fs_if;

    static constexpr uint32_t _uuid_size{16};
    // a045a112-b822-4820-8782-bd8faf68807b
    static constexpr uint8_t _service_uuid_base[_uuid_size] {
        0x7b, 0x80, 0x68, 0xaf, 0x8f, 0xbd, 0x82, 0x87,
        0x20, 0x48, 0x22, 0xb8, 0x12, 0xa1, 0x45, 0xa0
    };

    static constexpr uint32_t service_uuid{0x1001};
    static constexpr uint32_t service_uuid_type{BLE_UUID_TYPE_VENDOR_BEGIN};
    static constexpr uint32_t control_point_char_uuid{0x1002};
    static constexpr uint32_t file_list_char_uuid{0x1003};
    static constexpr uint32_t file_info_char_uuid{0x1004};
    static constexpr uint32_t file_data_char_uuid{0x1005};

    static constexpr uint32_t cp_char_max_len{9};
    static constexpr uint32_t file_list_char_max_len{32};
    static constexpr uint32_t file_info_char_max_len{32};
    static constexpr uint32_t file_data_char_max_len{128};

    // TODO: separate BLE commands from internal states
    enum class ControlPointOpcode: uint8_t
    {
        REQ_FILES_LIST = 1,
        REQ_FILE_INFO = 2,
        REQ_FILE_DATA = 3,
        REQ_FS_STATUS = 4,
        PUSH_DATA_PACKETS = 250,
        FINALIZE_TRANSACTION = 251,
        IDLE = 254,
    };
    bool is_command_a_request(const ControlPointOpcode opcode) const
    {
        return ControlPointOpcode::REQ_FILES_LIST == opcode ||
               ControlPointOpcode::REQ_FILE_INFO == opcode ||
               ControlPointOpcode::REQ_FILE_DATA == opcode ||
               ControlPointOpcode::REQ_FS_STATUS == opcode;
    }
    struct ClientContext
    {
        bool is_file_list_notifications_enabled{false};
        bool is_file_data_notifications_enabled{false};
        bool is_file_info_notifications_enabled{false};
        bool is_fs_status_notifications_enabled{false};
    };

    struct Context
    {
        uint8_t uuid_type;
        uint16_t service_handle;
        ble_gatts_char_handles_t control_point_handles;
        ble_gatts_char_handles_t files_list;
        ble_gatts_char_handles_t file_info;
        ble_gatts_char_handles_t file_data;

        uint16_t conn_handle;
        bool is_notification_enabled; 

        blcm_link_ctx_storage_t * const p_link_ctx_storage;

        ControlPointOpcode pending_command{ControlPointOpcode::IDLE};
        ControlPointOpcode active_command{ControlPointOpcode::IDLE};
        ClientContext * client_context;
    };

    static constexpr uint8_t _max_clients{1};
    static constexpr uint32_t _link_ctx_size{sizeof(ClientContext)};
    static uint32_t _ctx_data_pool[_max_clients * _link_ctx_size];
    static blcm_link_ctx_storage_t _link_ctx_storage;

    ble_uuid128_t get_service_uuid128();

    enum class CharacteristicInUseType
    {
        WRITE,
        READ_NOTIFY,
    };
    result::Result add_characteristic(
        uint8_t type, 
        uint32_t uuid, 
        uint32_t max_len, 
        ble_gatts_char_handles_t * handle, 
        CharacteristicInUseType char_type);

    void on_write(ble_evt_t const * p_ble_evt, ClientContext& client_context);
    void on_connect(ble_evt_t const * p_ble_evt, ClientContext& client_context);
    void on_disconnect(ble_evt_t const * p_ble_evt, ClientContext& client_context);
    void on_control_point_write(uint32_t len, const uint8_t * data);

    void on_req_files_list(uint32_t size);
    void on_req_file_info(uint32_t data_size, const uint8_t * file_id_data);
    void on_req_file_data(uint32_t data_size, const uint8_t * file_id_data);
    void on_req_fs_status(uint32_t size);

    static constexpr uint32_t file_id_size{sizeof(uint64_t)};
    file_id_type get_file_id_from_raw(const uint8_t * data) const;

    // API for functions that initiate transfer of FS data
    result::Result send_files_list();
    result::Result send_file_info();
    result::Result send_file_data();
    result::Result continue_sending_file_data();

    void process_client_request(ControlPointOpcode client_request);

    struct TransactionContext {
        static constexpr size_t buffer_size{256};
        static constexpr uint16_t packet_size_value{16};
        uint32_t idx{0};
        uint32_t size{0};
        uint8_t buffer[buffer_size];
        uint16_t packet_size{packet_size_value}; // it's a constant, but for the API of Nordic SDK it has to be a var.
        file_id_type file_id{0};
        uint32_t file_size{0};
        uint32_t file_sent_size{0};
        void update_next_packet_size() 
        {
            const auto leftover_size{size-idx};
            packet_size = (leftover_size > packet_size_value) ? packet_size_value : leftover_size;
        }
    } _transaction_ctx;

    static constexpr uint32_t files_list_max_count{(TransactionContext::buffer_size - 8) / file_id_size};

    // trigger the BLE transaction specified by the opcode.
    result::Result push_data_packets(ControlPointOpcode opcode);

    void process_hvn_tx_callback();

public:
    static Context _context;
};

}
}