// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_rtc.h"
#include "nrf_log.h"

#include "boards.h"

#include "nrf_i2c.h"
#include "rv4162.h"

namespace rtc
{

static constexpr uint32_t command_wait_time{50};

const i2c::NrfI2c::Config i2c_config{
    I2C_MODULE_IDX, I2C_CLK_PIN_NUMBER, I2C_DATA_PIN_NUMBER, i2c::NrfI2c::Config::Baudrate::_100K};

i2c::NrfI2c rtc_i2c{i2c_config};
rtc::Rv4162 rtc{rtc_i2c};

volatile bool is_i2c_slave_acknowledged{false};
void i2c_callback(i2c::TransactionResult result)
{
    is_i2c_slave_acknowledged = (result == i2c::TransactionResult::COMPLETE);
}

void scan_slaves()
{
    uint32_t counter{0};
    rtc_i2c.register_completion_callback(i2c_callback);
    for(auto i = 0; i < 0x7F; i += 2)
    {
        is_i2c_slave_acknowledged = false;
        uint8_t test_data[1]{0};
        const auto i2c_tx_result = rtc_i2c.read(i, test_data, sizeof(test_data));
        if(i2c::Result::OK != i2c_tx_result)
        {
            NRF_LOG_ERROR("i2c tx failed. adr (%x)", i);
        }
        vTaskDelay(3);
        if(is_i2c_slave_acknowledged)
        {
            ++counter;
            NRF_LOG_INFO("discovered slave @ 0x%x", i);
        }
    }
    NRF_LOG_INFO("discovered %d slaves", counter);
}

void task_rtc(void* context_ptr)
{
    NRF_LOG_DEBUG("task rtc: initialized");
    Context& context = *(reinterpret_cast<Context*>(context_ptr));

    const auto i2c_init_result = rtc_i2c.init();
    if(i2c::Result::OK != i2c_init_result)
    {
        NRF_LOG_ERROR("i2c init failed");
        vTaskSuspend(nullptr);
    }
    // scan_slaves();

    const auto rtc_init_result = rtc.init();
    if(result::Result::OK != rtc_init_result)
    {
        NRF_LOG_ERROR("failed to initialize RTC. ");
    }
    else
    {
        rtc::Rv4162::DateTime datetime;
        const auto rtc_get_result = rtc.get_date_time(datetime);
        if(result::Result::OK != rtc_get_result)
        {
            NRF_LOG_ERROR("rtc: failed to get current date and time. ");
        }
        else
        {
            NRF_LOG_INFO("date: %d-%d-%d, time: %d-%d-%d",
                         datetime.year,
                         datetime.month,
                         datetime.day,
                         datetime.hour,
                         datetime.minute,
                         datetime.second);
        }
    }

    CommandQueueElement command;
    while(1)
    {
        const auto cmd_queue_receive_status = xQueueReceive(
            context.command_queue, reinterpret_cast<void*>(&command), command_wait_time);
        if(pdPASS == cmd_queue_receive_status)
        {
            NRF_LOG_INFO("rtc: received command %d", command.command_id);
            switch(command.command_id)
            {
            case Command::GET_TIME: {
                // 1. read the time value
                rtc::Rv4162::DateTime datetime;
                const auto rtc_get_result = rtc.get_date_time(datetime);
                if(result::Result::OK != rtc_get_result)
                {
                    ResponseQueueElement rsp{Status::ERROR};
                    xQueueSend(context.response_queue, &rsp, 0);
                    break;
                }

                // 2. serialize the time value into the response
                ResponseQueueElement rsp{Status::OK};
                rsp.content[0] = datetime.second;
                rsp.content[1] = datetime.minute;
                rsp.content[2] = datetime.hour;
                rsp.content[3] = datetime.day;
                rsp.content[4] = datetime.month;
                rsp.content[5] = datetime.year;

                // 3. send the response back to the response queue
                xQueueSend(context.response_queue, &rsp, 0);

                break;
            }
            case Command::SET_TIME: {
                rtc::Rv4162::DateTime datetime;
                datetime.year = command.content[0];
                datetime.month = command.content[1];
                datetime.day = command.content[2];
                datetime.hour = command.content[3];
                datetime.minute = command.content[4];
                datetime.second = command.content[5];
                datetime.weekday = 7;

                const auto rtc_set_result = rtc.set_date_time(datetime);
                if(result::Result::OK != rtc_set_result)
                {
                    NRF_LOG_ERROR("rtc: failed to set time");
                }
                else
                {
                    NRF_LOG_DEBUG("rtc: successfully updated current time");
                }
                break;
            }
            }
        }
    }
}

} // namespace rtc
