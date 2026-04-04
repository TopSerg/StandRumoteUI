
#ifndef MCU_FAILURECODE_H
#define MCU_FAILURECODE_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MCU_FailCode1
{
    McuFailureCodeMCU_FailCode1FaultLevel4 = 4,
    McuFailureCodeMCU_FailCode1FaultLevel3 = 3,
    McuFailureCodeMCU_FailCode1FaultLevel2 = 2,
    McuFailureCodeMCU_FailCode1FaultLevel1 = 1,
    McuFailureCodeMCU_FailCode1NoFault = 0,
};





typedef struct unpack_c_McuFailureCode_res unpack_c_McuFailureCode_res;
typedef struct c_McuFailureCode c_McuFailureCode;

typedef void (*pack_McuFailureCode_callback)(c_McuFailureCode*, dbc_can_tx_message_type*);
typedef unpack_c_McuFailureCode_res (*unpack_McuFailureCode_callback)(dbc_can_rx_message_type*);

typedef struct c_McuFailureCode{
    pack_McuFailureCode_callback pack;
    unpack_McuFailureCode_callback unpack;

    uint8_t McuFailCode1;
    uint32_t McuSoftwaeFaultBit;
    uint16_t McuHardwareFault;
    uint8_t McuSensorFault;
    uint8_t McuCANFault;

} c_McuFailureCode;

typedef struct unpack_c_McuFailureCode_res{ 
    c_McuFailureCode val;
    bool is_valid;
} unpack_c_McuFailureCode_res;

void c_McuFailureCode_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_McuFailureCode_res c_McuFailureCode_unpack(dbc_can_rx_message_type* receivedPacket);

c_McuFailureCode c_McuFailureCode_new();

	extern c_McuFailureCode* cpT_McuFailureCode_gstate;

#ifdef __cplusplus
}
#endif
#endif
