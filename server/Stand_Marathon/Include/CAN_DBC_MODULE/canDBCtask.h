#include "CANdbcDriver.h"
#include "CANSignalsMap.h"

extern T_SEND_DISPATCHER SendDispatcher;
extern T_RECEIVE_DISPATCHER ReceiveDispatcher;

void canDBC_task(void *argument);
void canDBC_tick(void);

