#include "BleSystem.h"
#include <libraries/log/nrf_log_ctrl.h>
#include <libraries/log/nrf_log.h>
#include <libraries/log/nrf_log_default_backends.h>

static void log_init();
static void idle_state_handle();

ble::BleSystem bleSystem{};

int main()
{
    log_init();
    application_init();
    bleSystem.init();
    for (;;)
    {
        application_cyclic();
        bleSystem.cyclic();
        idle_state_handle();
    }
}

// Application helper functions

uint32_t get_timestamp()
{
    return app_timer_cnt_get();
}

static void log_init()
{
    ret_code_t err_code = NRF_LOG_INIT(get_timestamp);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void idle_state_handle()
{
    NRF_LOG_PROCESS();
}

uint32_t get_timestamp_delta(uint32_t base)
{
  uint32_t now = app_timer_cnt_get();

  if(base > now)
    return 0;
  else
    return  now - base;
}
