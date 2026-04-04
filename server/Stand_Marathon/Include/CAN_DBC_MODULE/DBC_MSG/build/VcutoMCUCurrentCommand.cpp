

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "VcutoMCUCurrentCommand.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "VcutoMCUCurrentCommand.h"
}

c_VcutoMCUCurrentCommand c_VcutoMCUCurrentCommand_gstate;
c_VcutoMCUCurrentCommand* cpT_VcutoMCUCurrentCommand_gstate = &c_VcutoMCUCurrentCommand_gstate;

void VcutoMCUCurrentCommand::saturateAdj() {
    this->VcuIdCommand = (float)SaturateSignalFloat(this->VcuIdCommand, -320, 319.9);
    this->VcuIqCommand = (float)SaturateSignalFloat(this->VcuIqCommand, -320, 319.9);
    this->VcuCurrentCommandEnable = (uint8_t)SaturateSignalInteger(this->VcuCurrentCommandEnable, 0, 1);
    this->Messagecounter300 = (uint8_t)SaturateSignalInteger(this->Messagecounter300, 0, 15);
    this->Checksum300 = (uint8_t)SaturateSignalInteger(this->Checksum300, 0, 255);
};
void VcutoMCUCurrentCommand::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 768;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuIdCommand - -320) / 0.1), 7, 13);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuIqCommand - -320) / 0.1), 23, 13);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuCurrentCommandEnable - 0) / 1), 39, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->Messagecounter300 - 0) / 1), 51, 4);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->Checksum300 - 0) / 1), 63, 8);
};

void VcutoMCUCurrentCommand::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<VcutoMCUCurrentCommand> VcutoMCUCurrentCommand::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return VcutoMCUCurrentCommand(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void VcutoMCUCurrentCommand::UpdateGlobalState()
{
        cpT_VcutoMCUCurrentCommand_gstate->VcuIdCommand = this->VcuIdCommand;
    cpT_VcutoMCUCurrentCommand_gstate->VcuIqCommand = this->VcuIqCommand;
    cpT_VcutoMCUCurrentCommand_gstate->VcuCurrentCommandEnable = this->VcuCurrentCommandEnable;
    cpT_VcutoMCUCurrentCommand_gstate->Messagecounter300 = this->Messagecounter300;
    cpT_VcutoMCUCurrentCommand_gstate->Checksum300 = this->Checksum300;

}

void VcutoMCUCurrentCommand::ReadGlobalState()
{
        this->VcuIdCommand = cpT_VcutoMCUCurrentCommand_gstate->VcuIdCommand;
    this->VcuIqCommand = cpT_VcutoMCUCurrentCommand_gstate->VcuIqCommand;
    this->VcuCurrentCommandEnable = cpT_VcutoMCUCurrentCommand_gstate->VcuCurrentCommandEnable;
    this->Messagecounter300 = cpT_VcutoMCUCurrentCommand_gstate->Messagecounter300;
    this->Checksum300 = cpT_VcutoMCUCurrentCommand_gstate->Checksum300;

}
    
c_VcutoMCUCurrentCommand VcutoMCUCurrentCommand::toc_VcutoMCUCurrentCommand(){
    c_VcutoMCUCurrentCommand out;
    out.VcuIdCommand = this->VcuIdCommand;
    out.VcuIqCommand = this->VcuIqCommand;
    out.VcuCurrentCommandEnable = this->VcuCurrentCommandEnable;
    out.Messagecounter300 = this->Messagecounter300;
    out.Checksum300 = this->Checksum300;
    return out;
};


VcutoMCUCurrentCommand::VcutoMCUCurrentCommand(c_VcutoMCUCurrentCommand* self):VcuIdCommand(self->VcuIdCommand), VcuIqCommand(self->VcuIqCommand), VcuCurrentCommandEnable(self->VcuCurrentCommandEnable), Messagecounter300(self->Messagecounter300), Checksum300(self->Checksum300){};
VcutoMCUCurrentCommand::VcutoMCUCurrentCommand(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->VcuIdCommand = (float)(UnpackSignalFromCANPacket(receivedPacket, 7, 13) * 0.1 + -320);
    this->VcuIqCommand = (float)(UnpackSignalFromCANPacket(receivedPacket, 23, 13) * 0.1 + -320);
    this->VcuCurrentCommandEnable = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 39, 1) * 1 + 0);
    this->Messagecounter300 = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 51, 4) * 1 + 0);
    this->Checksum300 = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 63, 8) * 1 + 0);
};

void VcutoMCUCurrentCommand::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool VcutoMCUCurrentCommand::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
{
	if (--cycleTimeRemain <= 0)
	{
        this->ReadGlobalState();
        cycleTimeRemain = cycleTime;
		msgPrepare(transmitPacket);
		return true;
	}
	return false;
}

void VcutoMCUCurrentCommand::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = VcutoMCUCurrentCommand(receivedPacket);
};
bool  VcutoMCUCurrentCommand::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = VcutoMCUCurrentCommand(receivedPacket);
            this->UpdateGlobalState();
            return true;
        }  else {
            this->errStats.crcError++;
            return false;
        }
    } 
    else {
        if ((0 - cycleTimeReceiveGap) > cycleTimeRemain){ this->errStats.timeOut++; }
        return false;
    }

};

void c_VcutoMCUCurrentCommand_pack(c_VcutoMCUCurrentCommand* self, dbc_can_tx_message_type* transmitPacket){
    VcutoMCUCurrentCommand _self(self);
    _self.pack(transmitPacket);
}

unpack_c_VcutoMCUCurrentCommand_res c_VcutoMCUCurrentCommand_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_VcutoMCUCurrentCommand_res result;
	result.is_valid = false;
    std::optional<VcutoMCUCurrentCommand> parsed_struct = VcutoMCUCurrentCommand::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_VcutoMCUCurrentCommand();
	}
    return result;
}

c_VcutoMCUCurrentCommand c_VcutoMCUCurrentCommand_new(){
    c_VcutoMCUCurrentCommand out;
    out.pack = c_VcutoMCUCurrentCommand_pack;
    out.unpack = c_VcutoMCUCurrentCommand_unpack;
    return out;
}
