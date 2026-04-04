

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "McuDeratingStatus.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "McuDeratingStatus.h"
}

c_McuDeratingStatus c_McuDeratingStatus_gstate;
c_McuDeratingStatus* cpT_McuDeratingStatus_gstate = &c_McuDeratingStatus_gstate;

void McuDeratingStatus::saturateAdj() {
    this->McubDernOvrIdc = (uint8_t)SaturateSignalInteger(this->McubDernOvrIdc, 0, 1);
    this->McubDernOvrUdc = (uint8_t)SaturateSignalInteger(this->McubDernOvrUdc, 0, 1);
    this->McubDernStrTemp = (uint8_t)SaturateSignalInteger(this->McubDernStrTemp, 0, 1);
    this->McubDernTrqMax = (uint8_t)SaturateSignalInteger(this->McubDernTrqMax, 0, 1);
    this->McubDernTrqMin = (uint8_t)SaturateSignalInteger(this->McubDernTrqMin, 0, 1);
    this->McubDernUndrIdc = (uint8_t)SaturateSignalInteger(this->McubDernUndrIdc, 0, 1);
    this->McubDernUndrUdc = (uint8_t)SaturateSignalInteger(this->McubDernUndrUdc, 0, 1);
    this->McubReadyHv = (uint8_t)SaturateSignalInteger(this->McubReadyHv, 0, 1);
    this->McubDernTempIGBT = (uint8_t)SaturateSignalInteger(this->McubDernTempIGBT, 0, 1);
    this->McubDernTempCool = (uint8_t)SaturateSignalInteger(this->McubDernTempCool, 0, 1);
    this->McubDernN = (uint8_t)SaturateSignalInteger(this->McubDernN, 0, 1);
    this->McuTrqAbsMax = (int16_t)SaturateSignalInteger(this->McuTrqAbsMax, -1024, 1023);
    this->McuTrqAbsMin = (int16_t)SaturateSignalInteger(this->McuTrqAbsMin, -1024, 1023);
    this->McustDischg = (uint8_t)SaturateSignalInteger(this->McustDischg, 0, 3);
    this->McuUT15Curr = (float)SaturateSignalFloat(this->McuUT15Curr, 0, 30.5);
};
void McuDeratingStatus::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 709;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernOvrIdc - 0) / 1), 6, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernOvrUdc - 0) / 1), 5, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernStrTemp - 0) / 1), 4, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernTrqMax - 0) / 1), 3, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernTrqMin - 0) / 1), 2, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernUndrIdc - 0) / 1), 1, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernUndrUdc - 0) / 1), 0, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubReadyHv - 0) / 1), 15, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernTempIGBT - 0) / 1), 14, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernTempCool - 0) / 1), 13, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDernN - 0) / 1), 12, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTrqAbsMax - -1024) / 1), 11, 11);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuTrqAbsMin - -1024) / 1), 16, 11);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McustDischg - 0) / 1), 55, 2);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuUT15Curr - 0) / 0.5), 63, 6);
};

void McuDeratingStatus::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<McuDeratingStatus> McuDeratingStatus::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return McuDeratingStatus(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void McuDeratingStatus::UpdateGlobalState()
{
        cpT_McuDeratingStatus_gstate->McubDernOvrIdc = this->McubDernOvrIdc;
    cpT_McuDeratingStatus_gstate->McubDernOvrUdc = this->McubDernOvrUdc;
    cpT_McuDeratingStatus_gstate->McubDernStrTemp = this->McubDernStrTemp;
    cpT_McuDeratingStatus_gstate->McubDernTrqMax = this->McubDernTrqMax;
    cpT_McuDeratingStatus_gstate->McubDernTrqMin = this->McubDernTrqMin;
    cpT_McuDeratingStatus_gstate->McubDernUndrIdc = this->McubDernUndrIdc;
    cpT_McuDeratingStatus_gstate->McubDernUndrUdc = this->McubDernUndrUdc;
    cpT_McuDeratingStatus_gstate->McubReadyHv = this->McubReadyHv;
    cpT_McuDeratingStatus_gstate->McubDernTempIGBT = this->McubDernTempIGBT;
    cpT_McuDeratingStatus_gstate->McubDernTempCool = this->McubDernTempCool;
    cpT_McuDeratingStatus_gstate->McubDernN = this->McubDernN;
    cpT_McuDeratingStatus_gstate->McuTrqAbsMax = this->McuTrqAbsMax;
    cpT_McuDeratingStatus_gstate->McuTrqAbsMin = this->McuTrqAbsMin;
    cpT_McuDeratingStatus_gstate->McustDischg = this->McustDischg;
    cpT_McuDeratingStatus_gstate->McuUT15Curr = this->McuUT15Curr;

}

void McuDeratingStatus::ReadGlobalState()
{
        this->McubDernOvrIdc = cpT_McuDeratingStatus_gstate->McubDernOvrIdc;
    this->McubDernOvrUdc = cpT_McuDeratingStatus_gstate->McubDernOvrUdc;
    this->McubDernStrTemp = cpT_McuDeratingStatus_gstate->McubDernStrTemp;
    this->McubDernTrqMax = cpT_McuDeratingStatus_gstate->McubDernTrqMax;
    this->McubDernTrqMin = cpT_McuDeratingStatus_gstate->McubDernTrqMin;
    this->McubDernUndrIdc = cpT_McuDeratingStatus_gstate->McubDernUndrIdc;
    this->McubDernUndrUdc = cpT_McuDeratingStatus_gstate->McubDernUndrUdc;
    this->McubReadyHv = cpT_McuDeratingStatus_gstate->McubReadyHv;
    this->McubDernTempIGBT = cpT_McuDeratingStatus_gstate->McubDernTempIGBT;
    this->McubDernTempCool = cpT_McuDeratingStatus_gstate->McubDernTempCool;
    this->McubDernN = cpT_McuDeratingStatus_gstate->McubDernN;
    this->McuTrqAbsMax = cpT_McuDeratingStatus_gstate->McuTrqAbsMax;
    this->McuTrqAbsMin = cpT_McuDeratingStatus_gstate->McuTrqAbsMin;
    this->McustDischg = cpT_McuDeratingStatus_gstate->McustDischg;
    this->McuUT15Curr = cpT_McuDeratingStatus_gstate->McuUT15Curr;

}
    
c_McuDeratingStatus McuDeratingStatus::toc_McuDeratingStatus(){
    c_McuDeratingStatus out;
    out.McubDernOvrIdc = this->McubDernOvrIdc;
    out.McubDernOvrUdc = this->McubDernOvrUdc;
    out.McubDernStrTemp = this->McubDernStrTemp;
    out.McubDernTrqMax = this->McubDernTrqMax;
    out.McubDernTrqMin = this->McubDernTrqMin;
    out.McubDernUndrIdc = this->McubDernUndrIdc;
    out.McubDernUndrUdc = this->McubDernUndrUdc;
    out.McubReadyHv = this->McubReadyHv;
    out.McubDernTempIGBT = this->McubDernTempIGBT;
    out.McubDernTempCool = this->McubDernTempCool;
    out.McubDernN = this->McubDernN;
    out.McuTrqAbsMax = this->McuTrqAbsMax;
    out.McuTrqAbsMin = this->McuTrqAbsMin;
    out.McustDischg = this->McustDischg;
    out.McuUT15Curr = this->McuUT15Curr;
    return out;
};


McuDeratingStatus::McuDeratingStatus(c_McuDeratingStatus* self):McubDernOvrIdc(self->McubDernOvrIdc), McubDernOvrUdc(self->McubDernOvrUdc), McubDernStrTemp(self->McubDernStrTemp), McubDernTrqMax(self->McubDernTrqMax), McubDernTrqMin(self->McubDernTrqMin), McubDernUndrIdc(self->McubDernUndrIdc), McubDernUndrUdc(self->McubDernUndrUdc), McubReadyHv(self->McubReadyHv), McubDernTempIGBT(self->McubDernTempIGBT), McubDernTempCool(self->McubDernTempCool), McubDernN(self->McubDernN), McuTrqAbsMax(self->McuTrqAbsMax), McuTrqAbsMin(self->McuTrqAbsMin), McustDischg(self->McustDischg), McuUT15Curr(self->McuUT15Curr){};
McuDeratingStatus::McuDeratingStatus(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->McubDernOvrIdc = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 6, 1) * 1 + 0);
    this->McubDernOvrUdc = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 5, 1) * 1 + 0);
    this->McubDernStrTemp = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 4, 1) * 1 + 0);
    this->McubDernTrqMax = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 3, 1) * 1 + 0);
    this->McubDernTrqMin = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 2, 1) * 1 + 0);
    this->McubDernUndrIdc = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 1, 1) * 1 + 0);
    this->McubDernUndrUdc = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 0, 1) * 1 + 0);
    this->McubReadyHv = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 15, 1) * 1 + 0);
    this->McubDernTempIGBT = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 14, 1) * 1 + 0);
    this->McubDernTempCool = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 13, 1) * 1 + 0);
    this->McubDernN = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 12, 1) * 1 + 0);
    this->McuTrqAbsMax = (int16_t)(UnpackSignalFromCANPacket(receivedPacket, 11, 11) * 1 + -1024);
    this->McuTrqAbsMin = (int16_t)(UnpackSignalFromCANPacket(receivedPacket, 16, 11) * 1 + -1024);
    this->McustDischg = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 55, 2) * 1 + 0);
    this->McuUT15Curr = (float)(UnpackSignalFromCANPacket(receivedPacket, 63, 6) * 0.5 + 0);
};

void McuDeratingStatus::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool McuDeratingStatus::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void McuDeratingStatus::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = McuDeratingStatus(receivedPacket);
};
bool  McuDeratingStatus::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = McuDeratingStatus(receivedPacket);
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

void c_McuDeratingStatus_pack(c_McuDeratingStatus* self, dbc_can_tx_message_type* transmitPacket){
    McuDeratingStatus _self(self);
    _self.pack(transmitPacket);
}

unpack_c_McuDeratingStatus_res c_McuDeratingStatus_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_McuDeratingStatus_res result;
	result.is_valid = false;
    std::optional<McuDeratingStatus> parsed_struct = McuDeratingStatus::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_McuDeratingStatus();
	}
    return result;
}

c_McuDeratingStatus c_McuDeratingStatus_new(){
    c_McuDeratingStatus out;
    out.pack = c_McuDeratingStatus_pack;
    out.unpack = c_McuDeratingStatus_unpack;
    return out;
}
