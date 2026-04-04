
#ifndef MCU_VCU_1_H
#define MCU_VCU_1_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif





enum MCU_ActualSpeedValid
{
    McuVCU1MCU_ActualSpeedValidvalid = 1,
    McuVCU1MCU_ActualSpeedValidinvalid = 0,
};
enum MCU_ActTrqValid
{
    McuVCU1MCU_ActTrqValidvalid = 1,
    McuVCU1MCU_ActTrqValidinvalid = 0,
};


typedef struct unpack_c_McuVCU1_res unpack_c_McuVCU1_res;
typedef struct c_McuVCU1 c_McuVCU1;

typedef void (*pack_McuVCU1_callback)(c_McuVCU1*, dbc_can_tx_message_type*);
typedef unpack_c_McuVCU1_res (*unpack_McuVCU1_callback)(dbc_can_rx_message_type*);

typedef struct c_McuVCU1{
    pack_McuVCU1_callback pack;
    unpack_McuVCU1_callback unpack;

    int16_t McuActualTorque;
    uint16_t McuUdcCurr;
    int16_t McuIsCurr;
    int16_t McuActualSpeed;
    uint8_t McuActualSpeedValid;
    uint8_t McuActTrqValid;
    uint8_t Messagecounter7A;

} c_McuVCU1;

typedef struct unpack_c_McuVCU1_res{ 
    c_McuVCU1 val;
    bool is_valid;
} unpack_c_McuVCU1_res;

void c_McuVCU1_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_McuVCU1_res c_McuVCU1_unpack(dbc_can_rx_message_type* receivedPacket);

c_McuVCU1 c_McuVCU1_new();

	extern c_McuVCU1* cpT_McuVCU1_gstate;

#ifdef __cplusplus
}
#endif
#endif
