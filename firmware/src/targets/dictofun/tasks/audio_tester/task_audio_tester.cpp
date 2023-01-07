#include "task_audio_tester.h"
#include "nrf_log.h"

namespace audio
{
namespace tester
{
void task_audio_tester(void * context_ptr)
{
    Context& context{*(reinterpret_cast<Context *>(context_ptr))};
    uint8_t buffer[64]{0};
    NRF_LOG_INFO("audio tester: initialized");
    size_t ticks_counter{0};
    size_t received_samples_count{0};
    while(1)
    {
        // TODO: add a suspension for the case when storage task is actually active 
        const auto result = xQueueReceive(
            context.data_queue,
            reinterpret_cast<void *>(buffer),
            10);
        if (pdPASS == result)
        {
            received_samples_count++;
        }

        ticks_counter += 10;
        if (ticks_counter % 10000 == 0)
        {
            NRF_LOG_INFO("autest: received %d samples", received_samples_count);
        }

    }
}
}
}