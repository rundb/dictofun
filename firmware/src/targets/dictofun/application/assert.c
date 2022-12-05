#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include <string.h>

void __assert(int a, char * filename, int line)
{
    uint8_t error_report[256];
    snprintf(error_report, 256, "Assertion failed: %s, line %d", filename, line);
    NRF_LOG_ERROR("%s",error_report);
    NRF_LOG_FLUSH();
    while(1);
}