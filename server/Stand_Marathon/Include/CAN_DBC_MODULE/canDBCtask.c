#include "DbcDispatcher.h"
#include "canDBCtask.h"

T_SEND_DISPATCHER SendDispatcher;
T_RECEIVE_DISPATCHER ReceiveDispatcher;

void canDBC_tick(void)
{
	/*DBC process*/\
	dbc_process();

}