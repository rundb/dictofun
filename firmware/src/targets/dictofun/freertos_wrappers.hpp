// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

namespace application
{

template<size_t stack_size_, UBaseType_t priority_>
struct TaskDescriptor
{
    static constexpr size_t stack_size{stack_size_};
    static constexpr UBaseType_t priority{priority_};
    StackType_t stack[stack_size] {0UL};
    StaticTask_t task;
    TaskHandle_t handle{nullptr};

    result::Result init(TaskFunction_t function, const char * task_name, void * task_parameters)
    {
        handle = xTaskCreateStatic(
            function,
            task_name,
            stack_size,
            task_parameters,
            priority,
            stack,
            &task);
        if (nullptr == handle)
        {
            return result::Result::ERROR_GENERAL;
        }
 
        return result::Result::OK;
    }
};

template<typename T, size_t N>
struct QueueDescriptor
{
    using type = T;
    static constexpr size_t element_size{sizeof(T)};
    static constexpr size_t depth{N};

    StaticQueue_t queue;
    uint8_t buffer[N * sizeof(T)];
    QueueHandle_t handle{nullptr};

    result::Result init()
    {
        handle = xQueueCreateStatic( depth, element_size, buffer, &queue);
        if (nullptr == handle)
        {
            return result::Result::ERROR_GENERAL;
        }
        return result::Result::OK;
    }
};


}