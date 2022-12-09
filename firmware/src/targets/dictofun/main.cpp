// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "app_error.h"

#include "nrf_sdm.h"
#include "BleSystem.h"
#include "ble_dfu.h"
#include "spi.h"
#include "spi_flash.h"
#include <boards.h>
#include <nrf_gpio.h>
#include "simple_fs.h"
#include "block_device_api.h"
#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <task_led.h>
#include <task_state.h>
#include "task_audio.h"
#include "task_cli_logger.h"

#include "audio_processor.h"
#include "microphone_pdm.h"

ble::BleSystem bleSystem{};
spi::Spi flashSpi(0, SPI_FLASH_CS_PIN);
flash::SpiFlash flashMemory(flashSpi);
flash::SpiFlash& getSpiFlash() { return flashMemory;}

static const spi::Spi::Configuration flash_spi_config{NRF_DRV_SPI_FREQ_2M,
                                                      NRF_DRV_SPI_MODE_0,
                                                      NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
                                                      SPI_FLASH_SCK_PIN,
                                                      SPI_FLASH_MOSI_PIN,
                                                      SPI_FLASH_MISO_PIN};

constexpr size_t pdm_sample_size{8U};
using AudioSampleType = audio::microphone::PdmSample<pdm_sample_size>;
audio::microphone::PdmMicrophone<pdm_sample_size> pdm_mic;
audio::AudioProcessor<audio::microphone::PdmMicrophone<pdm_sample_size>::SampleType> audio_processor{pdm_mic};

static TaskHandle_t m_audio_task;
static TaskHandle_t m_cli_logger_task;

void latch_ldo_enable()
{
    nrf_gpio_cfg_output(LDO_EN_PIN);
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_pin_set(LDO_EN_PIN);

    nrf_gpio_cfg(LDO_EN_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_DISCONNECT,
                 NRF_GPIO_PIN_PULLDOWN,
                 NRF_GPIO_PIN_H0S1,
                 NRF_GPIO_PIN_NOSENSE);
}

// TODO: reintegrate system elements after designing them in a test-driven way
int main()
{
    latch_ldo_enable();

    const auto err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);

    logger::log_init();

    bsp_board_init(BSP_INIT_LEDS);

    audio_processor.start();

    if (pdPASS != xTaskCreate(task_audio, "AUDIO", 256, NULL, 1, &m_audio_task))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
    if (pdPASS != xTaskCreate(logger::task_cli_logger, "CLI", 256, NULL, 1, &m_cli_logger_task))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    vTaskStartScheduler();

}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    __disable_irq();
    NRF_LOG_FINAL_FLUSH();

#ifndef DEBUG
    NRF_LOG_ERROR("Fatal error");
#else
    switch (id)
    {
#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT
        case NRF_FAULT_ID_SD_ASSERT:
            NRF_LOG_ERROR("SOFTDEVICE: ASSERTION FAILED");
            break;
        case NRF_FAULT_ID_APP_MEMACC:
            NRF_LOG_ERROR("SOFTDEVICE: INVALID MEMORY ACCESS");
            break;
#endif
        case NRF_FAULT_ID_SDK_ASSERT:
        {
            assert_info_t * p_info = (assert_info_t *)info;
            NRF_LOG_ERROR("ASSERTION FAILED at %s:%u",
                          p_info->p_file_name,
                          p_info->line_num);
            break;
        }
        case NRF_FAULT_ID_SDK_ERROR:
        {
            error_info_t * p_info = (error_info_t *)info;
            NRF_LOG_ERROR("ERROR %u [%s] at %s:%u\r\nPC at: 0x%08x",
                          p_info->err_code,
                          nrf_strerror_get(p_info->err_code),
                          p_info->p_file_name,
                          p_info->line_num,
                          pc);
             NRF_LOG_ERROR("End of error report");
            break;
        }
        default:
            NRF_LOG_ERROR("UNKNOWN FAULT at 0x%08X", pc);
            break;
    }
#endif

    NRF_BREAKPOINT_COND;
    // On assert, the system can only recover with a reset.

    nrf_gpio_pin_clear(LDO_EN_PIN);
}