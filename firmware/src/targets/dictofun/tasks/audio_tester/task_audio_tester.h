#pragma once

#include "FreeRTOS.h"
#include "queue.h"

namespace audio
{
namespace tester
{
void task_audio_tester(void * context_ptr);

struct Context
{
    QueueHandle_t data_queue;
};

}
}