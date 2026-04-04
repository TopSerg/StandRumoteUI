
#ifndef MCU_TEMPERATURE2_H
#define MCU_TEMPERATURE2_H

#include <stdint.h>
#include "CANdbcDriver.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif






enum MCU_TempStatus
{
    McuTemperature2MCU_TempStatusOverHeating = 3,
    McuTemperature2MCU_TempStatusHeatingalarm2 = 2,
    McuTemperature2MCU_TempStatusHeatingalarm1 = 1,
    McuTemperature2MCU_TempStatusNormalTemperature = 0,
};

typedef struct unpack_c_McuTemperature2_res unpack_c_McuTemperature2_res;
typedef struct c_McuTemperature2 c_McuTemperature2;

typedef void (*pack_McuTemperature2_callback)(c_McuTemperature2*, dbc_can_tx_message_type*);
typedef unpack_c_McuTemperature2_res (*unpack_McuTemperature2_callback)(dbc_can_rx_message_type*);

typedef struct c_McuTemperature2{
    pack_McuTemperature2_callback pack;
    unpack_McuTemperature2_callback unpack;

    int8_t McuTempCurrCool;
    int8_t McuTempCurrStr1;
    int8_t McuTempCurrStr2;
    int8_t McuTempCurrStr;
    uint8_t McuTempRiseCurrStr;
    uint8_t McuTempStatus;

} c_McuTemperature2;

typedef struct unpack_c_McuTemperature2_res{ 
    c_McuTemperature2 val;
    bool is_valid;
} unpack_c_McuTemperature2_res;

void c_McuTemperature2_pack(void* self, dbc_can_tx_message_type* transmitPacket);
unpack_c_McuTemperature2_res c_McuTemperature2_unpack(dbc_can_rx_message_type* receivedPacket);

c_McuTemperature2 c_McuTemperature2_new();

	extern c_McuTemperature2* cpT_McuTemperature2_gstate;

#ifdef __cplusplus
}
#endif
#endif
