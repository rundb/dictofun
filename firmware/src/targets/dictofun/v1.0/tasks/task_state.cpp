#include "tasks/task_state.h"
#include <nrf_gpio.h>
#include <nrf_log.h>
#include "tasks/task_memory.h"
#include "tasks/task_audio.h"
#include "tasks/task_led.h"
#include "BleSystem.h"

namespace application
{
AppSmState _applicationState{APP_SM_STARTUP};
static bool is_finalize_triggered{false};


AppSmState getApplicationState()
{
    return _applicationState;
}

bool isRecordButtonPressed()
{
    return nrf_gpio_pin_read(BUTTON_PIN) > 0;
}

static volatile int shutdown_counter = 100000;
void application_cyclic()
{
    const auto prevState = _applicationState;
    switch (_applicationState)
    {
        case APP_SM_STARTUP:
        {
            _applicationState = APP_SM_REC_INIT;
            led::task_led_set_indication_state(led::PREPARING);
            nrf_gpio_pin_set(LDO_EN_PIN);

            const auto erased = memory::isMemoryErased();
            if (!erased)
            {
                NRF_LOG_WARNING("Memory is not erased. Erasing now");
                int rc = spi_flash_erase_area(0x00, 0xB0000);
                if (rc != 0)
                {
                    NRF_LOG_ERROR("Erase failed. Finalizing.");
                    _applicationState = APP_SM_FINALIZE;
                }
                else
                {
                    _applicationState = APP_SM_PREPARE;
                }
            }

            break;
        }
        case APP_SM_PREPARE:
        {
            led::task_led_set_indication_state(led::PREPARING);
            // Wait until SPI Flash memory erase is done
            if (!spi_flash_is_busy())
            {
                _applicationState = APP_SM_REC_INIT;
            }
            break;
        }
        case APP_SM_REC_INIT:
        {
            led::task_led_set_indication_state(led::RECORDING);
            _applicationState = APP_SM_RECORDING;
            audio_start_record();
            break;
        }
        case APP_SM_RECORDING:
        {
            // if button is released - go further
            if (!isRecordButtonPressed())
            {
                audio_stop_record();
                _applicationState = APP_SM_CONN;
                const auto record_size = audio_get_record_size();
                NRF_LOG_INFO("Recorded %d bytes", record_size);
            }
            break;
        }
        case APP_SM_CONN:
        {
            led::task_led_set_indication_state(led::CONNECTING);
            if (!ble::BleSystem::getInstance().isActive())
            {
                ble::BleSystem::getInstance().start();
                const auto record_size = audio_get_record_size();
                ble::BleSystem::getInstance().getServices().setFileSizeForTransfer(record_size);
            }
            if (ble::BleSystem::getInstance().getConnectionHandle() != BLE_CONN_HANDLE_INVALID)
            {
                _applicationState = APP_SM_XFER;
            }
            break;
        }
        case APP_SM_XFER:
        {
            led::task_led_set_indication_state(led::SENDING);
            if (ble::BleSystem::getInstance().getServices().isFileTransmissionComplete())
            {
                _applicationState = APP_SM_FINALIZE;
            }
            break;
        }
        case APP_SM_FINALIZE:
        {
            led::task_led_set_indication_state(led::SHUTTING_DOWN);
            if (!is_finalize_triggered)
            {
                is_finalize_triggered = true;
                NRF_LOG_INFO("Finalizing operation: erasing memory");
                // trigger flash memory erase
                int rc = spi_flash_erase_area(0x00, 0xB0000);
                if (rc != 0)
                {
                    NRF_LOG_ERROR("Erase failed.");
                    _applicationState = APP_SM_FINALIZE;
                }
            }
            --shutdown_counter;
            if (shutdown_counter == 0 && (!spi_flash_is_busy()))
            {
                _applicationState = APP_SM_SHUTDOWN;
            }

            // after memory is erased - shutdown
            break;
        }
        case APP_SM_SHUTDOWN:
        {
            // disable LDO and enter low-power sleep mode
            nrf_gpio_pin_clear(LDO_EN_PIN);
            while(1);
            break;
        }
    }
    if (prevState != _applicationState)
    {
        // TODO: log state change
        NRF_LOG_INFO("state %d->%d", prevState, _applicationState);
    }
}
}
