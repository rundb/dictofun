// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#include "nvconfig.h"

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "nrf_log.h"
#include "crc32.h"

namespace application
{

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

static constexpr uint32_t fstorage_start_address{0x77000};
static constexpr uint32_t fstorage_end_address{0x78000};
/// @brief This value's unit is pages count (each page is 4096 bytes)
static constexpr uint32_t fstorage_size{1};

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    .evt_handler = fstorage_evt_handler,
    .start_addr = fstorage_start_address,
    .end_addr   = fstorage_end_address,
};

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_WARNING("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_DEBUG("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_DEBUG("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}

result::Result NvConfig::wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    uint32_t timeout{nvm_wait_timeout_ticks};
    while (nrf_fstorage_is_busy(p_fstorage) && timeout > 0)
    {
        if (_delay != nullptr)
        {
            _delay(1);
            --timeout;
        }
    }
    return (timeout == 0) ? result::Result::ERROR_TIMEOUT : result::Result::OK;
}


result::Result NvConfig::load(Configuration& config)
{
    if (!_is_initialized)
    {
        const auto init_result = init();
        if (result::Result::OK != init_result)
        {
            return result::Result::ERROR_GENERAL;
        }
    }
    Configuration tmp;

    const auto read_result = nrf_fstorage_read(&fstorage, fstorage_start_address, &tmp, sizeof(Configuration));
    if (NRF_SUCCESS != read_result)
    {
        NRF_LOG_ERROR("fstorage: config readout failed");
        return result::Result::ERROR_GENERAL;
    }

    if (!is_config_valid(tmp))
    {
        const auto res = apply_default_config();
        if (res != result::Result::OK)
        {
            NRF_LOG_ERROR("fstorage: default config application has failed");
            return res;
        }
        const auto reread_result = nrf_fstorage_read(&fstorage, fstorage_start_address, &tmp, sizeof(Configuration));
        if (NRF_SUCCESS != reread_result)
        {
            NRF_LOG_ERROR("fstorage: config readout failed");
            return result::Result::ERROR_GENERAL;
        }
        if (!is_config_valid(tmp))
        {
            NRF_LOG_ERROR("fatal error at fstorage. Can't store configuration.");
            return result::Result::ERROR_GENERAL;
        }
    }

    memcpy(&config, &tmp, sizeof(config));
    
    return result::Result::OK;
}

/// @brief Use this function before the FStorage is initialized
result::Result NvConfig::load_early(Configuration& config)
{
    Configuration tmp;

    memcpy(&tmp, reinterpret_cast<uint8_t*>(fstorage_start_address), sizeof(tmp));

    if (!is_config_valid(tmp))
    {
        return result::Result::ERROR_GENERAL;
    }
    memcpy(&config, &tmp, sizeof(config));
    
    return result::Result::OK;
}

result::Result NvConfig::store(Configuration& config)
{
    if (!_is_initialized)
    {
        const auto init_result = init();
        if (result::Result::OK != init_result)
        {
            return result::Result::ERROR_GENERAL;
        }
    }
    
    // erase configuration page
    const auto erase_result = nrf_fstorage_erase(&fstorage, fstorage_start_address, fstorage_size, NULL);
    if (NRF_SUCCESS != erase_result)
    {
        NRF_LOG_ERROR("fstorage: erase failed (%d)", erase_result);
        return result::Result::ERROR_GENERAL;
    }

    const auto wait_result = wait_for_flash_ready(&fstorage);
    if (result::Result::OK != wait_result)
    {
        return wait_result;
    }

    config.marker = marker_value;
    config.crc = crc32_compute(
        reinterpret_cast<const uint8_t *>(&config), 
        sizeof(Configuration) - sizeof(uint32_t), 
        NULL
    );

    // program the configuration block
    const auto write_result = nrf_fstorage_write(&fstorage, 
        fstorage_start_address, 
        &config, 
        sizeof(Configuration), 
        NULL);
    if (NRF_SUCCESS != write_result)
    {
        NRF_LOG_ERROR("fstorage: write failed (%d)", write_result);
        return result::Result::ERROR_GENERAL;
    }

    const auto wait_result_2 = wait_for_flash_ready(&fstorage);
    if (result::Result::OK != wait_result_2)
    {
        return wait_result;
    }

    return result::Result::OK;
}
result::Result NvConfig::init()
{
    if (_is_initialized)
    {
        return result::Result::OK;
    }

    const auto init_result = nrf_fstorage_init(&fstorage, &nrf_fstorage_sd, NULL);
    if (NRF_SUCCESS != init_result)
    {
        NRF_LOG_ERROR("nrf storage init has failed (%d)", init_result);
    }

    _is_initialized = true;
    return result::Result::OK;
}

result::Result NvConfig::apply_default_config()
{
    Configuration default_config{marker_value, Mode::DEVELOPMENT, 0};

    default_config.crc = crc32_compute(
        reinterpret_cast<const uint8_t *>(&default_config), 
        sizeof(Configuration) - sizeof(uint32_t), 
        NULL
    );

    const auto store_result = store(default_config);
    return store_result;
}

bool NvConfig::is_config_valid(const Configuration& config)
{
    const auto crc = crc32_compute(
        reinterpret_cast<const uint8_t *>(&config), 
        sizeof(Configuration) - sizeof(uint32_t), 
        NULL
    );

    return config.marker == marker_value && (crc == config.crc);
}

}
