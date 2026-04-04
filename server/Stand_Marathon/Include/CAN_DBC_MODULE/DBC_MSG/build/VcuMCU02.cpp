

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "VcuMCU02.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "VcuMCU02.h"
}

c_VcuMCU02 c_VcuMCU02_gstate;
c_VcuMCU02* cpT_VcuMCU02_gstate = &c_VcuMCU02_gstate;

void VcuMCU02::saturateAdj() {
    this->VcuMinTorqueLimit = (float)SaturateSignalFloat(this->VcuMinTorqueLimit, -1023, 1022);
    this->VcuMaxTorqueGradient = (float)SaturateSignalFloat(this->VcuMaxTorqueGradient, 0, 524192);
    this->VcuMaxTorqueLimit = (float)SaturateSignalFloat(this->VcuMaxTorqueLimit, -1023, 1022);
    this->VcuTrqThresholdDampgCtl = (float)SaturateSignalFloat(this->VcuTrqThresholdDampgCtl, 0, 25);
    this->Messagecounter047 = (uint8_t)SaturateSignalInteger(this->Messagecounter047, 0, 15);
    this->Checksum047 = (uint8_t)SaturateSignalInteger(this->Checksum047, 0, 255);
};
void VcuMCU02::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 71;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuMinTorqueLimit - -1023) / 1.00000001), 7, 11);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuMaxTorqueGradient - 0) / 32.00000001), 12, 14);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuMaxTorqueLimit - -1023) / 1.00000001), 30, 11);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuTrqThresholdDampgCtl - 0) / 0.1), 35, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->Messagecounter047 - 0) / 1), 51, 4);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->Checksum047 - 0) / 1), 63, 8);
};

void VcuMCU02::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<VcuMCU02> VcuMCU02::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return VcuMCU02(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void VcuMCU02::UpdateGlobalState()
{
        cpT_VcuMCU02_gstate->VcuMinTorqueLimit = this->VcuMinTorqueLimit;
    cpT_VcuMCU02_gstate->VcuMaxTorqueGradient = this->VcuMaxTorqueGradient;
    cpT_VcuMCU02_gstate->VcuMaxTorqueLimit = this->VcuMaxTorqueLimit;
    cpT_VcuMCU02_gstate->VcuTrqThresholdDampgCtl = this->VcuTrqThresholdDampgCtl;
    cpT_VcuMCU02_gstate->Messagecounter047 = this->Messagecounter047;
    cpT_VcuMCU02_gstate->Checksum047 = this->Checksum047;

}

void VcuMCU02::ReadGlobalState()
{
        this->VcuMinTorqueLimit = cpT_VcuMCU02_gstate->VcuMinTorqueLimit;
    this->VcuMaxTorqueGradient = cpT_VcuMCU02_gstate->VcuMaxTorqueGradient;
    this->VcuMaxTorqueLimit = cpT_VcuMCU02_gstate->VcuMaxTorqueLimit;
    this->VcuTrqThresholdDampgCtl = cpT_VcuMCU02_gstate->VcuTrqThresholdDampgCtl;
    this->Messagecounter047 = cpT_VcuMCU02_gstate->Messagecounter047;
    this->Checksum047 = cpT_VcuMCU02_gstate->Checksum047;

}
    
c_VcuMCU02 VcuMCU02::toc_VcuMCU02(){
    c_VcuMCU02 out;
    out.VcuMinTorqueLimit = this->VcuMinTorqueLimit;
    out.VcuMaxTorqueGradient = this->VcuMaxTorqueGradient;
    out.VcuMaxTorqueLimit = this->VcuMaxTorqueLimit;
    out.VcuTrqThresholdDampgCtl = this->VcuTrqThresholdDampgCtl;
    out.Messagecounter047 = this->Messagecounter047;
    out.Checksum047 = this->Checksum047;
    return out;
};


VcuMCU02::VcuMCU02(c_VcuMCU02* self):VcuMinTorqueLimit(self->VcuMinTorqueLimit), VcuMaxTorqueGradient(self->VcuMaxTorqueGradient), VcuMaxTorqueLimit(self->VcuMaxTorqueLimit), VcuTrqThresholdDampgCtl(self->VcuTrqThresholdDampgCtl), Messagecounter047(self->Messagecounter047), Checksum047(self->Checksum047){};
VcuMCU02::VcuMCU02(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->VcuMinTorqueLimit = (float)(UnpackSignalFromCANPacket(receivedPacket, 7, 11) * 1.00000001 + -1023);
    this->VcuMaxTorqueGradient = (float)(UnpackSignalFromCANPacket(receivedPacket, 12, 14) * 32.00000001 + 0);
    this->VcuMaxTorqueLimit = (float)(UnpackSignalFromCANPacket(receivedPacket, 30, 11) * 1.00000001 + -1023);
    this->VcuTrqThresholdDampgCtl = (float)(UnpackSignalFromCANPacket(receivedPacket, 35, 8) * 0.1 + 0);
    this->Messagecounter047 = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 51, 4) * 1 + 0);
    this->Checksum047 = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 63, 8) * 1 + 0);
};

void VcuMCU02::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool VcuMCU02::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void VcuMCU02::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = VcuMCU02(receivedPacket);
};
bool  VcuMCU02::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = VcuMCU02(receivedPacket);
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

void c_VcuMCU02_pack(c_VcuMCU02* self, dbc_can_tx_message_type* transmitPacket){
    VcuMCU02 _self(self);
    _self.pack(transmitPacket);
}

unpack_c_VcuMCU02_res c_VcuMCU02_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_VcuMCU02_res result;
	result.is_valid = false;
    std::optional<VcuMCU02> parsed_struct = VcuMCU02::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_VcuMCU02();
	}
    return result;
}

c_VcuMCU02 c_VcuMCU02_new(){
    c_VcuMCU02 out;
    out.pack = c_VcuMCU02_pack;
    out.unpack = c_VcuMCU02_unpack;
    return out;
}
