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

namespace led
{
// This task is responsible for the whole LED management.
// Separation of this logic into a separate unit allows us to
// concentrate all indication-dependencies inside one unit.

enum LedState
{
    OFF,
    SLOW_BLINKING,
    FAST_BLINKING,
    ON
};

enum LedColor
{
    RED = 0,
    GREEN = 1,
    BLUE = 2,
    COLORS_COUNT
};

enum IndicationState
{
    PREPARING,
    RECORDING,
    CONNECTING,
    SENDING,
    SHUTTING_DOWN,
    INDICATION_OFF,
};

void task_led_init();
void task_led_cyclic();
void task_led_set_indication_state(IndicationState state);

}