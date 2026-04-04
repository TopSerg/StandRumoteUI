

#include <stdint.h>
#include "CANdbcDriver.h"
#include <optional>
#include "IMsg.hpp"
#include "DbcDispatcher.hpp"
#include "McuStatus.hpp"

extern "C" {
    #include "CANSendDispatcher.h"
    #include "McuStatus.h"
}

c_McuStatus c_McuStatus_gstate;
c_McuStatus* cpT_McuStatus_gstate = &c_McuStatus_gstate;

void McuStatus::saturateAdj() {
    this->McuOfsAl = (float)SaturateSignalFloat(this->McuOfsAl, 0, 359.736);
    this->McuIsd = (float)SaturateSignalFloat(this->McuIsd, -1023.5, 1023);
    this->McuIsq = (float)SaturateSignalFloat(this->McuIsq, -1023.5, 1023);
    this->McubDmpCActv = (uint8_t)SaturateSignalInteger(this->McubDmpCActv, 0, 1);
    this->McustGateDrv = (uint8_t)SaturateSignalInteger(this->McustGateDrv, 0, 3);
    this->McuDmpCTrqCurr = (float)SaturateSignalFloat(this->McuDmpCTrqCurr, -25, 25);
    this->McuVCUWorkMode = (uint8_t)SaturateSignalInteger(this->McuVCUWorkMode, 0, 15);
};
void McuStatus::rawPack(dbc_can_tx_message_type* transmitPacket) {

    transmitPacket->message_id = 125;

	ClearCANDataField(transmitPacket);

    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuOfsAl - 0) / 0.0878906), 7, 12);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIsd - -1023.5) / 0.5), 11, 12);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuIsq - -1023.5) / 0.5), 31, 12);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McubDmpCActv - 0) / 1), 35, 1);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McustGateDrv - 0) / 1), 34, 2);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuDmpCTrqCurr - -25) / 0.2), 55, 8);
    PackSignalToCANPacket(transmitPacket, (uint32_t)((this->McuVCUWorkMode - 0) / 1), 59, 4);
};

void McuStatus::pack(dbc_can_tx_message_type* transmitPacket){
	this->saturateAdj();
	this->rawPack(transmitPacket);
};
std::optional<McuStatus> McuStatus::try_unpack(dbc_can_rx_message_type* receivedPacket){ 
    if (true){  
        return McuStatus(receivedPacket); 
    } 
    else { 
        return {}; 
    }
};

void McuStatus::UpdateGlobalState()
{
        cpT_McuStatus_gstate->McuOfsAl = this->McuOfsAl;
    cpT_McuStatus_gstate->McuIsd = this->McuIsd;
    cpT_McuStatus_gstate->McuIsq = this->McuIsq;
    cpT_McuStatus_gstate->McubDmpCActv = this->McubDmpCActv;
    cpT_McuStatus_gstate->McustGateDrv = this->McustGateDrv;
    cpT_McuStatus_gstate->McuDmpCTrqCurr = this->McuDmpCTrqCurr;
    cpT_McuStatus_gstate->McuVCUWorkMode = this->McuVCUWorkMode;

}

void McuStatus::ReadGlobalState()
{
        this->McuOfsAl = cpT_McuStatus_gstate->McuOfsAl;
    this->McuIsd = cpT_McuStatus_gstate->McuIsd;
    this->McuIsq = cpT_McuStatus_gstate->McuIsq;
    this->McubDmpCActv = cpT_McuStatus_gstate->McubDmpCActv;
    this->McustGateDrv = cpT_McuStatus_gstate->McustGateDrv;
    this->McuDmpCTrqCurr = cpT_McuStatus_gstate->McuDmpCTrqCurr;
    this->McuVCUWorkMode = cpT_McuStatus_gstate->McuVCUWorkMode;

}
    
c_McuStatus McuStatus::toc_McuStatus(){
    c_McuStatus out;
    out.McuOfsAl = this->McuOfsAl;
    out.McuIsd = this->McuIsd;
    out.McuIsq = this->McuIsq;
    out.McubDmpCActv = this->McubDmpCActv;
    out.McustGateDrv = this->McustGateDrv;
    out.McuDmpCTrqCurr = this->McuDmpCTrqCurr;
    out.McuVCUWorkMode = this->McuVCUWorkMode;
    return out;
};


McuStatus::McuStatus(c_McuStatus* self):McuOfsAl(self->McuOfsAl), McuIsd(self->McuIsd), McuIsq(self->McuIsq), McubDmpCActv(self->McubDmpCActv), McustGateDrv(self->McustGateDrv), McuDmpCTrqCurr(self->McuDmpCTrqCurr), McuVCUWorkMode(self->McuVCUWorkMode){};
McuStatus::McuStatus(dbc_can_rx_message_type* receivedPacket, LocalErrorStats errStats){
    this->McuOfsAl = (float)(UnpackSignalFromCANPacket(receivedPacket, 7, 12) * 0.0878906 + 0);
    this->McuIsd = (float)(UnpackSignalFromCANPacket(receivedPacket, 11, 12) * 0.5 + -1023.5);
    this->McuIsq = (float)(UnpackSignalFromCANPacket(receivedPacket, 31, 12) * 0.5 + -1023.5);
    this->McubDmpCActv = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 35, 1) * 1 + 0);
    this->McustGateDrv = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 34, 2) * 1 + 0);
    this->McuDmpCTrqCurr = (float)(UnpackSignalFromCANPacket(receivedPacket, 55, 8) * 0.2 + -25);
    this->McuVCUWorkMode = (uint8_t)(UnpackSignalFromCANPacket(receivedPacket, 59, 4) * 1 + 0);
};

void McuStatus::msgPrepare(dbc_can_tx_message_type* transmitPacket) {
		InitPacket(transmitPacket);
    	this->pack(transmitPacket);
	}
	;

bool McuStatus::tryMsgPrepare(dbc_can_tx_message_type* transmitPacket)
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

void McuStatus::msgParse(dbc_can_rx_message_type* receivedPacket){
    *this = McuStatus(receivedPacket);
};
bool  McuStatus::tryMsgParse(dbc_can_rx_message_type* receivedPacket){
    --cycleTimeRemain;
    if (receivedPacket->message_id == ownCanId){
        cycleTimeRemain = cycleTime;
        if (true){
            *this = McuStatus(receivedPacket);
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

void c_McuStatus_pack(c_McuStatus* self, dbc_can_tx_message_type* transmitPacket){
    McuStatus _self(self);
    _self.pack(transmitPacket);
}

unpack_c_McuStatus_res c_McuStatus_unpack(dbc_can_rx_message_type* receivedPacket){
    unpack_c_McuStatus_res result;
	result.is_valid = false;
    std::optional<McuStatus> parsed_struct = McuStatus::try_unpack(receivedPacket);
    if (parsed_struct) {
		result.is_valid = true;
		result.val = parsed_struct.value().toc_McuStatus();
	}
    return result;
}

c_McuStatus c_McuStatus_new(){
    c_McuStatus out;
    out.pack = c_McuStatus_pack;
    out.unpack = c_McuStatus_unpack;
    return out;
}
