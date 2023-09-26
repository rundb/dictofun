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

const char error_texts[errors_count][23] = {
    "NRF_SUCCESS",
    "SVC_HANDLER_MISSING",
    "SOFTDEVICE_NOT_ENABLED",
    "INTERNAL",
    "NO_MEM",
    "NOT_FOUND",
    "NOT_SUPPORTED",
    "INVALID_PARAM",
    "INVALID_STATE",
    "INVALID_LENGTH",
    "INVALID_FLAGS",
    "INVALID_DATA",
    "DATA_SIZE",
    "TIMEOUT",
    "NULL",
    "FORBIDDEN",
    "INVALID_ADDR",
    "BUSY",
    "CONN_COUNT",
    "RESOURCES",
};

const char* decode_error(const int error_code)
{
    if(error_code > static_cast<int>(errors_count))
        return "";
    return error_texts[error_code];
}

} // namespace helpers
