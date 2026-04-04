#ifndef CANERRORSHANDLERS_H
#define CANERRORSHANDLERS_H
#include "defaultStructures.h"
//#include "at32f403a_407_can.h"
#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct
	{
		int SendOverflow;
		int ReceiveOverflow;
		int CRCError;
		int Message46WrongDelta;
		int Message47WrongDelta;
		int Message46NotInTime;
		int Message47NotInTime;
	}T_ERROR_COUNTERS;

	extern T_ERROR_COUNTERS ErrorCounters;
	
    extern void SendQueueOverflowHandler();
    extern void ReceiveQueueOverflowHandler();
	extern void CRCErrorHandler(dbc_can_rx_message_type* packet);
    extern void MessageCounter046WrongDeltaHandler(uint8_t delta);
    extern void MessageCounter047WrongDeltaHandler(uint8_t delta);
    extern void Message046TimeoutHandler(uint32_t time_ms);
    extern void Message047TimeoutHandler(uint32_t time_ms);
	
#ifdef __cplusplus
}
#endif

#endif
