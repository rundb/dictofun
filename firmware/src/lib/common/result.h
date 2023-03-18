// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

namespace result
{

enum class Result
{
    OK,
    ERROR_GENERAL,
    ERROR_NOT_IMPLEMENTED,
    ERROR_BUSY,
    ERROR_INVALID_PARAMETER,
    ERROR_TIMEOUT,
};

}