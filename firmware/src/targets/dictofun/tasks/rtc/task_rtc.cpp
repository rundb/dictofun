// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "task_rtc.h"
#include "nrf_log.h"

namespace rtc
{

static constexpr uint32_t command_wait_time{1000};
    
void task_rtc(void * context_ptr)
{
    NRF_LOG_INFO("task rtc: initialized");
    Context& context = *(reinterpret_cast<Context *>(context_ptr));

    CommandQueueElement command;

    while(1)
    {
        const auto cmd_queue_receive_status = xQueueReceive(
            context.command_queue,
            reinterpret_cast<void *>(&command),
            command_wait_time);
        if (pdPASS == cmd_queue_receive_status)
        {
            NRF_LOG_INFO("received command %d", command.command_id);
        }
    }
}

}