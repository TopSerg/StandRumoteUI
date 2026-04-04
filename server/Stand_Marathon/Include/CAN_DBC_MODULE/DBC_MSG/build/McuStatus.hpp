
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "McuStatus.h"
}

class McuStatus : public IMsgConv
{

public:
    float McuOfsAl;
    float McuIsd;
    float McuIsq;
    uint8_t McubDmpCActv;
    uint8_t McustGateDrv;
    float McuDmpCTrqCurr;
    uint8_t McuVCUWorkMode;

    LocalErrorStats errStats = {};
    static const int cycleTime = 20;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<McuStatus> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_McuStatus toc_McuStatus();
	McuStatus() = default;
	McuStatus(c_McuStatus* self);
	McuStatus(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 125;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
