// #include "ble.h"
// #include "ble_file_transfer_service.h"

// BLE_FTS_DEF(m_fts, NRF_SDH_BLE_TOTAL_LINK_COUNT);

// static bool is_recording_active = false;
// uint8_t tmp_buf_after_write[128];
// bool is_dbg_read_triggered = false;
// bool is_erase_triggered = false;
// bool is_erase_done = false;
// int shutdown_counter = 10;

// enum command_id
// {
//     CMD_EMPTY = 0,
//     CMD_GET_FILE,
//     CMD_GET_FILE_INFO,
// };

// static uint8_t m_cmd = CMD_EMPTY;
// static uint32_t m_file_size = 0;
// static const uint16_t m_ble_its_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
// static bool is_file_transmission_done = false;
// uint32_t rec_start_ts = 0;

// #define MAX_REC_TIME_MS 4500

// static void idle_state_handle(void)
// {
//     NRF_LOG_PROCESS();
// }

// static uint32_t send_data(const uint8_t *data, uint32_t data_size)
// {
//     if (data_size > 0)
//     {
//         unsigned i = 0;

//         uint32_t size_left = data_size;
//         const uint8_t *send_buffer = data;

//         while (size_left)
//         {
//             uint8_t send_size = MIN(size_left, m_ble_its_max_data_len);
//             size_left -= send_size;

//             uint32_t err_code = NRF_SUCCESS;
//             while (true)
//             {
//                 err_code = ble_fts_send_file_fragment(&m_fts, send_buffer, send_size);
//                 if (err_code == NRF_SUCCESS)
//                 {
//                     break;
//                 }
//                 else if (err_code != NRF_ERROR_RESOURCES)
//                 {
//                     NRF_LOG_ERROR("Failed to send file, err = %d", err_code);
//                     return err_code;
//                 }
//             }

//             send_buffer += send_size;
//         }
//     }

//     return NRF_SUCCESS;
// }

// void fts_operation_handle()
// {
//     switch(m_cmd)
//     {
//         case CMD_GET_FILE:
//         {
//             if (read_pointer == 0) // TODO change to file start
//             {
//                 NRF_LOG_INFO("Starting file sending...");
//             }

//             if (!m_file_size || read_pointer >= write_pointer)
//             {
//                 is_file_transmission_done = true;
//                 m_cmd = CMD_EMPTY;
//                 NRF_LOG_INFO("File have been sent.");
//                 break;
//             }

//             uint8_t buffer[SPI_READ_SIZE];
//             spi_flash_trigger_read(read_pointer, sizeof(buffer));
//             while (spi_flash_is_spi_bus_busy());
//             spi_flash_copy_received_data(buffer, sizeof(buffer));
            
//             if(read_pointer == 0)
//             {
//               drv_audio_wav_header_apply(buffer, m_file_size);
//             }

//             send_data(buffer, sizeof(buffer));

//             read_pointer += 2 * SPI_READ_SIZE;
//             break;
//         }
//         case CMD_GET_FILE_INFO:
//         {
//             NRF_LOG_INFO("Sending file info, size %d (%X)", m_file_size, m_file_size);

//             ble_fts_file_info_t file_info;
//             file_info.file_size_bytes = m_file_size;

//             ble_fts_file_info_send(&m_fts, &file_info);
//             m_cmd = CMD_EMPTY;

//             break;
//         }
//         default:
//             m_cmd = CMD_EMPTY;
//     }
// }

// static void fts_data_handler(ble_fts_t * p_fts, uint8_t const * p_data, uint16_t length)
// {
//     switch(p_data[0])
//     {
//         case CMD_GET_FILE:
//             NRF_LOG_INFO("CMD_GET_FILE");
//             m_cmd = p_data[0];
//             m_file_size = write_pointer / 2;
//             break;
//         case CMD_GET_FILE_INFO:
//             NRF_LOG_INFO("CMD_GET_FILE_INFO");
//             m_cmd = p_data[0];
//             m_file_size = write_pointer / 2;
//             break;
//         default:
//             NRF_LOG_ERROR("Unknown command: %02x", p_data[0]);
//             break;
//     }
