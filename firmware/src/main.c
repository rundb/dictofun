/**
 * Copyright (c) 2015 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @brief Blinky Sample Application main file.
 *
 * This file contains the source code for a sample server application using the LED Button service.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "boards.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_lbs.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "ble_name.h"
#include "ble_file_transfer_service.h"
#include "peer_manager.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "spi_access.h"
#include "drv_audio.h"

#define ADVERTISING_LED                 BSP_BOARD_LED_0                         /**< Is on when device is advertising. */
#define CONNECTED_LED                   BSP_BOARD_LED_1                         /**< Is on when device has connected. */
#define LEDBUTTON_LED                   BSP_BOARD_LED_2                         /**< LED to be toggled with the help of the LED Button Service. */
#define LEDBUTTON_BUTTON                BSP_BUTTON_0                            /**< Button that will trigger the notification event with the LED Button Service */

#define DEVICE_NAME                     BLE_DEVICE_NAME                         /**< Name of device. Will be included in the advertising data. */

#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_ADV_INTERVAL                1600                                      /**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */
#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   /**< The advertising time-out (in units of seconds). When set to 0, we will never time out. */


#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory time-out (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(20000)                  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                     /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define SEC_PARAM_BOND                  1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                       /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                      /**< Maximum encryption key size. */

#define LDO_EN_PIN  11
#define BUTTON_PIN  25

BLE_FTS_DEF(m_fts, NRF_SDH_BLE_TOTAL_LINK_COUNT);                               /**< BLE file transfer service instance. */
BLE_LBS_DEF(m_lbs);                                                             /**< LED Button Service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                   /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                    /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];         /**< Buffer for storing an encoded scan data. */

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = m_enc_scan_response_data,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX

    }
};

enum command_id
{
    CMD_EMPTY = 0,
    CMD_GET_FILE,
    CMD_GET_FILE_INFO,
};

static uint8_t m_cmd = CMD_EMPTY;
static uint32_t m_file_size = 0;
static const uint16_t m_ble_its_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static bool is_file_transmission_done = false;
uint32_t rec_start_ts = 0;

enum APP_SM_STATES
{
  APP_SM_REC_INIT = 10,
  APP_SM_RECORDING,
  APP_SM_CONN,
  APP_SM_XFER,

  APP_SM_FINALISE = 50,
  APP_SM_SHUTDOWN,

};

#define SPI_XFER_DATA_STEP 0x100
static enum APP_SM_STATES app_sm_state = APP_SM_REC_INIT;
drv_audio_frame_t * pending_frame = NULL;
size_t write_pointer = 0;

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by the application.
 */
static void leds_init(void)
{
    bsp_board_init(BSP_INIT_LEDS);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
APP_TIMER_DEF(timestamp_timer);
void timestamp_timer_timeout_handler(void * p_context) {}
static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    app_timer_create(&timestamp_timer, APP_TIMER_MODE_REPEATED, timestamp_timer_timeout_handler);
    err_code = app_timer_start(&timestamp_timer, 32768, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    ret_code_t    err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;

    ble_uuid_t adv_uuids[] = {{LBS_UUID_SERVICE, m_lbs.uuid_type}};

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;


    memset(&srdata, 0, sizeof(srdata));
    srdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    srdata.uuids_complete.p_uuids  = adv_uuids;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);

    ble_gap_adv_params_t adv_params;

    // Set advertising parameters.
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
    adv_params.duration        = APP_ADV_DURATION;
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.p_peer_addr     = NULL;
    adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    adv_params.interval        = APP_ADV_INTERVAL;

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling write events to the LED characteristic.
 *
 * @param[in] p_lbs     Instance of LED Button Service to which the write applies.
 * @param[in] led_state Written/desired state of the LED.
 */
static void led_write_handler(uint16_t conn_handle, ble_lbs_t * p_lbs, uint8_t led_state)
{
    if (led_state)
    {
        bsp_board_led_on(LEDBUTTON_LED);
        NRF_LOG_INFO("Received LED ON!");
    }
    else
    {
        bsp_board_led_off(LEDBUTTON_LED);
        NRF_LOG_INFO("Received LED OFF!");
    }
}

/**@brief Function for handling the data from the FTS Service.
 *
 * @details This function will process the data received from the FTS BLE Service.
 *
 * @param[in] p_its    FTS Service structure.
 * @param[in] p_data   Data received.
 * @param[in] length   Length of the data.
 */
/**@snippet [Handling the data received over BLE] */
static void fts_data_handler(ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length)
{
    switch(p_data[0])
    {
        case CMD_GET_FILE:
            NRF_LOG_INFO("CMD_GET_FILE");
            m_cmd = p_data[0];
            m_file_size = write_pointer / 2;
            break;
        case CMD_GET_FILE_INFO:
            NRF_LOG_INFO("CMD_GET_FILE_INFO");
            m_cmd = p_data[0];
            m_file_size = write_pointer / 2;
            break;
        default:
            NRF_LOG_ERROR("Unknown command: %02x", p_data[0]);
            break;
    }
}
/**@snippet [Handling the data received over BLE] */

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    ble_lbs_init_t     lbs_init = {0};
    ble_fts_init_t     fts_init = {0};
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize LBS.
    lbs_init.led_write_handler = led_write_handler;

    err_code = ble_lbs_init(&m_lbs, &lbs_init);
    APP_ERROR_CHECK(err_code);

    // Initialize FTS.
    fts_init.data_handler = fts_data_handler;

    err_code = ble_fts_init(&m_fts, &fts_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module that
 *          are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply
 *       setting the disconnect_on_fail config parameter, but instead we use the event
 *       handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}



static volatile uint16_t ROTU_peerid;
/**@brief Function for adding a bonded client.
 *
 * @param[in] p_evt Pointer to peer manager event structure.
 */
static void bonded_client_add(pm_evt_t const * p_evt)
{
    uint16_t   conn_handle = p_evt->conn_handle;
    uint16_t   peer_id     = p_evt->peer_id;
    ROTU_peerid = peer_id;
    
}


/**@brief Function for removing all bonded clients.
 */
inline static void bonded_client_remove_all(void)
{
    
}


/**@brief Function for triggering notification with battery level
 *        after bonded client reconnection.
 *
 * @param[in] p_evt Pointer to peer manager event structure.
 */
static void on_bonded_peer_reconnection_lvl_notify(pm_evt_t const * p_evt)
{
    ret_code_t        err_code;
    static uint16_t   peer_id   = PM_PEER_ID_INVALID;

    peer_id = p_evt->peer_id;

    //// Notification must be sent later to allow the client to
    //// perform a database discovery first
    //err_code = app_timer_start(m_battery_notify_timer_id,
    //                                   BATTERY_NOTIFY_DELAY,
    //                                   &peer_id);
    //APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
            bonded_client_add(p_evt);
            on_bonded_peer_reconnection_lvl_notify(p_evt);
            break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
            bonded_client_add(p_evt);
            break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            bonded_client_remove_all();

            // Bonds are deleted. Start scanning.
            break;

        default:
            break;
    }
}

static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    ret_code_t           err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    bsp_board_led_on(ADVERTISING_LED);
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;
   
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            bsp_board_led_on(CONNECTED_LED);
            bsp_board_led_off(ADVERTISING_LED);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            bsp_board_led_off(CONNECTED_LED);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            advertising_start();
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            break;
        
        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;
       case BLE_GAP_EVT_AUTH_STATUS:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x",
                          p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
                          p_ble_evt->evt.gap_evt.params.auth_status.bonded,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

uint32_t get_timestamp()
{
    return app_timer_cnt_get();
}

uint32_t get_timestamp_delta(uint32_t base)
{
  uint32_t now = app_timer_cnt_get();

  if(base > now)
    return 0;
  else
    return  now - base;
}

static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(get_timestamp);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static bool rec_button_released()
{
 // auto tmp = nrf_gpio_pin_input_get(BUTTON_PIN);
  return 0;//(0 == tmp);
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}



static void audio_frame_handle()
{
  if(pending_frame) 
  {
    const int data_size = pending_frame->buffer_occupied;
    NRFX_ASSERT(data_size < SPI_XFER_DATA_STEP);
    NRFX_ASSERT(app_sm_state == APP_SM_RECORDING);
      //  nrf_gpio_pin_set(17);
    spi_flash_write(write_pointer, data_size, pending_frame->buffer);
       //     nrf_gpio_pin_clear(17);
    write_pointer += SPI_XFER_DATA_STEP;

    pending_frame->buffer_free_flag = true;
    pending_frame->buffer_occupied = 0;
    pending_frame = NULL;
  }
}
static void audio_frame_cb(drv_audio_frame_t * frame)
{
  NRFX_ASSERT(NULL == pending_frame);
  pending_frame = frame;
}
/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static bool is_recording_active = false;
uint8_t tmp_buf_after_write[128];
bool is_dbg_read_triggered = false;
bool is_erase_triggered = false;
bool is_erase_done = false;
int shutdown_counter = 10;

#define MAX_REC_TIME_MS 4500

static void idle_state_handle(void)
{
    NRF_LOG_PROCESS();
    enum APP_SM_STATES prev_state = app_sm_state;
    switch (app_sm_state)
    {
    case APP_SM_REC_INIT:
    {
      bsp_board_led_on(LEDBUTTON_LED);
      is_recording_active = true;
      drv_audio_transmission_enable();
      app_sm_state = APP_SM_RECORDING;
      rec_start_ts = get_timestamp();
    break;
    }
    case APP_SM_RECORDING:
    {
    // todo: if button or timeout - stop rec and goto APP_SM_CONN
    // todo: 
      const uint32_t timestamp = get_timestamp_delta(rec_start_ts);
      if ((timestamp >= MAX_REC_TIME_MS || rec_button_released()) && is_recording_active)
      {
          drv_audio_transmission_disable();
          is_recording_active  = false;
          bsp_board_led_off(LEDBUTTON_LED);
      }
      else
      if (timestamp >= (MAX_REC_TIME_MS+500))
      {
          if (!is_dbg_read_triggered)
          {
              is_dbg_read_triggered = true;
          }
          else
          {
              if (!spi_flash_is_spi_bus_busy())
              {
                  app_sm_state = APP_SM_CONN;
                  NRF_LOG_INFO("start advertising.");
                  advertising_start();
              }
          }
      }
    }
    break;

    case APP_SM_CONN:
    {
    // todo: establish connection
    // todo:
        //spi_flash_trigger_read(0x400, 128);

        //while (spi_flash_is_spi_bus_busy()) ;
        //spi_flash_copy_received_data(tmp_buf_after_write, 128);

////if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
////{
////    app_sm_state = APP_SM_XFER;
////}
    }   
    break;

    case APP_SM_XFER:
    // todo: perform transfer
    // todo: on xfer end goto APP_SM_FINALISE
        if (is_file_transmission_done)
        {
            uint32_t err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            app_sm_state = APP_SM_FINALISE;
        }
        break;

    case APP_SM_FINALISE:
    {
    // todo: erase flash
    // todo: power shutdown
          //app_sm_state = APP_FLASH_ERASE_START;
    //if (get_timestamp_delta(rec_start_ts) >= 20000 )
    {
        if (!is_erase_triggered)
        {
          NRF_LOG_INFO("start erasing");
          is_erase_triggered = true;
          int rc = spi_flash_erase_area(0x00, 0xB0000);
          if (rc != 0)
          {
              NRF_LOG_INFO("erase failed");
              is_erase_done = true;
          }
        }
        else if (!is_erase_done)
        {
            is_erase_done = !spi_flash_is_busy();
            if (is_erase_done)
            {
                NRF_LOG_INFO("verify SPI Flash state");
                spi_flash_trigger_read(0x00, 8);
                while (spi_flash_is_spi_bus_busy()) ;
                uint8_t tmp[8];
                spi_flash_copy_received_data(tmp, 8);
                if (tmp[0] == 0xFF && tmp[1] == 0xFF)
                {
                  NRF_LOG_INFO("flash - ok");
                }
                else
                {
                  NRF_LOG_ERROR("flash - failed");
                }
                
            }
        }
        else
        {
            --shutdown_counter;
            if (shutdown_counter == 5)
            {
              NRF_LOG_INFO("done erasing, shutdown");
            }
            else
            if (shutdown_counter == 0)
            {
                nrf_gpio_pin_clear(LDO_EN_PIN);
            }
        }
    }
    
    break;
    }
    }

    if(app_sm_state != prev_state)
    {
      NRF_LOG_INFO("SM state %d > %d",prev_state , app_sm_state);
    }
}

static uint32_t send_data(const uint8_t *data, uint32_t data_size)
{
    if (data_size > 0)
    {
        unsigned i = 0;

        uint32_t size_left = data_size;
        const uint8_t *send_buffer = data;

        while (size_left)
        {
            uint8_t send_size = MIN(size_left, m_ble_its_max_data_len);
            size_left -= send_size;

            uint32_t err_code = NRF_SUCCESS;
            while (true)
            {
                err_code = ble_fts_send_file_fragment(&m_fts, send_buffer, send_size);
                if (err_code == NRF_SUCCESS)
                {
                    break;
                }
                else if (err_code != NRF_ERROR_RESOURCES)
                {
                    NRF_LOG_ERROR("Failed to send file, err = %d", err_code);
                    return err_code;
                }
            }

            send_buffer += send_size;
        }
    }

    return NRF_SUCCESS;
}

static uint32_t read_pointer = 0;
#define SPI_READ_SIZE 128

void ble_operation_handle()
{
    switch(m_cmd)
    {
        case CMD_GET_FILE:
        {
            if (read_pointer == 0) // TODO change to file start
            {
                NRF_LOG_INFO("Starting file sending...");
            }

            if (!m_file_size || read_pointer >= write_pointer)
            {
                is_file_transmission_done = true;
                m_cmd = CMD_EMPTY;
                NRF_LOG_INFO("File have been sent.");
                break;
            }

            uint8_t buffer[SPI_READ_SIZE];
            spi_flash_trigger_read(read_pointer, sizeof(buffer));
            while (spi_flash_is_spi_bus_busy());
            spi_flash_copy_received_data(buffer, sizeof(buffer));
            
            if(read_pointer == 0)
            {
              drv_audio_wav_header_apply(buffer, m_file_size);
            }

            send_data(buffer, sizeof(buffer));

            read_pointer += 2 * SPI_READ_SIZE;
            break;
        }
        case CMD_GET_FILE_INFO:
        {
            NRF_LOG_INFO("Sending file info, size %d (%X)", m_file_size, m_file_size);

            ble_fts_file_info_t file_info;
            file_info.file_size_bytes = m_file_size;

            ble_fts_file_info_send(&m_fts, &file_info);
            m_cmd = CMD_EMPTY;

            break;
        }
        default:
            m_cmd = CMD_EMPTY;
    }
}

static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    nrf_gpio_cfg_output(LDO_EN_PIN);
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_pin_set(LDO_EN_PIN);
    log_init();
    leds_init();
    timers_init();
    power_management_init();

    ble_stack_init();
    gap_params_init();
    gatt_init();
    peer_manager_init();
    delete_bonds();
    services_init();
    advertising_init();
    conn_params_init();
    drv_audio_init(audio_frame_cb);

    // Start execution.
    NRF_LOG_INFO("dictofun: start main loop");
    
    spi_access_init();
    write_pointer = 0;

    ble_gap_addr_t ble_addr;
    sd_ble_gap_addr_get(&ble_addr);
    NRF_LOG_INFO("%x:%x:%x:%x:%x:%x", ble_addr.addr[5],ble_addr.addr[4],ble_addr.addr[3],ble_addr.addr[2],ble_addr.addr[1],ble_addr.addr[0]);
    for (;;)
    {
        ble_operation_handle();
        spi_flash_cyclic();
        audio_frame_handle();
        idle_state_handle();
    }
    // Shut the LDO down
    
    nrf_gpio_pin_clear(LDO_EN_PIN);
}


/**
 * @}
 */
