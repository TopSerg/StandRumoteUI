
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "VcuMCU01.h"
}

class VcuMCU01 : public IMsgConv
{

public:
    float VcuMCUDesiredTorque;
    uint8_t VcuBrakepedalStatus;
    uint8_t VcuMCUSurgeDamperState;
    uint8_t VcuKL15On;
    uint8_t VcuTCSActive;
    uint8_t VcuActualGear;
    uint8_t VcuMCURequestedState;
    uint8_t Messagecounter046;
    uint8_t Checksum046;

    LocalErrorStats errStats = {};
    static const int cycleTime = 10;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<VcuMCU01> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_VcuMCU01 toc_VcuMCU01();
	VcuMCU01() = default;
	VcuMCU01(c_VcuMCU01* self);
	VcuMCU01(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 70;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
