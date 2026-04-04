
#ifndef MCU_STATUS_H
#define MCU_STATUS_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif




enum MCU_bDmpCActv
{
    McuStatusMCU_bDmpCActvDampingOff = 1,
    McuStatusMCU_bDmpCActvDampingOn = 0,
};
enum MCU_stGateDrv
{
    McuStatusMCU_stGateDrvPWMrun = 2,
    McuStatusMCU_stGateDrvFreeWheel = 1,
    McuStatusMCU_stGateDrvASC = 0,
};

enum MCU_VCUWorkMode
{
    McuStatusMCU_VCUWorkModeMotorActiveHearting = 7,
    McuStatusMCU_VCUWorkModeAfterrun = 6,
    McuStatusMCU_VCUWorkModeActiveDischarge = 5,
    McuStatusMCU_VCUWorkModeShutdown = 4,
    McuStatusMCU_VCUWorkModeTorqueControl = 3,
    McuStatusMCU_VCUWorkModeReady = 2,
    McuStatusMCU_VCUWorkModeStandby = 1,
    McuStatusMCU_VCUWorkModeInitialization = 0,
};

typedef struct unpack_c_McuStatus_res unpack_c_McuStatus_res;
typedef struct c_McuStatus c_McuStatus;

typedef void (*pack_McuStatus_callback)(c_McuStatus*, dbc_can_tx_message_type*);
typedef unpack_c_McuStatus_res (*unpack_McuStatus_callback)(dbc_can_rx_message_type*);

typedef struct c_McuStatus{
    pack_McuStatus_callback pack;
    unpack_McuStatus_callback unpack;

    float McuOfsAl;
    float McuIsd;
    float McuIsq;
    uint8_t McubDmpCActv;
    uint8_t McustGateDrv;
    float McuDmpCTrqCurr;
    uint8_t McuVCUWorkMode;

} c_McuStatus;

typedef struct unpack_c_McuStatus_res{ 
    c_McuStatus val;
    bool is_valid;
} unpack_c_McuStatus_res;

void c_McuStatus_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_McuStatus_res c_McuStatus_unpack(dbc_can_rx_message_type* receivedPacket);

c_McuStatus c_McuStatus_new();

	extern c_McuStatus* cpT_McuStatus_gstate;

#ifdef __cplusplus
}
#endif
#endif
