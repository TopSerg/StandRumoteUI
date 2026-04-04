
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "McuFailureCode.h"
}

class McuFailureCode : public IMsgConv
{

public:
    uint8_t McuFailCode1;
    uint32_t McuSoftwaeFaultBit;
    uint16_t McuHardwareFault;
    uint8_t McuSensorFault;
    uint8_t McuCANFault;

    LocalErrorStats errStats = {};
    static const int cycleTime = 100;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<McuFailureCode> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_McuFailureCode toc_McuFailureCode();
	McuFailureCode() = default;
	McuFailureCode(c_McuFailureCode* self);
	McuFailureCode(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 710;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
