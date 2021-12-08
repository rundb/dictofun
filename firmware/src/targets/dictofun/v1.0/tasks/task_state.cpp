#include "tasks/task_state.h"
#include <nrf_gpio.h>

namespace application
{
AppSmState _applicationState{APP_SM_STARTUP};

AppSmState getApplicationState()
{
    return _applicationState;
}

void application_cyclic()
{
    const auto prevState = _applicationState;
    switch (_applicationState)
    {
        case APP_SM_STARTUP:
        {
            _applicationState = APP_SM_REC_INIT;
            break;
        }
        case APP_SM_REC_INIT:
        {
            _applicationState = APP_SM_RECORDING;
            break;
        }
        case APP_SM_RECORDING:
        {
            // if button is released - go further
            _applicationState = APP_SM_FINALISE;
            break;
        }
        case APP_SM_CONN:
        case APP_SM_XFER:
        case APP_SM_FINALISE:
        {
            // trigger flash memory erase

            // after memory is erased - shutdown
            break;
        }
        case APP_SM_SHUTDOWN:
        {
            // disable LDO and enter low-power sleep mode
            nrf_gpio_pin_clear(LDO_EN_PIN);
            while(1);
            break;
        }
    }
    if (prevState != _applicationState)
    {
        // TODO: log state change
    }
}
}
