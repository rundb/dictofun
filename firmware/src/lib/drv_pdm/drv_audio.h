/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/**
 * @defgroup DRV_AUDIO Audio driver
 * @{
 * @ingroup MOD_AUDIO
 * @brief Audio top level driver
 *
 * @details
 */
#ifndef __DRV_AUDIO_H__
#define __DRV_AUDIO_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>

#include "app_scheduler.h"
#include "app_error.h"

//#include "nrf_drv_config.h"
//#include "nRF52_config.h"

#define CONFIG_AUDIO_FRAME_SIZE_SAMPLES 64 //CONFIG_PCM_FRAME_SIZE_SAMPLES
#define CONFIG_AUDIO_FRAME_SIZE_BYTES   128

typedef struct
{
    uint8_t buffer[CONFIG_AUDIO_FRAME_SIZE_BYTES];
    int     buffer_occupied;
    bool    buffer_free_flag;
} drv_audio_frame_t;

typedef void (*audio_frame_cb_t)(drv_audio_frame_t*);

/**@brief Enable audio transmission.
 *
 * @return
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_INTERNAL
 */
uint32_t drv_audio_transmission_enable(void);

/**@brief Disable audio transmission.
 *
 * @return
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_INTERNAL
 */
uint32_t drv_audio_transmission_disable(void);

/**@brief Initialization.
 *
 * @return
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_INVALID_PARAM
 * @retval NRF_ERROR_INTERNAL
 */
uint32_t drv_audio_init(audio_frame_cb_t frame_cb);

void drv_audio_wav_header_apply(uint8_t* data, size_t wav_sz);

#endif /* __DRV_AUDIO_H__ */
/** @} */
