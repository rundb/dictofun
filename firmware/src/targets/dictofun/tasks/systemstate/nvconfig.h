// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include "result.h"
#include <stdint.h>
#include "nrf_fstorage.h"
#include <functional>

namespace application
{

enum Mode: uint32_t
{
    DEVELOPMENT,
    ENGINEERING,
    FIELD,
};

/// @brief This class contains all application-specific configuration fields and 
/// provides methods to load and store the device configuration elements.
class NvConfig
{
public:
    using DelayFunction = std::function<void(uint32_t)>;
    explicit NvConfig(DelayFunction delay_function)
    : _delay{delay_function} 
    {}
    NvConfig(const NvConfig&) = delete;
    NvConfig(NvConfig&&) = delete;
    NvConfig& operator= (const NvConfig&) = delete;
    NvConfig& operator= (NvConfig&&) = delete;
    ~NvConfig() = default;

    struct Configuration
    {
        uint32_t marker;
        Mode mode;
        uint32_t crc;
    } __attribute__((packed));

    result::Result load_early(Configuration& config);
    result::Result load(Configuration& config);
    result::Result store(Configuration& config);

    void set_sd_backend();

private:
    DelayFunction _delay;
    bool _is_initialized{false};
    result::Result init();
    result::Result apply_default_config();
    static constexpr uint32_t marker_value{0xcbdf9b3a};
    static constexpr uint32_t data_array_size{sizeof(Configuration)};
    uint8_t _tmp_data[data_array_size]{0};

    static constexpr uint32_t nvm_wait_timeout_ticks{200};
    result::Result wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);

    bool is_config_valid(const Configuration& config);

};

}
