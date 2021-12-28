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

namespace application
{

enum AppSmState
{
  APP_SM_STARTUP = 0,
  APP_SM_PREPARE = 1,
  APP_SM_REC_INIT = 10,
  APP_SM_RECORDING,
  APP_SM_CONN,
  APP_SM_XFER,
  APP_SM_FINALIZE = 50,
  APP_SM_SHUTDOWN,
};

AppSmState getApplicationState();

void application_cyclic();
}
