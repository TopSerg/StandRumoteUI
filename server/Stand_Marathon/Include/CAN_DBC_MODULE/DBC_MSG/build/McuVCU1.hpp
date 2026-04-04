
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "McuVCU1.h"
}

class McuVCU1 : public IMsgConv
{

public:
    int16_t McuActualTorque;
    uint16_t McuUdcCurr;
    int16_t McuIsCurr;
    int16_t McuActualSpeed;
    uint8_t McuActualSpeedValid;
    uint8_t McuActTrqValid;
    uint8_t Messagecounter7A;

    LocalErrorStats errStats = {};
    static const int cycleTime = 10;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<McuVCU1> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_McuVCU1 toc_McuVCU1();
	McuVCU1() = default;
	McuVCU1(c_McuVCU1* self);
	McuVCU1(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 122;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
