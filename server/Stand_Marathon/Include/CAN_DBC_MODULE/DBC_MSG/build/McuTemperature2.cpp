

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "McuTemperature2.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "McuTemperature2.h"
}

c_McuTemperature2 c_McuTemperature2_gstate;
c_McuTemperature2* cpT_McuTemperature2_gstate = &c_McuTemperature2_gstate;

void McuTemperature2::saturateAdj() {
    this->McuTempCurrCool = (int8_t)SaturateSignalInteger(this->McuTempCurrCool, -50, 205);
    this->McuTempCurrStr1 = (int8_t)SaturateSignalInteger(this->McuTempCurrStr1, -50, 205);
    this->McuTempCurrStr2 = (int8_t)SaturateSignalInteger(this->McuTempCurrStr2, -50, 205);
    this->McuTempCurrStr = (int8_t)SaturateSignalInteger(this->McuTempCurrStr, -50, 205);
    this->McuTempRiseCurrStr = (uint8_t)SaturateSignalInteger(this->McuTempRiseCurrStr, 0, 255);
    this->McuTempStatus = (uint8_t)SaturateSignalInteger(this->McuTempStatus, 0, 3);
};
void McuTemperature2::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 124;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTempCurrCool - -50) / 1), 7, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTempCurrStr1 - -50) / 1), 15, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTempCurrStr2 - -50) / 1), 23, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTempCurrStr - -50) / 1), 31, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTempRiseCurrStr - 0) / 1), 39, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTempStatus - 0) / 1), 47, 2);
};

void McuTemperature2::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<McuTemperature2> McuTemperature2::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return McuTemperature2(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void McuTemperature2::UpdateGlobalState()
{
        cpT_McuTemperature2_gstate->McuTempCurrCool = this->McuTempCurrCool;
    cpT_McuTemperature2_gstate->McuTempCurrStr1 = this->McuTempCurrStr1;
    cpT_McuTemperature2_gstate->McuTempCurrStr2 = this->McuTempCurrStr2;
    cpT_McuTemperature2_gstate->McuTempCurrStr = this->McuTempCurrStr;
    cpT_McuTemperature2_gstate->McuTempRiseCurrStr = this->McuTempRiseCurrStr;
    cpT_McuTemperature2_gstate->McuTempStatus = this->McuTempStatus;

}

void McuTemperature2::ReadGlobalState()
{
        this->McuTempCurrCool = cpT_McuTemperature2_gstate->McuTempCurrCool;
    this->McuTempCurrStr1 = cpT_McuTemperature2_gstate->McuTempCurrStr1;
    this->McuTempCurrStr2 = cpT_McuTemperature2_gstate->McuTempCurrStr2;
    this->McuTempCurrStr = cpT_McuTemperature2_gstate->McuTempCurrStr;
    this->McuTempRiseCurrStr = cpT_McuTemperature2_gstate->McuTempRiseCurrStr;
    this->McuTempStatus = cpT_McuTemperature2_gstate->McuTempStatus;

}
    
c_McuTemperature2 McuTemperature2::toc_McuTemperature2(){
    c_McuTemperature2 out;
    out.McuTempCurrCool = this->McuTempCurrCool;
    out.McuTempCurrStr1 = this->McuTempCurrStr1;
    out.McuTempCurrStr2 = this->McuTempCurrStr2;
    out.McuTempCurrStr = this->McuTempCurrStr;
    out.McuTempRiseCurrStr = this->McuTempRiseCurrStr;
    out.McuTempStatus = this->McuTempStatus;
    return out;
};


McuTemperature2::McuTemperature2(c_McuTemperature2* self):McuTempCurrCool(self->McuTempCurrCool), McuTempCurrStr1(self->McuTempCurrStr1), McuTempCurrStr2(self->McuTempCurrStr2), McuTempCurrStr(self->McuTempCurrStr), McuTempRiseCurrStr(self->McuTempRiseCurrStr), McuTempStatus(self->McuTempStatus){};
McuTemperature2::McuTemperature2(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->McuTempCurrCool = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 7, 8) * 1 + -50);
    this->McuTempCurrStr1 = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 15, 8) * 1 + -50);
    this->McuTempCurrStr2 = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 23, 8) * 1 + -50);
    this->McuTempCurrStr = (int8_t)(UnpackSignalFromCANPacket(receivedPacket, 31, 8) * 1 + -50);
    this->McuTempRiseCurrStr = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 39, 8) * 1 + 0);
    this->McuTempStatus = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 47, 2) * 1 + 0);
};

void McuTemperature2::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool McuTemperature2::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void McuTemperature2::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = McuTemperature2(receivedPacket);
};
bool  McuTemperature2::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = McuTemperature2(receivedPacket);
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

void c_McuTemperature2_pack(c_McuTemperature2* self, dbc_can_tx_message_type* transmitPacket){
    McuTemperature2 _self(self);
    _self.pack(transmitPacket);
}

unpack_c_McuTemperature2_res c_McuTemperature2_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_McuTemperature2_res result;
	result.is_valid = false;
    std::optional<McuTemperature2> parsed_struct = McuTemperature2::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_McuTemperature2();
	}
    return result;
}

c_McuTemperature2 c_McuTemperature2_new(){
    c_McuTemperature2 out;
    out.pack = c_McuTemperature2_pack;
    out.unpack = c_McuTemperature2_unpack;
    return out;
}
