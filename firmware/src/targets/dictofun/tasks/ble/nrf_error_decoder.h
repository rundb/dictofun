// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

namespace helpers
{
/// Convert ret_code_t value to human readable string
const char * decode_error(const int error_code);
}
