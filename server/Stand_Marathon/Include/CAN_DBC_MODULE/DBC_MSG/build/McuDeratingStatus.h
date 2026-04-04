
#ifndef MCU_DERATINGSTATUS_H
#define MCU_DERATINGSTATUS_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MCU_bDernOvrIdc
{
    McuDeratingStatusMCU_bDernOvrIdcDerating = 1,
    McuDeratingStatusMCU_bDernOvrIdcNormal = 0,
};
enum MCU_bDernOvrUdc
{
    McuDeratingStatusMCU_bDernOvrUdcDerating = 1,
    McuDeratingStatusMCU_bDernOvrUdcNormal = 0,
};
enum MCU_bDernStrTemp
{
    McuDeratingStatusMCU_bDernStrTempDerating = 1,
    McuDeratingStatusMCU_bDernStrTempNormal = 0,
};
enum MCU_bDernTrqMax
{
    McuDeratingStatusMCU_bDernTrqMaxDerating = 1,
    McuDeratingStatusMCU_bDernTrqMaxNormal = 0,
};
enum MCU_bDernTrqMin
{
    McuDeratingStatusMCU_bDernTrqMinDerating = 1,
    McuDeratingStatusMCU_bDernTrqMinNormal = 0,
};
enum MCU_bDernUndrIdc
{
    McuDeratingStatusMCU_bDernUndrIdcDerating = 1,
    McuDeratingStatusMCU_bDernUndrIdcNormal = 0,
};
enum MCU_bDernUndrUdc
{
    McuDeratingStatusMCU_bDernUndrUdcDerating = 1,
    McuDeratingStatusMCU_bDernUndrUdcNormal = 0,
};
enum MCU_bReadyHv
{
    McuDeratingStatusMCU_bReadyHvPrechargeFinished = 1,
    McuDeratingStatusMCU_bReadyHvPreChargeNotFinished = 0,
};
enum MCU_bDernTempIGBT
{
    McuDeratingStatusMCU_bDernTempIGBTDerating = 1,
    McuDeratingStatusMCU_bDernTempIGBTNormal = 0,
};
enum MCU_bDernTempCool
{
    McuDeratingStatusMCU_bDernTempCoolDerating = 1,
    McuDeratingStatusMCU_bDernTempCoolNormal = 0,
};
enum MCU_bDernN
{
    McuDeratingStatusMCU_bDernNDerating = 1,
    McuDeratingStatusMCU_bDernNNormal = 0,
};


enum MCU_stDischg
{
    McuDeratingStatusMCU_stDischgFailed = 3,
    McuDeratingStatusMCU_stDischgFinished = 2,
    McuDeratingStatusMCU_stDischgActive = 1,
    McuDeratingStatusMCU_stDischgStandby = 0,
};


typedef struct unpack_c_McuDeratingStatus_res unpack_c_McuDeratingStatus_res;
typedef struct c_McuDeratingStatus c_McuDeratingStatus;

typedef void (*pack_McuDeratingStatus_callback)(c_McuDeratingStatus*, dbc_can_tx_message_type*);
typedef unpack_c_McuDeratingStatus_res (*unpack_McuDeratingStatus_callback)(dbc_can_rx_message_type*);

typedef struct c_McuDeratingStatus{
    pack_McuDeratingStatus_callback pack;
    unpack_McuDeratingStatus_callback unpack;

    uint8_t McubDernOvrIdc;
    uint8_t McubDernOvrUdc;
    uint8_t McubDernStrTemp;
    uint8_t McubDernTrqMax;
    uint8_t McubDernTrqMin;
    uint8_t McubDernUndrIdc;
    uint8_t McubDernUndrUdc;
    uint8_t McubReadyHv;
    uint8_t McubDernTempIGBT;
    uint8_t McubDernTempCool;
    uint8_t McubDernN;
    int16_t McuTrqAbsMax;
    int16_t McuTrqAbsMin;
    uint8_t McustDischg;
    float McuUT15Curr;

} c_McuDeratingStatus;

typedef struct unpack_c_McuDeratingStatus_res{ 
    c_McuDeratingStatus val;
    bool is_valid;
} unpack_c_McuDeratingStatus_res;

void c_McuDeratingStatus_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_McuDeratingStatus_res c_McuDeratingStatus_unpack(dbc_can_rx_message_type* receivedPacket);

c_McuDeratingStatus c_McuDeratingStatus_new();

	extern c_McuDeratingStatus* cpT_McuDeratingStatus_gstate;

#ifdef __cplusplus
}
#endif
#endif
