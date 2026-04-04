#pragma once
#include "CANSendDispatcher.h"
#include "CANReceiveDispatcher.h"

void dbc_init(T_RECEIVE_DISPATCHER* receive, T_SEND_DISPATCHER* send, uint8_t(*sendToCAN_ext)(dbc_can_tx_message_type*));
void dbc_process();