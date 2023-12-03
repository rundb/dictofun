// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

namespace wdt
{
void task_wdt(void* context);
}
