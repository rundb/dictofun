#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include "nrf_log.h"

#include <cstdint>

// RAII class targeting at profiling of a particular block of code.
namespace memory
{

class TimeProfile
{
public:
    explicit TimeProfile(const char * description)
    : _start_tick{xTaskGetTickCount()}
    , _description(description)
    {}

    ~TimeProfile() {
        const auto end_tick = xTaskGetTickCount();
        NRF_LOG_INFO("%s:%d ms", _description, end_tick - _start_tick);
    }

private:
    std::uint32_t _start_tick;
    const char * _description;

};

}
