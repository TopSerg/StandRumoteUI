
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "McuCurrentVoltage.h"
}

class McuCurrentVoltage : public IMsgConv
{

public:
    int16_t Ud;
    int16_t Uq;
    int16_t Id;
    int16_t Iq;

    LocalErrorStats errStats = {};
    static const int cycleTime = 500;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<McuCurrentVoltage> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_McuCurrentVoltage toc_McuCurrentVoltage();
	McuCurrentVoltage() = default;
	McuCurrentVoltage(c_McuCurrentVoltage* self);
	McuCurrentVoltage(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 126;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
