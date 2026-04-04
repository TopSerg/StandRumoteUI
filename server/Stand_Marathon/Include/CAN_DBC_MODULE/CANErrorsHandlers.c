#include "CANErrorsHandlers.h"

T_ERROR_COUNTERS ErrorCounters;

void SendQueueOverflowHandler()
{
    ErrorCounters.SendOverflow++;
}

void ReceiveQueueOverflowHandler()
{
    ErrorCounters.ReceiveOverflow++;
}

void CRCErrorHandler(dbc_can_rx_message_type* packet)
{
    ErrorCounters.CRCError++;
}

void MessageCounter046WrongDeltaHandler(uint8_t delta)
{
    ErrorCounters.Message46WrongDelta++;
}

void MessageCounter047WrongDeltaHandler(uint8_t delta)
{
    ErrorCounters.Message47WrongDelta++;
}

void Message046TimeoutHandler(uint32_t time_ms)
{
    ErrorCounters.Message46NotInTime++;
}

void Message047TimeoutHandler(uint32_t time_ms)
{
    ErrorCounters.Message47NotInTime++;
}
