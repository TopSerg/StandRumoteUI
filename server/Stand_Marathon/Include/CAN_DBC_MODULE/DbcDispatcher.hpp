#pragma once
extern "C"
{
#include "DbcDispatcher.h"

}
class DbcTransmitter;
class DbcReceiver;
struct Dbc;
	
extern Dbc dbc;

typedef union
{
	struct
	{
		float data0;
		float data1;
	}Signals;
	uint8_t rawData[8];
}T_PACKED_FLOATS;

class DbcTransmitter
{
	T_SEND_DISPATCHER* dispatcher;
	
	void InitSendDispatcher();
	int IsSendQueueFull();
	int IsSendQueueEmpty();
	void InitPacket(dbc_can_tx_message_type* packet);
	void CopySendPacket(dbc_can_tx_message_type* source, dbc_can_tx_message_type* destination);
	void SendQueue();
	void SendDebugSignals();
	friend void dbc_transmitNextPacket();
	friend void dbc_init(T_RECEIVE_DISPATCHER* receive, T_SEND_DISPATCHER* send, uint8_t(*sendToCAN_ext)(dbc_can_tx_message_type*));
	friend void dbc_process();

public:
	void EnqueueSendPacket(dbc_can_tx_message_type* packet);
	void processTx();
};

class DbcReceiver
{
	T_RECEIVE_DISPATCHER* dispatcher;
	
	int IsReceiveQueueFull();
	void DequeueReceivePacket(dbc_can_rx_message_type* packet);
	void InitReceiveDispatcher();
	void CopyReceivePacket(dbc_can_rx_message_type* source, dbc_can_rx_message_type* destination);
	void ProcessSignalMappingPacket(dbc_can_rx_message_type* packet);
	
	int IsReceiveQueueEmpty();
	friend void EnqueueReceivePacket(dbc_can_rx_message_type* packet);
	friend void dbc_init(T_RECEIVE_DISPATCHER* receive, T_SEND_DISPATCHER* send, uint8_t(*sendToCAN_ext)(dbc_can_tx_message_type*));
	friend void dbc_process();

public:
	void processRx();

};

struct Dbc
{
	DbcTransmitter transmitter;
	DbcReceiver receiver;
};
