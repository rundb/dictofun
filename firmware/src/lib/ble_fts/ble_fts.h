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

    file_list_get_function_type file_list_get_function;
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
private:
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
    static constexpr uint32_t fs_info_char_uuid{0x1003};

    static constexpr uint32_t cp_char_max_len{16};

    struct ClientContext
    {
        bool is_notifications_enabled;
    };

    struct Context
    {
        uint8_t uuid_type;
        uint16_t service_handle;
        ble_gatts_char_handles_t control_point_handles;
        ble_gatts_char_handles_t rx_handles;

        uint16_t conn_handle;
        bool is_notification_enabled; 

        blcm_link_ctx_storage_t * const p_link_ctx_storage;
    };

    static constexpr uint8_t _max_clients{1};
    static constexpr uint32_t _link_ctx_size{sizeof(ClientContext)};
    static uint32_t _ctx_data_pool[_max_clients * _link_ctx_size];
    static blcm_link_ctx_storage_t _link_ctx_storage;

    ble_uuid128_t get_service_uuid128();

    result::Result add_characteristic(uint8_t type, uint32_t uuid, uint32_t max_len, ble_gatts_char_handles_t * handle);
    result::Result add_control_point_char();

public:
    static Context _context;
};

}
}