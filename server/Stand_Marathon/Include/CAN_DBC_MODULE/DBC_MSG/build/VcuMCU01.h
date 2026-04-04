
#ifndef VCU_MCU01_H
#define VCU_MCU01_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


enum VCU_BrakepedalStatus
{
    VcuMCU01VCU_BrakepedalStatusBrakepressed = 1,
    VcuMCU01VCU_BrakepedalStatusBrakeNotPressed = 0,
};
enum VCU_MCUSurgeDamperState
{
    VcuMCU01VCU_MCUSurgeDamperStateDstC_int_ext = 2,
    VcuMCU01VCU_MCUSurgeDamperStateDstC_ext = 1,
    VcuMCU01VCU_MCUSurgeDamperStateDstC_off = 0,
};
enum VCU_KL15On
{
    VcuMCU01VCU_KL15OnOn = 1,
    VcuMCU01VCU_KL15OnOff = 0,
};
enum VCU_TCSActive
{
    VcuMCU01VCU_TCSActiveActive = 1,
    VcuMCU01VCU_TCSActiveNotActive = 0,
};
enum VCU_ActualGear
{
    VcuMCU01VCU_ActualGearD = 4,
    VcuMCU01VCU_ActualGearR = 3,
    VcuMCU01VCU_ActualGearN = 2,
    VcuMCU01VCU_ActualGearP = 1,
    VcuMCU01VCU_ActualGearreserve = 0,
};
enum VCU_MCURequestedState
{
    VcuMCU01VCU_MCURequestedStateShutdown = 4,
    VcuMCU01VCU_MCURequestedStateDischarge = 3,
    VcuMCU01VCU_MCURequestedStateActiveHearting = 2,
    VcuMCU01VCU_MCURequestedStateTrqCtr = 1,
    VcuMCU01VCU_MCURequestedStateReady = 0,
};



typedef struct unpack_c_VcuMCU01_res unpack_c_VcuMCU01_res;
typedef struct c_VcuMCU01 c_VcuMCU01;

typedef void (*pack_VcuMCU01_callback)(c_VcuMCU01*, dbc_can_tx_message_type*);
typedef unpack_c_VcuMCU01_res (*unpack_VcuMCU01_callback)(dbc_can_rx_message_type*);

typedef struct c_VcuMCU01{
    pack_VcuMCU01_callback pack;
    unpack_VcuMCU01_callback unpack;

    float VcuMCUDesiredTorque;
    uint8_t VcuBrakepedalStatus;
    uint8_t VcuMCUSurgeDamperState;
    uint8_t VcuKL15On;
    uint8_t VcuTCSActive;
    uint8_t VcuActualGear;
    uint8_t VcuMCURequestedState;
    uint8_t Messagecounter046;
    uint8_t Checksum046;

} c_VcuMCU01;

typedef struct unpack_c_VcuMCU01_res{ 
    c_VcuMCU01 val;
    bool is_valid;
} unpack_c_VcuMCU01_res;

void c_VcuMCU01_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_VcuMCU01_res c_VcuMCU01_unpack(dbc_can_rx_message_type* receivedPacket);

c_VcuMCU01 c_VcuMCU01_new();

	extern c_VcuMCU01* cpT_VcuMCU01_gstate;

#ifdef __cplusplus
}
#endif
#endif
