
#ifndef VCU_TO_MCU_CURRENTCOMMAND_H
#define VCU_TO_MCU_CURRENTCOMMAND_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif



enum VCU_CurrentCommandEnable
{
    VcutoMCUCurrentCommandVCU_CurrentCommandEnableEnabled = 1,
    VcutoMCUCurrentCommandVCU_CurrentCommandEnableDisabled = 0,
};



typedef struct unpack_c_VcutoMCUCurrentCommand_res unpack_c_VcutoMCUCurrentCommand_res;
typedef struct c_VcutoMCUCurrentCommand c_VcutoMCUCurrentCommand;

typedef void (*pack_VcutoMCUCurrentCommand_callback)(c_VcutoMCUCurrentCommand*, dbc_can_tx_message_type*);
typedef unpack_c_VcutoMCUCurrentCommand_res (*unpack_VcutoMCUCurrentCommand_callback)(dbc_can_rx_message_type*);

typedef struct c_VcutoMCUCurrentCommand{
    pack_VcutoMCUCurrentCommand_callback pack;
    unpack_VcutoMCUCurrentCommand_callback unpack;

    float VcuIdCommand;
    float VcuIqCommand;
    uint8_t VcuCurrentCommandEnable;
    uint8_t Messagecounter300;
    uint8_t Checksum300;

} c_VcutoMCUCurrentCommand;

typedef struct unpack_c_VcutoMCUCurrentCommand_res{ 
    c_VcutoMCUCurrentCommand val;
    bool is_valid;
} unpack_c_VcutoMCUCurrentCommand_res;

void c_VcutoMCUCurrentCommand_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_VcutoMCUCurrentCommand_res c_VcutoMCUCurrentCommand_unpack(dbc_can_rx_message_type* receivedPacket);

c_VcutoMCUCurrentCommand c_VcutoMCUCurrentCommand_new();

	extern c_VcutoMCUCurrentCommand* cpT_VcutoMCUCurrentCommand_gstate;

#ifdef __cplusplus
}
#endif
#endif
