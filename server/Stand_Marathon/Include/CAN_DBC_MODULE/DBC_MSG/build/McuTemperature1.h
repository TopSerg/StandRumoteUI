
#ifndef MCU_TEMPERATURE1_H
#define MCU_TEMPERATURE1_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif










typedef struct unpack_c_McuTemperature1_res unpack_c_McuTemperature1_res;
typedef struct c_McuTemperature1 c_McuTemperature1;

typedef void (*pack_McuTemperature1_callback)(c_McuTemperature1*, dbc_can_tx_message_type*);
typedef unpack_c_McuTemperature1_res (*unpack_McuTemperature1_callback)(dbc_can_rx_message_type*);

typedef struct c_McuTemperature1{
    pack_McuTemperature1_callback pack;
    unpack_McuTemperature1_callback unpack;

    int8_t McuIGBTTempU;
    int8_t McuIGBTTempV;
    int8_t McuIGBTTempW;
    int8_t McuIGBTTempMax;
    uint8_t McuIGBTTempRiseU;
    uint8_t McuIGBTTempRiseV;
    uint8_t McuIGBTTempRiseW;
    uint8_t McuIGBTTempRiseMax;

} c_McuTemperature1;

typedef struct unpack_c_McuTemperature1_res{ 
    c_McuTemperature1 val;
    bool is_valid;
} unpack_c_McuTemperature1_res;

void c_McuTemperature1_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_McuTemperature1_res c_McuTemperature1_unpack(dbc_can_rx_message_type* receivedPacket);

c_McuTemperature1 c_McuTemperature1_new();

	extern c_McuTemperature1* cpT_McuTemperature1_gstate;

#ifdef __cplusplus
}
#endif
#endif
