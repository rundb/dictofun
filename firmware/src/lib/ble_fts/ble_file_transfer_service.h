#pragma once
/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "ble_link_ctx_manager.h"
#include "app_util_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief   Macro for defining a ble_fts instance.
 *
 * @param     _name            Name of the instance.
 * @hideinitializer
 */
#define BLE_FTS_DEF(_name, _fts_max_clients)                      \
    BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(_name, _link_ctx_storage),  \
                             (_fts_max_clients),                  \
                             sizeof(ble_fts_client_context_t));   \
    static ble_fts_t _name = {0, 0, {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, \
                              0, 0, 0, 0,                            \
                              &CONCAT_2(_name, _link_ctx_storage)}; \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_NUS_BLE_OBSERVER_PRIO,               \
                         ble_fts_on_ble_evt,                      \
                         &_name)
    
#define BLE_UUID_FTS_SERVICE 0x0001                      /**< The UUID of the Nordic UART Service. */

#define OPCODE_LENGTH 1
#define HANDLE_LENGTH 2

#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_FTS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
#else
    #define BLE_FTS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
    #warning NRF_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif

/* Forward declaration of the ble_fts_t type. */
typedef struct ble_fts_s ble_fts_t;


/**@brief Nordic UART Service event handler type. */
typedef void (*ble_fts_data_handler_t) (ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length);

typedef struct
{
    bool is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
} ble_fts_client_context_t;

typedef struct
{
    ble_fts_data_handler_t data_handler;
} ble_fts_init_t;

struct ble_fts_s
{
    uint8_t                  uuid_type;               /**< UUID type for Nordic UART Service Base UUID. */
    uint16_t                 service_handle;          /**< Handle of Nordic UART Service (as provided by the SoftDevice). */
    ble_gatts_char_handles_t tx_handles;              /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t rx_handles;              /**< Handles related to the RX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t file_info_handles;
    ble_gatts_char_handles_t filesystem_info_handles;
    ble_gatts_char_handles_t pairing_char_handles;
    uint16_t                 conn_handle;             /**< Handle of the current connection (as provided by the SoftDevice). BLE_CONN_HANDLE_INVALID if not in a connection. */
    bool                     is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
    bool                     is_info_char_notification_enabled;
    bool                     is_fsinfo_char_notification_enabled;
    blcm_link_ctx_storage_t * const p_link_ctx_storage;
    ble_fts_data_handler_t   data_handler;            /**< Event handler to be called for handling received data. */
};

typedef struct
{
    uint32_t file_size_bytes;
    
}ble_fts_file_info_t;

typedef struct
{
    uint32_t valid_files_count;
}ble_fts_filesystem_info_t;

typedef PACKED_STRUCT
{
    uint16_t mtu;
    uint16_t con_interval;
    uint8_t  tx_phy;
    uint8_t  rx_phy;
}ble_fts_ble_params_info_t;

enum BleCommands
{
    CMD_EMPTY = 0,
    CMD_GET_FILE = 1,
    CMD_GET_FILE_INFO = 2,
    CMD_GET_VALID_FILES_COUNT = 3,
};

uint32_t ble_fts_init(ble_fts_t * p_fts, const ble_fts_init_t * p_fts_init);

void ble_fts_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

uint32_t ble_fts_send_array(ble_fts_t * p_fts, uint8_t * p_data, uint16_t length);

uint32_t ble_fts_file_info_send(ble_fts_t * p_fts, ble_fts_file_info_t * file_info);

uint32_t ble_fts_filesystem_info_send(ble_fts_t * p_fts, ble_fts_filesystem_info_t * fs_info);

uint32_t ble_fts_send_file(ble_fts_t * p_fts, uint8_t * p_data, uint32_t data_length, uint32_t max_packet_length);

uint32_t ble_fts_send_file_fragment(ble_fts_t * p_fts, uint8_t * p_data, uint32_t data_length);

bool ble_fts_file_transfer_busy(void);

uint8_t fts_get_uuid_type();

#ifdef __cplusplus
}
#endif

