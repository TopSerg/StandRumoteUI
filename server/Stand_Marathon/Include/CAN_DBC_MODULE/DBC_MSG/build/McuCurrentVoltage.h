
#ifndef MCU_CURRENTVOLTAGE_H
#define MCU_CURRENTVOLTAGE_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif






typedef struct unpack_c_McuCurrentVoltage_res unpack_c_McuCurrentVoltage_res;
typedef struct c_McuCurrentVoltage c_McuCurrentVoltage;

typedef void (*pack_McuCurrentVoltage_callback)(c_McuCurrentVoltage*, dbc_can_tx_message_type*);
typedef unpack_c_McuCurrentVoltage_res (*unpack_McuCurrentVoltage_callback)(dbc_can_rx_message_type*);

typedef struct c_McuCurrentVoltage{
    pack_McuCurrentVoltage_callback pack;
    unpack_McuCurrentVoltage_callback unpack;

    int16_t Ud;
    int16_t Uq;
    int16_t Id;
    int16_t Iq;

} c_McuCurrentVoltage;

typedef struct unpack_c_McuCurrentVoltage_res{ 
    c_McuCurrentVoltage val;
    bool is_valid;
} unpack_c_McuCurrentVoltage_res;

void c_McuCurrentVoltage_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_McuCurrentVoltage_res c_McuCurrentVoltage_unpack(dbc_can_rx_message_type* receivedPacket);

c_McuCurrentVoltage c_McuCurrentVoltage_new();

	extern c_McuCurrentVoltage* cpT_McuCurrentVoltage_gstate;

#ifdef __cplusplus
}
#endif
#endif
