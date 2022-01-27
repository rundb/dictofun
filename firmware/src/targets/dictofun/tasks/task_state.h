#pragma once

/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "boards/boards.h"
#include "lfs.h"

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

void application_init(lfs_t * fs);
void application_cyclic();
} // namespace application
