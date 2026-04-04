
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "build/generatedParams.hpp"
extern "C"
{
#include "DbcDispatcher.h"
#include "CANdbcDriver.h"
#include "CANSendDispatcher.h"
#include "CANReceiveDispatcher.h"
}

#define MCU_DEBUG_CYCLE 10

#define isDebugging 0


void DbcTransmitter::InitSendDispatcher()
	{
		dispatcher->loopCounter = 0;
		dispatcher->PacketsQueueFirstIndex = 0;
		dispatcher->PacketsQueueLastIndex = 0;

		dispatcher->QueueOverflow_Callback = 0;
#if isDebugging
		dispatcher->DebugSignalsIds[0] = 0x50;
		dispatcher->DebugSignalsIds[1] = 0x51;
		dispatcher->DebugSignalsIds[2] = 0x52;
		dispatcher->DebugSignalsIds[3] = 0x53;

		dispatcher->DebugSignalsMapping[0] = 0;
		dispatcher->DebugSignalsMapping[1] = 1;
		dispatcher->DebugSignalsMapping[2] = 2;
		dispatcher->DebugSignalsMapping[3] = 3;
		dispatcher->DebugSignalsMapping[4] = 4;
		dispatcher->DebugSignalsMapping[5] = 5;
		dispatcher->DebugSignalsMapping[6] = 6;
		dispatcher->DebugSignalsMapping[7] = 7;
#endif

		dispatcher->Map = &SignalsMap;
		InitSignalMap(dispatcher->Map);
#if isDebugging
		dispatcher->DebugSignalsEnabled = 1;
#endif
	}

int DbcTransmitter::IsSendQueueFull() {
		return (dispatcher->PacketsQueueLastIndex + 1) % PACKETS_SEND_QUEUE_SIZE == dispatcher->PacketsQueueFirstIndex;
	}

int DbcTransmitter::IsSendQueueEmpty() {
		return dispatcher->PacketsQueueLastIndex == dispatcher->PacketsQueueFirstIndex;
	}
#if isDebugging
void DbcTransmitter::InitPacket(dbc_can_tx_message_type* packet)
	{
		packet->extended_id = 0;
		packet->id_type = CAN_ID_STANDARD;
		packet->frame_type = CAN_TFT_DATA;
		packet->dlc = 8;
	}
#endif

void DbcTransmitter::CopySendPacket(dbc_can_tx_message_type* source, dbc_can_tx_message_type* destination)
	{
		//TODO use memcpy (it's safe?)
		destination->message_id = source->message_id;
		destination->dlc = source->dlc;
		for (int i = 0; i < 8; i++)
			destination->data[i] = source->data[i];
	}

void DbcTransmitter::EnqueueSendPacket(dbc_can_tx_message_type* packet)
	{
		if (IsSendQueueFull())
		{
			if (dispatcher->QueueOverflow_Callback != 0)
				dispatcher->QueueOverflow_Callback();
			return;
		}
		CopySendPacket(packet, &(dispatcher->PacketsQueue[dispatcher->PacketsQueueLastIndex]));
		dispatcher->PacketsQueueLastIndex = (dispatcher->PacketsQueueLastIndex + 1) % PACKETS_SEND_QUEUE_SIZE;
	}
	

uint8_t(*sendToCAN)(dbc_can_tx_message_type*);

void DbcTransmitter::SendQueue()
	{
		uint8_t isTransmitSucces = 1;
		while (!IsSendQueueEmpty() && isTransmitSucces == CAN_TX_STATUS_SUCCESS_DEFAULT)
		{
			isTransmitSucces = (*sendToCAN)(&(dispatcher->PacketsQueue[dispatcher->PacketsQueueFirstIndex]));
			if (isTransmitSucces == CAN_TX_STATUS_SUCCESS_DEFAULT)
			{
				dispatcher->PacketsQueueFirstIndex = (dispatcher->PacketsQueueFirstIndex + 1) % PACKETS_SEND_QUEUE_SIZE;
			}
		}
	}

#if isDebugging

void DbcTransmitter::SendDebugSignals()
	{
		dbc_can_tx_message_type transmitPacket;
		T_PACKED_FLOATS packer;

		InitPacket(&transmitPacket);

		for (int i = 0; i < 4; i++)
		{
			transmitPacket.standard_id = dispatcher->DebugSignalsIds[i];
			packer.Signals.data0 = *(dispatcher->Map->Table[dispatcher->DebugSignalsMapping[i * 2]]);
			packer.Signals.data1 = *(dispatcher->Map->Table[dispatcher->DebugSignalsMapping[i * 2 + 1]]);
			for (int i = 0; i < 8; i++)
			{
				transmitPacket.data[i] = packer.rawData[i];
			}
			EnqueueSendPacket(&transmitPacket);
		}
	}
#endif

	
extern IMsgConv* txArr[];

void DbcTransmitter::processTx()
	{
		

		for (auto &el : txArr) {
			dbc_can_tx_message_type transmitPacket;
			if (el->tryMsgPrepare(&transmitPacket)){
				dbc.transmitter.EnqueueSendPacket(&transmitPacket);
			}
		}
		
#if isDebugging
		if (dispatcher->DebugSignalsEnabled)
		{
			if (dispatcher->loopCounter % MCU_DEBUG_CYCLE == 0)
				SendDebugSignals();
		}
#endif

		SendQueue();

		dispatcher->loopCounter = (dispatcher->loopCounter + 1) % 1000;
	}
	
int DbcReceiver::IsReceiveQueueFull() {
		return (dispatcher->PacketsQueueLastIndex + 1) % PACKETS_RECEIVED_QUEUE_SIZE == dispatcher->PacketsQueueFirstIndex;
	}
	
int DbcReceiver::IsReceiveQueueEmpty() {
		return dispatcher->PacketsQueueLastIndex == dispatcher->PacketsQueueFirstIndex;
	}

void DbcReceiver::DequeueReceivePacket(dbc_can_rx_message_type* packet)
	{
		if (IsReceiveQueueEmpty())
			return; // User must check for empty errors itself

		CopyReceivePacket(&(dispatcher->PacketsQueue[dispatcher->PacketsQueueFirstIndex]), packet);
		dispatcher->PacketsQueueFirstIndex = (dispatcher->PacketsQueueFirstIndex + 1) % PACKETS_RECEIVED_QUEUE_SIZE;
	}

void DbcReceiver::InitReceiveDispatcher()
	{
		dispatcher->loopCounter = 0;
		dispatcher->PacketsQueueFirstIndex = 0;
		dispatcher->PacketsQueueLastIndex = 0;

		dispatcher->QueueOverflow_Callback = 0;

		dispatcher->UnexpectedID_Callback = 0;
		dispatcher->SignalMappingCallback = 0;

	}

void DbcReceiver::CopyReceivePacket(dbc_can_rx_message_type* source, dbc_can_rx_message_type* destination)
	{
		//TODO use memcpy (it's safe?)
		destination->message_id = source->message_id;
		destination->dlc = source->dlc;
		for (int i = 0; i < 8; i++)
			destination->data[i] = source->data[i];
	}



/*void DbcReceiver::ProcessSignalMappingPacket(dbc_can_rx_message_type* packet)         ?????? no possible implementation
	{
		if (dispatcher->SignalMappingCallback != 0)
			dispatcher->SignalMappingCallback(packet->data);
	}*/

	
extern IMsgConv* rxArr[];

void DbcReceiver::processRx()
	{
		dbc_can_rx_message_type packet;
		while (!IsReceiveQueueEmpty())
		{
			DequeueReceivePacket(&packet);
			

			for (auto &el : rxArr) {
				el->tryMsgParse(&packet);
			}
			if (dispatcher->UnexpectedID_Callback != 0)
				dispatcher->UnexpectedID_Callback(&packet);
		}

		dispatcher->loopCounter = (dispatcher->loopCounter + 1) % 1000;
	}

Dbc dbc;

extern "C" void EnqueueReceivePacket(dbc_can_rx_message_type* packet)
{
	if (dbc.receiver.IsReceiveQueueFull())
	{
		if (dbc.receiver.dispatcher->QueueOverflow_Callback != 0)
			dbc.receiver.dispatcher->QueueOverflow_Callback();
		return;
	}
        
	dbc.receiver.CopyReceivePacket(packet, &(dbc.receiver.dispatcher->PacketsQueue[dbc.receiver.dispatcher->PacketsQueueLastIndex]));
	dbc.receiver.dispatcher->PacketsQueueLastIndex = (dbc.receiver.dispatcher->PacketsQueueLastIndex + 1) % PACKETS_RECEIVED_QUEUE_SIZE;
}

extern "C" void dbc_transmitNextPacket()
{
	dbc.transmitter.SendQueue();
}

extern "C" void dbc_init(T_RECEIVE_DISPATCHER* receive, T_SEND_DISPATCHER* transmitter, uint8_t (*sendToCAN_ext) (dbc_can_tx_message_type*))
{
	dbc.receiver.dispatcher = receive;
	dbc.transmitter.dispatcher = transmitter;
	dbc.receiver.InitReceiveDispatcher();
	dbc.transmitter.InitSendDispatcher();
	sendToCAN = sendToCAN_ext;
}

extern "C" void dbc_process()
{
	dbc.transmitter.processTx();
	dbc.receiver.processRx();
}