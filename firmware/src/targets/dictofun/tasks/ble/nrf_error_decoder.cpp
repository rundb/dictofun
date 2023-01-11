// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "nrf_error.h"
#include <cstdint>
#include <stdint.h>

namespace helpers
{
constexpr std::size_t errors_count{20};

const char error_texts[errors_count][33] =
{
    "NRF_SUCCESS",
    "NRF_ERROR_SVC_HANDLER_MISSING",
    "NRF_ERROR_SOFTDEVICE_NOT_ENABLED",
    "NRF_ERROR_INTERNAL",
    "NRF_ERROR_NO_MEM",
    "NRF_ERROR_NOT_FOUND",
    "NRF_ERROR_NOT_SUPPORTED",
    "NRF_ERROR_INVALID_PARAM",
    "NRF_ERROR_INVALID_STATE",
    "NRF_ERROR_INVALID_LENGTH",
    "NRF_ERROR_INVALID_FLAGS",
    "NRF_ERROR_INVALID_DATA",
    "NRF_ERROR_DATA_SIZE",
    "NRF_ERROR_TIMEOUT",
    "NRF_ERROR_NULL",
    "NRF_ERROR_FORBIDDEN",
    "NRF_ERROR_INVALID_ADDR",
    "NRF_ERROR_BUSY",
    "NRF_ERROR_CONN_COUNT",
    "NRF_ERROR_RESOURCES",
};

const char * decode_error(const int error_code)
{
    if (error_code > static_cast<int>(errors_count))return "";
    return error_texts[error_code];
}

}