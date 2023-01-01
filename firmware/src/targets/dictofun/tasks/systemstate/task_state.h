// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "boards.h"

namespace systemstate
{
void task_system_state(void *);
}

namespace application
{

enum class AppSmState
{
    INIT,
    PREPARE,
    RECORD_START,
    RECORD,
    RECORD_FINALIZATION,
    CONNECT,
    TRANSFER,
    DISCONNECT,
    FINALIZE,
    SHUTDOWN,
    RESTART
};

AppSmState getApplicationState();

void application_init();
void application_cyclic();
} // namespace application
