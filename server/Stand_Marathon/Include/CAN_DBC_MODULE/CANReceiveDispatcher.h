#ifndef CANRECEIVEDISPATCHER_H
#define CANRECEIVEDISPATCHER_H
#include "defaultStructures.h"
//#include "at32f403a_407_can.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PACKETS_RECEIVED_QUEUE_SIZE (10)


    typedef void(*T_CAN_RECEIVE_QUEUE_OVERFLOW_CALLBACK)();

    typedef void(*T_CAN_RECEIVE_UNEXPECTED_ID_CALLBACK)(dbc_can_rx_message_type* packet);
    typedef void(*T_CAN_RECEIVE_SIGNALS_MAPPING)(uint8_t* signalMapping);

typedef struct{
    uint32_t loopCounter;
    T_CAN_RECEIVE_QUEUE_OVERFLOW_CALLBACK QueueOverflow_Callback;

    T_CAN_RECEIVE_UNEXPECTED_ID_CALLBACK UnexpectedID_Callback;
    T_CAN_RECEIVE_SIGNALS_MAPPING SignalMappingCallback;

    dbc_can_rx_message_type PacketsQueue[PACKETS_RECEIVED_QUEUE_SIZE];
	
    int8_t PacketsQueueLastIndex;
    int8_t PacketsQueueFirstIndex;

}T_RECEIVE_DISPATCHER;

	void EnqueueReceivePacket(dbc_can_rx_message_type* packet);

#ifdef __cplusplus
}
#endif

#endif
