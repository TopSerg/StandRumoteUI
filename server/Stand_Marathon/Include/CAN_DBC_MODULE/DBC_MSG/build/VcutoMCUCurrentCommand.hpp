
#pragma once
#include "CANdbcDriver.h"
#include "IMsg.hpp"
#include "optional"
extern "C" {
#include "VcutoMCUCurrentCommand.h"
}

class VcutoMCUCurrentCommand : public IMsgConv
{

public:
    float VcuIdCommand;
    float VcuIqCommand;
    uint8_t VcuCurrentCommandEnable;
    uint8_t Messagecounter300;
    uint8_t Checksum300;

    LocalErrorStats errStats = {};
    static const int cycleTime = 10;
    static const int cycleTimeReceiveGap = 10;

    int cycleTimeRemain = cycleTime;
	void pack(dbc_can_tx_message_type* transmitPacket);
	static std::optional<VcutoMCUCurrentCommand> try_unpack(dbc_can_rx_message_type* receivedPacket);
    
	c_VcutoMCUCurrentCommand toc_VcutoMCUCurrentCommand();
	VcutoMCUCurrentCommand() = default;
	VcutoMCUCurrentCommand(c_VcutoMCUCurrentCommand* self);
	VcutoMCUCurrentCommand(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats = {});
	
    virtual void msgPrepare(dbc_can_tx_message_type* transmitPacket);
	virtual bool tryMsgPrepare(dbc_can_tx_message_type* transmitPacket);
    virtual void msgParse(dbc_can_rx_message_type* receivedPacket);
	virtual bool tryMsgParse(dbc_can_rx_message_type* receivedPacket);



private:
    static const int ownCanId = 768;


	void saturateAdj();
	void rawPack(dbc_can_tx_message_type* transmitPacket);
    void UpdateGlobalState();
    void ReadGlobalState();
};
