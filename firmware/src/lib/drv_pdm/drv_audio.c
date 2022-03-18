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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_sdm.h"

#include "drv_audio.h"
#include "sdk_config.h"
#include "nrf_log.h"

#include "nrf_drv_pdm.h"
#include "boards/boards.h"


//#define DBG_RADIO_STATUS			31
#define DBG_PDM_HANDLER_PIN			17
//#define DBG_PDM_AUDIO_BUFFER			27

#define CONFIG_AUDIO_PDM_GAIN 			0x38

#define CONFIG_PDM_FRAME_SIZE_SAMPLES		64
//#define CONFIG_PCM_FRAME_SIZE_SAMPLES		64

// <o> Number of Audio Frame Buffers <2-16>
#define CONFIG_AUDIO_FRAME_BUFFERS 		2 //6
// </h>
#define PWR_LEVEL_MID  0x8000
#define PWR_SAMPLES		64
uint16_t signal_pwr[PWR_SAMPLES];
int pwr_sample = 0;

uint8_t samples[64*270*2];
size_t samples_offset =0;

#define PCM_RES_BYTES  2
#define PCM_DEC_FACTOR 1

#define PCM_BASE_FREQ 16000

size_t rec_skip_counter = 0;
#define REC_SKIP_BUFFERS	0.2*(PCM_BASE_FREQ/CONFIG_PDM_FRAME_SIZE_SAMPLES)

typedef struct wav_header {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    int fmt_chunk_size; // Should be 16 for PCM
    short audio_format; // Should be 1 for PCM. 3 for IEEE Float
    short num_channels;
    int sample_rate;
    int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    short sample_alignment; // num_channels * Bytes Per Sample
    short bit_depth; // Number of bits per sample
    
    // Data
    char data_header[4]; // Contains "data"
    int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    // uint8_t bytes[]; // Remainder of wave file is bytes
} wav_header;

const uint8_t wav_prefill[] = { 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00,
                                0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
                                0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
                                0x22, 0x56, 0x00, 0x00, 0x88, 0x58, 0x01, 0x00,
                                0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61,
                                0x00, 0x08, 0x00, 0x00} ;
///////////////////////////////////////////////////////////////////////

#ifdef CONFIG_AUDIO_ENABLED

static volatile drv_audio_frame_t       m_audio_frame[CONFIG_AUDIO_FRAME_BUFFERS];
static int16_t                          m_pdm_buff[2][CONFIG_AUDIO_FRAME_SIZE_SAMPLES];
uint16_t				m_pdm_buff_idx = 0;

uint16_t				m_frame_index = 0;
uint16_t                          	m_curr_frame_index = CONFIG_AUDIO_FRAME_BUFFERS;
static audio_frame_cb_t                 m_audio_frame_handler = NULL;
static bool allow_cb = false;
/**@brief Search for free frame index, start from value stored in m_next_frame_index
 *
 * @retval - valid number is 0..CONFIG_AUDIO_FRAME_BUFFERS-1, CONFIG_AUDIO_FRAME_BUFFERS indicates, that there is no free buffer available
 */
static uint16_t drv_audio_find_free_frame_index(void)
{
    uint16_t frame_index;
    uint16_t i;
    frame_index = 0;
    for (i = 0; i < CONFIG_AUDIO_FRAME_BUFFERS; i++)
    {
        if (m_audio_frame[frame_index].buffer_free_flag)
        {
            return frame_index;
        }
        frame_index = (frame_index + 1) % CONFIG_AUDIO_FRAME_BUFFERS;
    }
    return CONFIG_AUDIO_FRAME_BUFFERS;
}


uint32_t drv_audio_transmission_enable(void)
{
    unsigned int i;
    m_curr_frame_index = CONFIG_AUDIO_FRAME_BUFFERS;

    for (i = 0; i < CONFIG_AUDIO_FRAME_BUFFERS; i++ )
    {
        m_audio_frame[i].buffer_free_flag = true;
        m_audio_frame[i].buffer_occupied = 0;
    }
    allow_cb = true;
    rec_skip_counter = 0;
    return nrf_drv_pdm_start();
}

uint32_t drv_audio_transmission_disable(void)
{
    allow_cb = false;
    return nrf_drv_pdm_stop();
}

static void drv_audio_pdm_event_handler(nrfx_pdm_evt_t * const p_evt)
{
  if(!p_evt) {
    APP_ERROR_CHECK_BOOL(false);  // assert error for unsupported events
    return;
  }

  if(p_evt->buffer_requested) 
  {
      nrf_drv_pdm_buffer_set(m_pdm_buff[m_pdm_buff_idx++], CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
    if(m_pdm_buff_idx == 2)
      m_pdm_buff_idx = 0;
  }

    //nrf_gpio_pin_toggle(DBG_PDM_HANDLER_PIN);

#if 0//CONFIG_DEBUG_ENABLED
	
    if ((int16_t *)p_evt->buffer_released == m_pdm_buff[0])
    {
        nrf_gpio_pin_set(DBG_PDM_HANDLER_PIN);
    }
    else if ((int16_t *)p_evt->buffer_released == m_pdm_buff[1])
    {
        nrf_gpio_pin_clear(DBG_PDM_HANDLER_PIN);
    }

#endif /* CONFIG_DEBUG_ENABLED */
//nrf_gpio_pin_set(DBG_PDM_HANDLER_PIN);

  if(p_evt->buffer_released && allow_cb && (rec_skip_counter++ > REC_SKIP_BUFFERS))
  {
    if(m_curr_frame_index >= CONFIG_AUDIO_FRAME_BUFFERS)
      m_curr_frame_index = drv_audio_find_free_frame_index();
    NRFX_ASSERT(m_curr_frame_index < CONFIG_AUDIO_FRAME_BUFFERS);
    
    int data_to_copy =  (CONFIG_AUDIO_FRAME_SIZE_SAMPLES*PCM_RES_BYTES)/PCM_DEC_FACTOR;
    uint8_t  *wp = m_audio_frame[m_curr_frame_index].buffer + m_audio_frame[m_curr_frame_index].buffer_occupied;
    uint16_t *rp = p_evt->buffer_released;
    for(int i = 0; i< CONFIG_AUDIO_FRAME_SIZE_SAMPLES/PCM_DEC_FACTOR; i++)
    {
      if(2 == PCM_RES_BYTES)
      {
        *((uint16_t*)wp) = *rp;
          wp += 2;
          rp += PCM_DEC_FACTOR;
      }
      else
      {
        *(wp) = (*rp)/256;
        wp++;
        rp += PCM_DEC_FACTOR;
      }
    }
    //memcpy(m_audio_frame[m_curr_frame_index].buffer + m_audio_frame[m_curr_frame_index].buffer_occupied, 
           //p_evt->buffer_released, CONFIG_AUDIO_FRAME_SIZE_SAMPLES*2);

 #if 0
    if(samples_offset < (sizeof(samples)-CONFIG_AUDIO_FRAME_SIZE_SAMPLES*2 ) )
    {
        memcpy(samples+ samples_offset, 
               m_audio_frame[m_curr_frame_index].buffer + m_audio_frame[m_curr_frame_index].buffer_occupied,
               data_to_copy);
        samples_offset += data_to_copy;
    }
#endif
    m_audio_frame[m_curr_frame_index].buffer_occupied += data_to_copy;

    if(m_audio_frame[m_curr_frame_index].buffer_occupied >= CONFIG_AUDIO_FRAME_SIZE_BYTES)
    {
      m_audio_frame[m_curr_frame_index].buffer_free_flag = false;

      (*m_audio_frame_handler)(&m_audio_frame[m_curr_frame_index]);
      m_curr_frame_index = CONFIG_AUDIO_FRAME_BUFFERS;
    }

  }
}

uint32_t drv_audio_init(audio_frame_cb_t  frame_cb)
{
    nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(CONFIG_IO_PDM_CLK,
                                                              CONFIG_IO_PDM_DATA);//,

    if ((frame_cb == NULL))
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    nrf_gpio_cfg_output(DBG_PDM_HANDLER_PIN);

    m_audio_frame_handler   = frame_cb;
    pdm_cfg.gain_l          = CONFIG_AUDIO_PDM_GAIN;
    pdm_cfg.gain_r          = CONFIG_AUDIO_PDM_GAIN;

    auto rc =  nrf_drv_pdm_init(&pdm_cfg, drv_audio_pdm_event_handler);
    NRFX_ASSERT(rc==NRFX_SUCCESS);
    return rc;
}

void drv_audio_wav_header_apply(uint8_t* data, size_t wav_sz)
{
  wav_header wav_hdr;
  memcpy(&wav_hdr, wav_prefill, sizeof(wav_hdr));

  wav_hdr.sample_rate = PCM_BASE_FREQ/PCM_DEC_FACTOR;
  wav_hdr.byte_rate  = wav_hdr.sample_rate * PCM_RES_BYTES;
  wav_hdr.bit_depth  = PCM_RES_BYTES * 8;
  wav_hdr.wav_size   = wav_sz - 8;
  wav_hdr.data_bytes = wav_sz - 44;
  wav_hdr.sample_alignment = 1 * PCM_RES_BYTES;

  memcpy(data, &wav_hdr, sizeof(wav_hdr));
}

#endif /* CONFIG_AUDIO_ENABLED */
