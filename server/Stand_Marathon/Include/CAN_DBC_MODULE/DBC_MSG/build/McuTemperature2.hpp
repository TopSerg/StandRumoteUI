
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "McuTemperature2.h"
}

class McuTemperature2 : public IMsgConv
{

public:
    int8_t McuTempCurrCool;
    int8_t McuTempCurrStr1;
    int8_t McuTempCurrStr2;
    int8_t McuTempCurrStr;
    uint8_t McuTempRiseCurrStr;
    uint8_t McuTempStatus;

    LocalErrorStats errStats = {};
    static const int cycleTime = 500;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<McuTemperature2> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_McuTemperature2 toc_McuTemperature2();
	McuTemperature2() = default;
	McuTemperature2(c_McuTemperature2* self);
	McuTemperature2(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 124;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
