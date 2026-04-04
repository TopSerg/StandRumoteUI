

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "McuVCU1.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "McuVCU1.h"
}

c_McuVCU1 c_McuVCU1_gstate;
c_McuVCU1* cpT_McuVCU1_gstate = &c_McuVCU1_gstate;

void McuVCU1::saturateAdj() {
    this->McuActualTorque = (int16_t)SaturateSignalInteger(this->McuActualTorque, -1024, 1023);
    this->McuUdcCurr = (uint16_t)SaturateSignalInteger(this->McuUdcCurr, 0, 1021);
    this->McuIsCurr = (int16_t)SaturateSignalInteger(this->McuIsCurr, -1024, 1022.5);
    this->McuActualSpeed = (int16_t)SaturateSignalInteger(this->McuActualSpeed, -32768, 32767);
    this->McuActualSpeedValid = (uint8_t)SaturateSignalInteger(this->McuActualSpeedValid, 0, 1);
    this->McuActTrqValid = (uint8_t)SaturateSignalInteger(this->McuActTrqValid, 0, 1);
    this->Messagecounter7A = (uint8_t)SaturateSignalInteger(this->Messagecounter7A, 0, 15);
};
void McuVCU1::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 122;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuActualTorque - -1024) / 1), 7, 11);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuUdcCurr - 0) / 1), 12, 10);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIsCurr - -1024) / 1), 18, 11);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuActualSpeed - -32768) / 1), 39, 16);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuActualSpeedValid - 0) / 1), 53, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuActTrqValid - 0) / 1), 52, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->Messagecounter7A - 0) / 1), 51, 4);
};

void McuVCU1::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<McuVCU1> McuVCU1::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return McuVCU1(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void McuVCU1::UpdateGlobalState()
{
        cpT_McuVCU1_gstate->McuActualTorque = this->McuActualTorque;
    cpT_McuVCU1_gstate->McuUdcCurr = this->McuUdcCurr;
    cpT_McuVCU1_gstate->McuIsCurr = this->McuIsCurr;
    cpT_McuVCU1_gstate->McuActualSpeed = this->McuActualSpeed;
    cpT_McuVCU1_gstate->McuActualSpeedValid = this->McuActualSpeedValid;
    cpT_McuVCU1_gstate->McuActTrqValid = this->McuActTrqValid;
    cpT_McuVCU1_gstate->Messagecounter7A = this->Messagecounter7A;

}

void McuVCU1::ReadGlobalState()
{
        this->McuActualTorque = cpT_McuVCU1_gstate->McuActualTorque;
    this->McuUdcCurr = cpT_McuVCU1_gstate->McuUdcCurr;
    this->McuIsCurr = cpT_McuVCU1_gstate->McuIsCurr;
    this->McuActualSpeed = cpT_McuVCU1_gstate->McuActualSpeed;
    this->McuActualSpeedValid = cpT_McuVCU1_gstate->McuActualSpeedValid;
    this->McuActTrqValid = cpT_McuVCU1_gstate->McuActTrqValid;
    this->Messagecounter7A = cpT_McuVCU1_gstate->Messagecounter7A;

}
    
c_McuVCU1 McuVCU1::toc_McuVCU1(){
    c_McuVCU1 out;
    out.McuActualTorque = this->McuActualTorque;
    out.McuUdcCurr = this->McuUdcCurr;
    out.McuIsCurr = this->McuIsCurr;
    out.McuActualSpeed = this->McuActualSpeed;
    out.McuActualSpeedValid = this->McuActualSpeedValid;
    out.McuActTrqValid = this->McuActTrqValid;
    out.Messagecounter7A = this->Messagecounter7A;
    return out;
};


McuVCU1::McuVCU1(c_McuVCU1* self):McuActualTorque(self->McuActualTorque), McuUdcCurr(self->McuUdcCurr), McuIsCurr(self->McuIsCurr), McuActualSpeed(self->McuActualSpeed), McuActualSpeedValid(self->McuActualSpeedValid), McuActTrqValid(self->McuActTrqValid), Messagecounter7A(self->Messagecounter7A){};
McuVCU1::McuVCU1(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->McuActualTorque = (int16_t)(UnpackSignalFromCANPacket(receivedPacket, 7, 11) * 1 + -1024);
    this->McuUdcCurr = (uint16_t)(UnpackSignalFromCANPacket(receivedPacket, 12, 10) * 1 + 0);
    this->McuIsCurr = (int16_t)(UnpackSignalFromCANPacket(receivedPacket, 18, 11) * 1 + -1024);
    this->McuActualSpeed = (int16_t)(UnpackSignalFromCANPacket(receivedPacket, 39, 16) * 1 + -32768);
    this->McuActualSpeedValid = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 53, 1) * 1 + 0);
    this->McuActTrqValid = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 52, 1) * 1 + 0);
    this->Messagecounter7A = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 51, 4) * 1 + 0);
};

void McuVCU1::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool McuVCU1::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void McuVCU1::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = McuVCU1(receivedPacket);
};
bool  McuVCU1::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = McuVCU1(receivedPacket);
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

void c_McuVCU1_pack(c_McuVCU1* self, dbc_can_tx_message_type* transmitPacket){
    McuVCU1 _self(self);
    _self.pack(transmitPacket);
}

unpack_c_McuVCU1_res c_McuVCU1_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_McuVCU1_res result;
	result.is_valid = false;
    std::optional<McuVCU1> parsed_struct = McuVCU1::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_McuVCU1();
	}
    return result;
}

c_McuVCU1 c_McuVCU1_new(){
    c_McuVCU1 out;
    out.pack = c_McuVCU1_pack;
    out.unpack = c_McuVCU1_unpack;
    return out;
}
