

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "VcuMCU01.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "VcuMCU01.h"
}

c_VcuMCU01 c_VcuMCU01_gstate;
c_VcuMCU01* cpT_VcuMCU01_gstate = &c_VcuMCU01_gstate;

void VcuMCU01::saturateAdj() {
    this->VcuMCUDesiredTorque = (float)SaturateSignalFloat(this->VcuMCUDesiredTorque, -1023, 1022);
    this->VcuBrakepedalStatus = (uint8_t)SaturateSignalInteger(this->VcuBrakepedalStatus, 0, 1);
    this->VcuMCUSurgeDamperState = (uint8_t)SaturateSignalInteger(this->VcuMCUSurgeDamperState, 0, 3);
    this->VcuKL15On = (uint8_t)SaturateSignalInteger(this->VcuKL15On, 0, 1);
    this->VcuTCSActive = (uint8_t)SaturateSignalInteger(this->VcuTCSActive, 0, 1);
    this->VcuActualGear = (uint8_t)SaturateSignalInteger(this->VcuActualGear, 0, 7);
    this->VcuMCURequestedState = (uint8_t)SaturateSignalInteger(this->VcuMCURequestedState, 0, 15);
    this->Messagecounter046 = (uint8_t)SaturateSignalInteger(this->Messagecounter046, 0, 15);
    this->Checksum046 = (uint8_t)SaturateSignalInteger(this->Checksum046, 0, 255);
};
void VcuMCU01::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 70;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuMCUDesiredTorque - -1023.0) / 1.00000001), 7, 11);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuBrakepedalStatus - 0) / 1), 11, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuMCUSurgeDamperState - 0) / 1), 10, 2);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuKL15On - 0) / 1), 8, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuTCSActive - 0) / 1), 23, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuActualGear - 0) / 1), 22, 3);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->VcuMCURequestedState - 0) / 1), 47, 4);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->Messagecounter046 - 0) / 1), 51, 4);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->Checksum046 - 0) / 1), 63, 8);
};

void VcuMCU01::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<VcuMCU01> VcuMCU01::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return VcuMCU01(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void VcuMCU01::UpdateGlobalState()
{
        cpT_VcuMCU01_gstate->VcuMCUDesiredTorque = this->VcuMCUDesiredTorque;
    cpT_VcuMCU01_gstate->VcuBrakepedalStatus = this->VcuBrakepedalStatus;
    cpT_VcuMCU01_gstate->VcuMCUSurgeDamperState = this->VcuMCUSurgeDamperState;
    cpT_VcuMCU01_gstate->VcuKL15On = this->VcuKL15On;
    cpT_VcuMCU01_gstate->VcuTCSActive = this->VcuTCSActive;
    cpT_VcuMCU01_gstate->VcuActualGear = this->VcuActualGear;
    cpT_VcuMCU01_gstate->VcuMCURequestedState = this->VcuMCURequestedState;
    cpT_VcuMCU01_gstate->Messagecounter046 = this->Messagecounter046;
    cpT_VcuMCU01_gstate->Checksum046 = this->Checksum046;

}

void VcuMCU01::ReadGlobalState()
{
        this->VcuMCUDesiredTorque = cpT_VcuMCU01_gstate->VcuMCUDesiredTorque;
    this->VcuBrakepedalStatus = cpT_VcuMCU01_gstate->VcuBrakepedalStatus;
    this->VcuMCUSurgeDamperState = cpT_VcuMCU01_gstate->VcuMCUSurgeDamperState;
    this->VcuKL15On = cpT_VcuMCU01_gstate->VcuKL15On;
    this->VcuTCSActive = cpT_VcuMCU01_gstate->VcuTCSActive;
    this->VcuActualGear = cpT_VcuMCU01_gstate->VcuActualGear;
    this->VcuMCURequestedState = cpT_VcuMCU01_gstate->VcuMCURequestedState;
    this->Messagecounter046 = cpT_VcuMCU01_gstate->Messagecounter046;
    this->Checksum046 = cpT_VcuMCU01_gstate->Checksum046;

}
    
c_VcuMCU01 VcuMCU01::toc_VcuMCU01(){
    c_VcuMCU01 out;
    out.VcuMCUDesiredTorque = this->VcuMCUDesiredTorque;
    out.VcuBrakepedalStatus = this->VcuBrakepedalStatus;
    out.VcuMCUSurgeDamperState = this->VcuMCUSurgeDamperState;
    out.VcuKL15On = this->VcuKL15On;
    out.VcuTCSActive = this->VcuTCSActive;
    out.VcuActualGear = this->VcuActualGear;
    out.VcuMCURequestedState = this->VcuMCURequestedState;
    out.Messagecounter046 = this->Messagecounter046;
    out.Checksum046 = this->Checksum046;
    return out;
};


VcuMCU01::VcuMCU01(c_VcuMCU01* self):VcuMCUDesiredTorque(self->VcuMCUDesiredTorque), VcuBrakepedalStatus(self->VcuBrakepedalStatus), VcuMCUSurgeDamperState(self->VcuMCUSurgeDamperState), VcuKL15On(self->VcuKL15On), VcuTCSActive(self->VcuTCSActive), VcuActualGear(self->VcuActualGear), VcuMCURequestedState(self->VcuMCURequestedState), Messagecounter046(self->Messagecounter046), Checksum046(self->Checksum046){};
VcuMCU01::VcuMCU01(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->VcuMCUDesiredTorque = (float)(UnpackSignalFromCANPacket(receivedPacket, 7, 11) * 1.00000001 + -1023.0);
    this->VcuBrakepedalStatus = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 11, 1) * 1 + 0);
    this->VcuMCUSurgeDamperState = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 10, 2) * 1 + 0);
    this->VcuKL15On = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 8, 1) * 1 + 0);
    this->VcuTCSActive = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 23, 1) * 1 + 0);
    this->VcuActualGear = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 22, 3) * 1 + 0);
    this->VcuMCURequestedState = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 47, 4) * 1 + 0);
    this->Messagecounter046 = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 51, 4) * 1 + 0);
    this->Checksum046 = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 63, 8) * 1 + 0);
};

void VcuMCU01::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool VcuMCU01::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void VcuMCU01::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = VcuMCU01(receivedPacket);
};
bool  VcuMCU01::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = VcuMCU01(receivedPacket);
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

void c_VcuMCU01_pack(c_VcuMCU01* self, dbc_can_tx_message_type* transmitPacket){
    VcuMCU01 _self(self);
    _self.pack(transmitPacket);
}

unpack_c_VcuMCU01_res c_VcuMCU01_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_VcuMCU01_res result;
	result.is_valid = false;
    std::optional<VcuMCU01> parsed_struct = VcuMCU01::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_VcuMCU01();
	}
    return result;
}

c_VcuMCU01 c_VcuMCU01_new(){
    c_VcuMCU01 out;
    out.pack = c_VcuMCU01_pack;
    out.unpack = c_VcuMCU01_unpack;
    return out;
}
