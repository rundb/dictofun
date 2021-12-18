#ifndef _TASK_STATE_H_
#define _TASK_STATE_H_

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

#endif //_TASK_STATE_H_