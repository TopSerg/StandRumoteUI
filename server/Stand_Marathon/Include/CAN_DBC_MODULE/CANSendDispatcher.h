#ifndef CANSENDDISPATCHER_H
#define CANSENDDISPATCHER_H
#include "defaultStructures.h"
//#include "at32f403a_407_can.h"
#include "CANSignalsMap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PACKETS_SEND_QUEUE_SIZE (10)
    typedef void(*T_CAN_SEND_QUEUE_OVERFLOW_CALLBACK)();

typedef struct{
    uint32_t loopCounter;
    uint8_t DebugSignalsEnabled;
    uint8_t DebugSignalsMapping[8];
    uint32_t DebugSignalsIds[4];
    T_SIGNALS_MAP* Map;

    T_CAN_SEND_QUEUE_OVERFLOW_CALLBACK QueueOverflow_Callback;

    dbc_can_tx_message_type PacketsQueue[PACKETS_SEND_QUEUE_SIZE];
	uint8_t(*sendToCan)(dbc_can_tx_message_type *);
    int8_t PacketsQueueLastIndex;
    int8_t PacketsQueueFirstIndex;

}T_SEND_DISPATCHER;

void dbc_transmitNextPacket();
	
#ifdef __cplusplus
}
#endif

#endif
