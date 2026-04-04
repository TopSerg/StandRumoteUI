

#pragma once
#ifndef MCU_ST_H
#define MCU_ST_H
//#include "at32f403a_407_can.h"
#include "defaultStructures.h"
struct GlobalErrorStats
{
	int SendOverflow;
	int ReceiveOverflow;
};

struct LocalErrorStats
{
	int crcError;
	int timeOut;
	int deltaError;
};

static void InitPacket(dbc_can_tx_message_type* packet)
{
	packet->message_id = 0;
	packet->dlc = 8;
}

struct IMsgConv
{
	virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket) = 0;
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket) = 0;
	
	virtual void msgParse(dbc_can_rx_message_type* receivedPacket) = 0;
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket) = 0;
};

#endif