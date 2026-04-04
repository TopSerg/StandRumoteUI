
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "McuDeratingStatus.h"
}

class McuDeratingStatus : public IMsgConv
{

public:
    uint8_t McubDernOvrIdc;
    uint8_t McubDernOvrUdc;
    uint8_t McubDernStrTemp;
    uint8_t McubDernTrqMax;
    uint8_t McubDernTrqMin;
    uint8_t McubDernUndrIdc;
    uint8_t McubDernUndrUdc;
    uint8_t McubReadyHv;
    uint8_t McubDernTempIGBT;
    uint8_t McubDernTempCool;
    uint8_t McubDernN;
    int16_t McuTrqAbsMax;
    int16_t McuTrqAbsMin;
    uint8_t McustDischg;
    float McuUT15Curr;

    LocalErrorStats errStats = {};
    static const int cycleTime = 100;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<McuDeratingStatus> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_McuDeratingStatus toc_McuDeratingStatus();
	McuDeratingStatus() = default;
	McuDeratingStatus(c_McuDeratingStatus* self);
	McuDeratingStatus(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 709;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
