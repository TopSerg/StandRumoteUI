
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "McuTemperature1.h"
}

class McuTemperature1 : public IMsgConv
{

public:
    int8_t McuIGBTTempU;
    int8_t McuIGBTTempV;
    int8_t McuIGBTTempW;
    int8_t McuIGBTTempMax;
    uint8_t McuIGBTTempRiseU;
    uint8_t McuIGBTTempRiseV;
    uint8_t McuIGBTTempRiseW;
    uint8_t McuIGBTTempRiseMax;

    LocalErrorStats errStats = {};
    static const int cycleTime = 500;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<McuTemperature1> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_McuTemperature1 toc_McuTemperature1();
	McuTemperature1() = default;
	McuTemperature1(c_McuTemperature1* self);
	McuTemperature1(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 123;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
