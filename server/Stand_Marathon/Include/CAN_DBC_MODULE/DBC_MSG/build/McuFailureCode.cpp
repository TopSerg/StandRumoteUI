

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "McuFailureCode.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "McuFailureCode.h"
}

c_McuFailureCode c_McuFailureCode_gstate;
c_McuFailureCode* cpT_McuFailureCode_gstate = &c_McuFailureCode_gstate;

void McuFailureCode::saturateAdj() {
    this->McuFailCode1 = (uint8_t)SaturateSignalInteger(this->McuFailCode1, 0, 7);
    this->McuSoftwaeFaultBit = (uint32_t)SaturateSignalInteger(this->McuSoftwaeFaultBit, 0, 524287);
    this->McuHardwareFault = (uint16_t)SaturateSignalInteger(this->McuHardwareFault, 0, 65535);
    this->McuSensorFault = (uint8_t)SaturateSignalInteger(this->McuSensorFault, 0, 255);
    this->McuCANFault = (uint8_t)SaturateSignalInteger(this->McuCANFault, 0, 255);
};
void McuFailureCode::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 710;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuFailCode1 - 0) / 1), 7, 3);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuSoftwaeFaultBit - 0) / 1), 15, 19);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuHardwareFault - 0) / 1), 39, 16);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuSensorFault - 0) / 1), 55, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuCANFault - 0) / 1), 63, 8);
};

void McuFailureCode::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<McuFailureCode> McuFailureCode::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return McuFailureCode(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void McuFailureCode::UpdateGlobalState()
{
        cpT_McuFailureCode_gstate->McuFailCode1 = this->McuFailCode1;
    cpT_McuFailureCode_gstate->McuSoftwaeFaultBit = this->McuSoftwaeFaultBit;
    cpT_McuFailureCode_gstate->McuHardwareFault = this->McuHardwareFault;
    cpT_McuFailureCode_gstate->McuSensorFault = this->McuSensorFault;
    cpT_McuFailureCode_gstate->McuCANFault = this->McuCANFault;

}

void McuFailureCode::ReadGlobalState()
{
        this->McuFailCode1 = cpT_McuFailureCode_gstate->McuFailCode1;
    this->McuSoftwaeFaultBit = cpT_McuFailureCode_gstate->McuSoftwaeFaultBit;
    this->McuHardwareFault = cpT_McuFailureCode_gstate->McuHardwareFault;
    this->McuSensorFault = cpT_McuFailureCode_gstate->McuSensorFault;
    this->McuCANFault = cpT_McuFailureCode_gstate->McuCANFault;

}
    
c_McuFailureCode McuFailureCode::toc_McuFailureCode(){
    c_McuFailureCode out;
    out.McuFailCode1 = this->McuFailCode1;
    out.McuSoftwaeFaultBit = this->McuSoftwaeFaultBit;
    out.McuHardwareFault = this->McuHardwareFault;
    out.McuSensorFault = this->McuSensorFault;
    out.McuCANFault = this->McuCANFault;
    return out;
};


McuFailureCode::McuFailureCode(c_McuFailureCode* self):McuFailCode1(self->McuFailCode1), McuSoftwaeFaultBit(self->McuSoftwaeFaultBit), McuHardwareFault(self->McuHardwareFault), McuSensorFault(self->McuSensorFault), McuCANFault(self->McuCANFault){};
McuFailureCode::McuFailureCode(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->McuFailCode1 = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 7, 3) * 1 + 0);
    this->McuSoftwaeFaultBit = (uint32_t)(UnpackSignalFromCANPacket(receivedPacket, 15, 19) * 1 + 0);
    this->McuHardwareFault = (uint16_t)(UnpackSignalFromCANPacket(receivedPacket, 39, 16) * 1 + 0);
    this->McuSensorFault = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 55, 8) * 1 + 0);
    this->McuCANFault = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 63, 8) * 1 + 0);
};

void McuFailureCode::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool McuFailureCode::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void McuFailureCode::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = McuFailureCode(receivedPacket);
};
bool  McuFailureCode::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = McuFailureCode(receivedPacket);
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

void c_McuFailureCode_pack(c_McuFailureCode* self, dbc_can_tx_message_type* transmitPacket){
    McuFailureCode _self(self);
    _self.pack(transmitPacket);
}

unpack_c_McuFailureCode_res c_McuFailureCode_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_McuFailureCode_res result;
	result.is_valid = false;
    std::optional<McuFailureCode> parsed_struct = McuFailureCode::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_McuFailureCode();
	}
    return result;
}

c_McuFailureCode c_McuFailureCode_new(){
    c_McuFailureCode out;
    out.pack = c_McuFailureCode_pack;
    out.unpack = c_McuFailureCode_unpack;
    return out;
}
