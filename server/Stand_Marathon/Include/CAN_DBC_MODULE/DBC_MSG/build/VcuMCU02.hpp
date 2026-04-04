
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "VcuMCU02.h"
}

class VcuMCU02 : public IMsgConv
{

public:
    float VcuMinTorqueLimit;
    float VcuMaxTorqueGradient;
    float VcuMaxTorqueLimit;
    float VcuTrqThresholdDampgCtl;
    uint8_t Messagecounter047;
    uint8_t Checksum047;

    LocalErrorStats errStats = {};
    static const int cycleTime = 20;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<VcuMCU02> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_VcuMCU02 toc_VcuMCU02();
	VcuMCU02() = default;
	VcuMCU02(c_VcuMCU02* self);
	VcuMCU02(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 71;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
