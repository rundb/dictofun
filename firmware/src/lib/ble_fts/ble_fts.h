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

    void init();
private:
    FileSystemInterface& _fs_if;

    static constexpr uint32_t _uuid_size{16};
    // a045a112-b822-4820-8782-bd8faf68807b
    static constexpr uint8_t _service_uuid_base[_uuid_size] {
        0x7b, 0x80, 0x68, 0xaf, 0x8f, 0xbd, 0x82, 0x87,
        0x20, 0x48, 0x22, 0xb8, 0x12, 0xa1, 0x45, 0xa0
    };

    static constexpr uint8_t control_point_char_uuid{0x01};
    static constexpr uint8_t fs_info_char_uuid{0x02};

    struct ClientContext
    {
        bool is_notifications_enabled;
    };

    struct Context
    {
        uint8_t uuid_type;
        uint16_t service_handle;
        ble_gatts_char_handles_t tx_handles;
        ble_gatts_char_handles_t rx_handles;

        uint16_t conn_handle;
        bool is_notification_enabled; 

        blcm_link_ctx_storage_t * const p_link_ctx_storage;
    };

    static constexpr uint8_t _max_clients{1};
    static constexpr uint32_t _link_ctx_size{sizeof(ClientContext)};
    uint32_t _ctx_data_pool[_max_clients * _link_ctx_size];
    blcm_link_ctx_storage_t _link_ctx_storage =
    {
        .p_ctx_data_pool = _ctx_data_pool,
        .max_links_cnt   = _max_clients,
        .link_ctx_size   = sizeof(_ctx_data_pool)/_max_clients
    };

    Context _context {
        0, 0, 
        {0, 0, 0, 0,}, {0,0,0,0,},
        0, false,
        &_link_ctx_storage
    };
    // NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
    //                      BLE_NUS_BLE_OBSERVER_PRIO,               \
    //                      ble_its_on_ble_evt,                      \
    //                      &_name)

    // #define NRF_SDH_BLE_OBSERVER(_name, _prio, _handler, _context)                                      \
    // NRF_SECTION_SET_ITEM_REGISTER(sdh_ble_observers, _prio, static nrf_sdh_ble_evt_observer_t _name) =  \
    // {                                                                                                   \
    //     .handler   = _handler,                                                                          \
    //     .p_context = _context                                                                           \
    // }   

    // #define NRF_SECTION_SET_ITEM_REGISTER(_name, _priority, _var)                                       \
    // NRF_SECTION_ITEM_REGISTER(CONCAT_2(_name, _priority), _var)

    // #define NRF_SECTION_ITEM_REGISTER(section_name, section_var) \
    // section_var __attribute__ ((section("." STRINGIFY(section_name)))) __attribute__((used))

};

}
}