
#ifndef VCU_MCU02_H
#define VCU_MCU02_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

enum VCU_MinTorqueLimit
{
    VcuMCU02VCU_MinTorqueLimitError = 2047,
    VcuMCU02VCU_MinTorqueLimitInit = 2046,
};
enum VCU_MaxTorqueGradient
{
    VcuMCU02VCU_MaxTorqueGradientError = 16383,
    VcuMCU02VCU_MaxTorqueGradientInit = 16382,
};
enum VCU_MaxTorqueLimit
{
    VcuMCU02VCU_MaxTorqueLimitError = 2047,
    VcuMCU02VCU_MaxTorqueLimitInit = 2046,
};
enum VCU_TrqThresholdDampgCtl
{
    VcuMCU02VCU_TrqThresholdDampgCtlError = 255,
    VcuMCU02VCU_TrqThresholdDampgCtlInit = 254,
};



typedef struct unpack_c_VcuMCU02_res unpack_c_VcuMCU02_res;
typedef struct c_VcuMCU02 c_VcuMCU02;

typedef void (*pack_VcuMCU02_callback)(c_VcuMCU02*, dbc_can_tx_message_type*);
typedef unpack_c_VcuMCU02_res (*unpack_VcuMCU02_callback)(dbc_can_rx_message_type*);

typedef struct c_VcuMCU02{
    pack_VcuMCU02_callback pack;
    unpack_VcuMCU02_callback unpack;

    float VcuMinTorqueLimit;
    float VcuMaxTorqueGradient;
    float VcuMaxTorqueLimit;
    float VcuTrqThresholdDampgCtl;
    uint8_t Messagecounter047;
    uint8_t Checksum047;

} c_VcuMCU02;

typedef struct unpack_c_VcuMCU02_res{ 
    c_VcuMCU02 val;
    bool is_valid;
} unpack_c_VcuMCU02_res;

void c_VcuMCU02_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_VcuMCU02_res c_VcuMCU02_unpack(dbc_can_rx_message_type* receivedPacket);

c_VcuMCU02 c_VcuMCU02_new();

	extern c_VcuMCU02* cpT_VcuMCU02_gstate;

#ifdef __cplusplus
}
#endif
#endif
