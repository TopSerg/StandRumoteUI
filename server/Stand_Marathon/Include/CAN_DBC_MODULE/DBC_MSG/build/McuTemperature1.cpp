

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "McuTemperature1.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "McuTemperature1.h"
}

c_McuTemperature1 c_McuTemperature1_gstate;
c_McuTemperature1* cpT_McuTemperature1_gstate = &c_McuTemperature1_gstate;

void McuTemperature1::saturateAdj() {
    this->McuIGBTTempU = (int8_t)SaturateSignalInteger(this->McuIGBTTempU, -50, 205);
    this->McuIGBTTempV = (int8_t)SaturateSignalInteger(this->McuIGBTTempV, -50, 205);
    this->McuIGBTTempW = (int8_t)SaturateSignalInteger(this->McuIGBTTempW, -50, 205);
    this->McuIGBTTempMax = (int8_t)SaturateSignalInteger(this->McuIGBTTempMax, -50, 205);
    this->McuIGBTTempRiseU = (uint8_t)SaturateSignalInteger(this->McuIGBTTempRiseU, 0, 255);
    this->McuIGBTTempRiseV = (uint8_t)SaturateSignalInteger(this->McuIGBTTempRiseV, 0, 255);
    this->McuIGBTTempRiseW = (uint8_t)SaturateSignalInteger(this->McuIGBTTempRiseW, 0, 255);
    this->McuIGBTTempRiseMax = (uint8_t)SaturateSignalInteger(this->McuIGBTTempRiseMax, 0, 255);
};
void McuTemperature1::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 123;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempU - -50) / 1), 7, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempV - -50) / 1), 15, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempW - -50) / 1), 23, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempMax - -50) / 1), 31, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempRiseU - 0) / 1), 39, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempRiseV - 0) / 1), 47, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempRiseW - 0) / 1), 55, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIGBTTempRiseMax - 0) / 1), 63, 8);
};

void McuTemperature1::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<McuTemperature1> McuTemperature1::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return McuTemperature1(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void McuTemperature1::UpdateGlobalState()
{
        cpT_McuTemperature1_gstate->McuIGBTTempU = this->McuIGBTTempU;
    cpT_McuTemperature1_gstate->McuIGBTTempV = this->McuIGBTTempV;
    cpT_McuTemperature1_gstate->McuIGBTTempW = this->McuIGBTTempW;
    cpT_McuTemperature1_gstate->McuIGBTTempMax = this->McuIGBTTempMax;
    cpT_McuTemperature1_gstate->McuIGBTTempRiseU = this->McuIGBTTempRiseU;
    cpT_McuTemperature1_gstate->McuIGBTTempRiseV = this->McuIGBTTempRiseV;
    cpT_McuTemperature1_gstate->McuIGBTTempRiseW = this->McuIGBTTempRiseW;
    cpT_McuTemperature1_gstate->McuIGBTTempRiseMax = this->McuIGBTTempRiseMax;

}

void McuTemperature1::ReadGlobalState()
{
        this->McuIGBTTempU = cpT_McuTemperature1_gstate->McuIGBTTempU;
    this->McuIGBTTempV = cpT_McuTemperature1_gstate->McuIGBTTempV;
    this->McuIGBTTempW = cpT_McuTemperature1_gstate->McuIGBTTempW;
    this->McuIGBTTempMax = cpT_McuTemperature1_gstate->McuIGBTTempMax;
    this->McuIGBTTempRiseU = cpT_McuTemperature1_gstate->McuIGBTTempRiseU;
    this->McuIGBTTempRiseV = cpT_McuTemperature1_gstate->McuIGBTTempRiseV;
    this->McuIGBTTempRiseW = cpT_McuTemperature1_gstate->McuIGBTTempRiseW;
    this->McuIGBTTempRiseMax = cpT_McuTemperature1_gstate->McuIGBTTempRiseMax;

}
    
c_McuTemperature1 McuTemperature1::toc_McuTemperature1(){
    c_McuTemperature1 out;
    out.McuIGBTTempU = this->McuIGBTTempU;
    out.McuIGBTTempV = this->McuIGBTTempV;
    out.McuIGBTTempW = this->McuIGBTTempW;
    out.McuIGBTTempMax = this->McuIGBTTempMax;
    out.McuIGBTTempRiseU = this->McuIGBTTempRiseU;
    out.McuIGBTTempRiseV = this->McuIGBTTempRiseV;
    out.McuIGBTTempRiseW = this->McuIGBTTempRiseW;
    out.McuIGBTTempRiseMax = this->McuIGBTTempRiseMax;
    return out;
};


McuTemperature1::McuTemperature1(c_McuTemperature1* self):McuIGBTTempU(self->McuIGBTTempU), McuIGBTTempV(self->McuIGBTTempV), McuIGBTTempW(self->McuIGBTTempW), McuIGBTTempMax(self->McuIGBTTempMax), McuIGBTTempRiseU(self->McuIGBTTempRiseU), McuIGBTTempRiseV(self->McuIGBTTempRiseV), McuIGBTTempRiseW(self->McuIGBTTempRiseW), McuIGBTTempRiseMax(self->McuIGBTTempRiseMax){};
McuTemperature1::McuTemperature1(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->McuIGBTTempU = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 7, 8) * 1 + -50);
    this->McuIGBTTempV = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 15, 8) * 1 + -50);
    this->McuIGBTTempW = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 23, 8) * 1 + -50);
    this->McuIGBTTempMax = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 31, 8) * 1 + -50);
    this->McuIGBTTempRiseU = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 39, 8) * 1 + 0);
    this->McuIGBTTempRiseV = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 47, 8) * 1 + 0);
    this->McuIGBTTempRiseW = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 55, 8) * 1 + 0);
    this->McuIGBTTempRiseMax = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 63, 8) * 1 + 0);
};

void McuTemperature1::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool McuTemperature1::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void McuTemperature1::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = McuTemperature1(receivedPacket);
};
bool  McuTemperature1::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = McuTemperature1(receivedPacket);
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

void c_McuTemperature1_pack(c_McuTemperature1* self, dbc_can_tx_message_type* transmitPacket){
    McuTemperature1 _self(self);
    _self.pack(transmitPacket);
}

unpack_c_McuTemperature1_res c_McuTemperature1_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_McuTemperature1_res result;
	result.is_valid = false;
    std::optional<McuTemperature1> parsed_struct = McuTemperature1::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_McuTemperature1();
	}
    return result;
}

c_McuTemperature1 c_McuTemperature1_new(){
    c_McuTemperature1 out;
    out.pack = c_McuTemperature1_pack;
    out.unpack = c_McuTemperature1_unpack;
    return out;
}
