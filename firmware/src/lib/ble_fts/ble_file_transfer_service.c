#include "ble_file_transfer_service.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_gpio.h"
#include "sdk_common.h"
#include <stdio.h>

#define BLE_UUID_FTS_TX_CHARACTERISTIC              0x0003 /**< The UUID of the TX Characteristic. */
#define BLE_UUID_FTS_RX_CHARACTERISTIC              0x0002 /**< The UUID of the RX Characteristic. */
#define BLE_UUID_FTS_FILE_INFO_CHARACTERISTIC       0x0004
#define BLE_UUID_FTS_FILESYSTEM_INFO_CHARACTERISTIC 0x0005

#define BLE_FTS_MAX_RX_CHAR_LEN                                                                    \
    BLE_FTS_MAX_DATA_LEN /**< Maximum length of the RX Characteristic (in bytes). */
#define BLE_FTS_MAX_TX_CHAR_LEN                                                                    \
    BLE_FTS_MAX_DATA_LEN /**< Maximum length of the TX Characteristic (in bytes). */

// clang-format off
// generated on https://www.uuidgenerator.net/
// eb3caea4-0db1-11ec-82a8-0242ac130003
#define FTS_BASE_UUID                                                                          \
{                                                                                              \
    {                                                                                          \
        0xEB, 0x3C, 0xAE, 0xA4,                                                                \
        0x0D, 0xB1,                                                                            \ 
        0x11, 0xEC,                                                                            \
        0x82, 0xA8,                                                                            \
        0x02, 0x42, 0xAC, 0x13,0x00, 0x03                                                      \
    }                                                                                          \
}
// clang-format on

#include "nrf_log.h"

volatile uint32_t file_size = 0, file_pos = 0, m_max_data_length = 20;
uint8_t* file_data;
ble_fts_t* m_fts;


/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the S110 SoftDevice.
 *
 * @param[in] p_fts     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_connect(ble_fts_t* p_fts, ble_evt_t const* p_ble_evt)
{
    p_fts->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

/**@brief Function for handling the @ref BLE_GAP_EVT_DISCONNECTED event from the S110 SoftDevice.
 *
 * @param[in] p_fts     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_disconnect(ble_fts_t* p_fts, ble_evt_t const* p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_fts->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE event from the S110 SoftDevice.
 *
 * @param[in] p_fts     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write(ble_fts_t* p_fts, ble_evt_t const* p_ble_evt)
{
    ble_gatts_evt_write_t const* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if((p_evt_write->handle == p_fts->tx_handles.cccd_handle) && (p_evt_write->len == 2))
    {
        if(ble_srv_is_notification_enabled(p_evt_write->data))
        {
            p_fts->is_notification_enabled = true;
        }
        else
        {
            p_fts->is_notification_enabled = false;
        }
    }
    else if((p_evt_write->handle == p_fts->file_info_handles.cccd_handle) &&
            (p_evt_write->len == 2))
    {
        if(ble_srv_is_notification_enabled(p_evt_write->data))
        {
            p_fts->is_info_char_notification_enabled = true;
        }
        else
        {
            p_fts->is_info_char_notification_enabled = false;
        }
    }
    if((p_evt_write->handle == p_fts->filesystem_info_handles.cccd_handle) &&
            (p_evt_write->len == 2))
    {
        if(ble_srv_is_notification_enabled(p_evt_write->data))
        {
            p_fts->is_fsinfo_char_notification_enabled = true;
        }
        else
        {
            p_fts->is_fsinfo_char_notification_enabled = false;
        }
    }
    else if((p_evt_write->handle == p_fts->rx_handles.value_handle) &&
            (p_fts->data_handler != NULL))
    {
        p_fts->data_handler(p_fts, p_evt_write->data, p_evt_write->len);
    }
    else
    {
        // Do Nothing. This event is not relevant for this service.
    }
}

/**@brief Function for adding TX characteristic.
 *
 * @param[in] p_fts       Nordic UART Service structure.
 * @param[in] p_fts_init  Information needed to initialize the service.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t tx_char_add(ble_fts_t* p_fts, const ble_fts_init_t* p_fts_init)
{
    /**@snippet [Adding proprietary characteristic to S110 SoftDevice] */
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t attr_char_value;
    ble_uuid_t ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf = NULL;
    char_md.p_user_desc_md = NULL;
    char_md.p_cccd_md = &cccd_md;
    char_md.p_sccd_md = NULL;

    ble_uuid.type = p_fts->uuid_type;
    ble_uuid.uuid = BLE_UUID_FTS_TX_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = BLE_FTS_MAX_TX_CHAR_LEN;

    return sd_ble_gatts_characteristic_add(
        p_fts->service_handle, &char_md, &attr_char_value, &p_fts->tx_handles);
}

static uint32_t file_info_char_add(ble_fts_t* p_fts, const ble_fts_init_t* p_fts_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t attr_char_value;
    ble_uuid_t ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf = NULL;
    char_md.p_user_desc_md = NULL;
    char_md.p_cccd_md = &cccd_md;
    char_md.p_sccd_md = NULL;

    ble_uuid.type = p_fts->uuid_type;
    ble_uuid.uuid = BLE_UUID_FTS_FILE_INFO_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = 1 + sizeof(ble_fts_file_info_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = 16;

    return sd_ble_gatts_characteristic_add(
        p_fts->service_handle, &char_md, &attr_char_value, &p_fts->file_info_handles);
}

static uint32_t filesystem_info_char_add(ble_fts_t* p_fts, const ble_fts_init_t* p_fts_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t attr_char_value;
    ble_uuid_t ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf = NULL;
    char_md.p_user_desc_md = NULL;
    char_md.p_cccd_md = &cccd_md;
    char_md.p_sccd_md = NULL;

    ble_uuid.type = p_fts->uuid_type;
    ble_uuid.uuid = BLE_UUID_FTS_FILESYSTEM_INFO_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = 1 + sizeof(ble_fts_file_info_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = 16;

    return sd_ble_gatts_characteristic_add(
        p_fts->service_handle, &char_md, &attr_char_value, &p_fts->filesystem_info_handles);
}

/**@brief Function for adding RX characteristic.
 *
 * @param[in] p_fts       Nordic UART Service structure.
 * @param[in] p_fts_init  Information needed to initialize the service.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t rx_char_add(ble_fts_t* p_fts, const ble_fts_init_t* p_fts_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t attr_char_value;
    ble_uuid_t ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write = 1;
    char_md.char_props.write_wo_resp = 1;
    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf = NULL;
    char_md.p_user_desc_md = NULL;
    char_md.p_cccd_md = NULL;
    char_md.p_sccd_md = NULL;

    ble_uuid.type = p_fts->uuid_type;
    ble_uuid.uuid = BLE_UUID_FTS_RX_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = 1;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = BLE_FTS_MAX_RX_CHAR_LEN;

    return sd_ble_gatts_characteristic_add(
        p_fts->service_handle, &char_md, &attr_char_value, &p_fts->rx_handles);
}

static uint32_t push_data_packets()
{
    uint32_t return_code = NRF_SUCCESS;
    uint32_t packet_length = m_max_data_length;
    uint32_t packet_size = 0;
    while(return_code == NRF_SUCCESS)
    {
        if((file_size - file_pos) > packet_length)
        {
            packet_size = packet_length;
        }
        else if((file_size - file_pos) > 0)
        {
            packet_size = file_size - file_pos;
        }

        if(packet_size > 0)
        {
            return_code = ble_fts_string_send(m_fts, &file_data[file_pos], packet_size);
            if(return_code == NRF_SUCCESS)
            {
                file_pos += packet_size;
            }
        }
        else
        {
            file_size = 0;
            break;
        }
    }
    return return_code;
}

static volatile bool nrf_error_resources = false;

void ble_fts_on_ble_evt(ble_evt_t const* p_ble_evt, void* p_context)
{
    if((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_fts_t* p_fts = (ble_fts_t*)p_context;
    m_fts = p_fts;

    switch(p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_CONNECTED:
        NRF_LOG_INFO("on_evt: on_connect");
        on_connect(p_fts, p_ble_evt);
        break;

    case BLE_GAP_EVT_DISCONNECTED:
        NRF_LOG_INFO("on_evt: on_disconnect");
        on_disconnect(p_fts, p_ble_evt);
        break;

    case BLE_GATTS_EVT_WRITE:
        on_write(p_fts, p_ble_evt);
        break;

    case BLE_GATTS_EVT_HVN_TX_COMPLETE: {
        //uint32_t count = p_ble_evt->evt.gatts_evt.params.hvn_tx_complete.count;
        if(file_size > 0)
        {
            push_data_packets();
        }
        nrf_error_resources = false;
    }
    break;

    default:
        // No implementation needed.
        break;
    }
}

uint32_t ble_fts_init(ble_fts_t* p_fts, const ble_fts_init_t* p_fts_init)
{
    uint32_t err_code;
    ble_uuid_t ble_uuid;
    ble_uuid128_t fts_base_uuid = FTS_BASE_UUID;

    VERIFY_PARAM_NOT_NULL(p_fts);
    VERIFY_PARAM_NOT_NULL(p_fts_init);

    // Initialize the service structure.
    p_fts->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_fts->data_handler = p_fts_init->data_handler;
    p_fts->is_notification_enabled = false;

    /**@snippet [Adding proprietary Service to S110 SoftDevice] */
    // Add a custom base UUID.
    err_code = sd_ble_uuid_vs_add(&fts_base_uuid, &p_fts->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_fts->uuid_type;
    ble_uuid.uuid = BLE_UUID_FTS_SERVICE;

    // Add the service.
    err_code =
        sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_fts->service_handle);
    /**@snippet [Adding proprietary Service to S110 SoftDevice] */
    VERIFY_SUCCESS(err_code);

    // Add the RX Characteristic.
    err_code = rx_char_add(p_fts, p_fts_init);
    VERIFY_SUCCESS(err_code);

    // Add the TX Characteristic.
    err_code = tx_char_add(p_fts, p_fts_init);
    VERIFY_SUCCESS(err_code);

    // Add the File Info Characteristic.
    err_code = file_info_char_add(p_fts, p_fts_init);
    VERIFY_SUCCESS(err_code);

    err_code = filesystem_info_char_add(p_fts, p_fts_init);
    VERIFY_SUCCESS(err_code);

    return NRF_SUCCESS;
}

uint32_t ble_fts_string_send(ble_fts_t* p_fts, uint8_t* p_string, uint16_t length)
{
    ble_gatts_hvx_params_t hvx_params;
    uint32_t err_code;

    if(nrf_error_resources)
    {
        return NRF_ERROR_RESOURCES;
    }

    VERIFY_PARAM_NOT_NULL(p_fts);

    if((p_fts->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_fts->is_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if(length > BLE_FTS_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = p_fts->tx_handles.value_handle;
    hvx_params.p_data = p_string;
    hvx_params.p_len = &length;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    err_code = sd_ble_gatts_hvx(p_fts->conn_handle, &hvx_params);

    if(err_code == NRF_ERROR_RESOURCES)
    {
        nrf_error_resources = true;
    }
    if((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_RESOURCES))
    {
        NRF_LOG_INFO("NOT - Len: %i, err_code: 0x%X (%d)", length, err_code, err_code);
    }
    return err_code;
}

uint32_t ble_fts_file_info_send(ble_fts_t* p_fts, ble_fts_file_info_t* file_info)
{
    uint8_t data_buf[1 + sizeof(ble_fts_file_info_t)];
    ble_gatts_hvx_params_t hvx_params;

    VERIFY_PARAM_NOT_NULL(p_fts);

    if((p_fts->conn_handle == BLE_CONN_HANDLE_INVALID) ||
       (!p_fts->is_info_char_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }

    uint16_t length = 1 + sizeof(ble_fts_file_info_t);

    data_buf[0] = 1;
    memcpy(&data_buf[1], file_info, sizeof(ble_fts_file_info_t));

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_fts->file_info_handles.value_handle;
    hvx_params.p_data = data_buf;
    hvx_params.p_len = &length;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_fts->conn_handle, &hvx_params);
}

uint32_t ble_fts_filesystem_info_send(ble_fts_t* p_fts, ble_fts_filesystem_info_t* fs_info)
{
    uint8_t data_buf[1 + sizeof(ble_fts_filesystem_info_t)];
    ble_gatts_hvx_params_t hvx_params;

    VERIFY_PARAM_NOT_NULL(p_fts);

    if((p_fts->conn_handle == BLE_CONN_HANDLE_INVALID) ||
       (!p_fts->is_fsinfo_char_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }

    uint16_t length = 1 + sizeof(ble_fts_filesystem_info_t);

    data_buf[0] = 1;
    memcpy(&data_buf[1], fs_info, sizeof(ble_fts_filesystem_info_t));

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_fts->filesystem_info_handles.value_handle;
    hvx_params.p_data = data_buf;
    hvx_params.p_len = &length;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_fts->conn_handle, &hvx_params);
}

uint32_t ble_fts_send_file_fragment(ble_fts_t* p_fts, uint8_t* p_data, uint32_t data_length)
{
    uint32_t err_code;

    if((p_fts->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_fts->is_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if(file_size != 0)
    {
        return NRF_ERROR_BUSY;
    }

    err_code = ble_fts_string_send(p_fts, p_data, data_length);
    return err_code;
}

bool ble_fts_file_transfer_busy(void)
{
    return file_size != 0;
}
