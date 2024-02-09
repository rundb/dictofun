// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_button.h"

#include "FreeRTOS.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "task.h"

#include "nrf_log.h"

namespace button
{

static ButtonState button_state{ButtonState::RELEASED};
static RequestQueueElement request_buffer;

ButtonState get_button_state()
{
    return nrf_gpio_pin_read(BUTTON_PIN) > 0 ? ButtonState::PRESSED : ButtonState::RELEASED;
}

void task_button(void* context_ptr)
{
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLDOWN);

    NRF_LOG_DEBUG("task button: initialized");
    Context& context = *(reinterpret_cast<Context*>(context_ptr));

    static constexpr uint32_t button_request_wait_time_ms{10};
    while(1)
    {
        const auto new_state = get_button_state();
        if(new_state != button_state)
        {
            NRF_LOG_INFO("button: %s", new_state == ButtonState::PRESSED ? "ON" : "OFF");
            EventQueueElement evt;
            evt.event = (new_state == ButtonState::PRESSED) ? Event::SINGLE_PRESS_ON
                                                            : Event::SINGLE_PRESS_OFF;
            const auto send_result = xQueueSend(context.events_handle, &evt, 0);
            if(pdTRUE != send_result)
            {
                NRF_LOG_ERROR("failed to send button state");
            }

            button_state = new_state;
        }

        const auto req_rx_result = xQueueReceive(context.commands_handle, &request_buffer, button_request_wait_time_ms);
        if (pdPASS == req_rx_result)
        {
            ResponseQueueElement response;
            response.state = get_button_state();
            xQueueSend(context.response_handle, &response, 0);
        }
    }
}

} // namespace button
