#ifndef CANDBCUTILS_H
#define CANDBCUTILS_H
#include "defaultStructures.h"
//#include "at32f403a_407_can.h"

#define MESSAGECOUNTER_NOT_INIT 255
#ifdef __cplusplus
extern "C" {
#endif

    extern float SaturateSignalFloat(float signal, float minValue, float maxValue);

    extern uint8_t SaturateSignalByte(uint8_t signal, uint8_t minValue, uint8_t maxValue);
    extern uint32_t SaturateSignalInteger(int signal, int minValue, int maxValue);

    extern uint8_t IsSignalInRangeFloat(float signal, float minValue, float maxValue);

    extern uint8_t IsSignalInRangeByte(uint8_t signal, uint8_t minValue, uint8_t maxValue);
    extern uint32_t IsSignalInRangeInteger(uint32_t signal, uint32_t minValue, uint32_t maxValue);
    void PackSignalToBytes(uint8_t* data, uint32_t value, uint8_t startBit, uint8_t length);
    uint32_t UnpackSignalFromBytes(uint8_t* data, uint8_t startBit, uint8_t length);

    uint8_t IsCRCValid(dbc_can_rx_message_type* packet);
    uint8_t RolingCounterDeltaValue(uint8_t lastValue, uint8_t newValue);

    extern void PackSignalToCANPacket(dbc_can_tx_message_type* transmitPacket, uint32_t signalValue, uint8_t startBit, uint8_t length);
    extern uint32_t UnpackSignalFromCANPacket(dbc_can_rx_message_type* receivedPacket, uint8_t startBit, uint8_t length);
	extern void ClearCANDataField(dbc_can_tx_message_type* transmitPacket);

#ifdef __cplusplus
}
#endif

#endif
