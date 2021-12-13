#pragma once

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
};

void task_led_init();
void task_led_cyclic();
void task_led_set_indication_state(IndicationState state);

}