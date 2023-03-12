// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "task_rtc.h"
#include "nrf_log.h"

#include "boards.h"

#include "nrf_i2c.h"

namespace rtc
{

static constexpr uint32_t command_wait_time{1000};

const i2c::NrfI2c::Config i2c_config{
    I2C_MODULE_IDX, 
    I2C_CLK_PIN_NUMBER,
    I2C_DATA_PIN_NUMBER,
    i2c::NrfI2c::Config::Baudrate::_100K
};

i2c::NrfI2c rtc_i2c{i2c_config};

volatile bool is_i2c_slave_acknowledged{false};
void i2c_callback(i2c::NrfI2c::TransactionResult result)
{
    is_i2c_slave_acknowledged = (result == i2c::NrfI2c::TransactionResult::COMPLETE);
}

void scan_slaves()
{
    uint32_t counter{0};
    rtc_i2c.register_completion_callback(i2c_callback);
    for (auto i = 0; i < 0x7F; i += 2)
    {
        is_i2c_slave_acknowledged = false;
        uint8_t test_data[1] {0};
        const auto i2c_tx_result = rtc_i2c.read(i, test_data, sizeof(test_data));
        if (i2c::Result::OK != i2c_tx_result)
        {
            NRF_LOG_ERROR("i2c tx failed. adr (%x)", i);
        }
        vTaskDelay(3);
        if (is_i2c_slave_acknowledged)
        {
            ++counter;
            NRF_LOG_INFO("discovered slave @ 0x%x", i);
        }
    }
    NRF_LOG_INFO("discovered %d slaves", counter);
}
    
void task_rtc(void * context_ptr)
{
    NRF_LOG_INFO("task rtc: initialized");
    Context& context = *(reinterpret_cast<Context *>(context_ptr));

    CommandQueueElement command;

    const auto i2c_init_result = rtc_i2c.init();
    if (i2c::Result::OK != i2c_init_result)
    {
        NRF_LOG_ERROR("i2c init failed");
        vTaskSuspend(nullptr);
    }
    scan_slaves();

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