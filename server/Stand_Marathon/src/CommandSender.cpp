//Stand_Marathon/src/CommandSender.cpp
#include "CommandSender.h"
#include <iostream>

#include <cstdio>
#include <cstdint>
#include <cstdlib>

// Эта функция будет использоваться для упаковки сигналов
void PackSignalToBytes(uint8_t* data, uint32_t value, uint8_t startBit, uint8_t length)
{
    uint8_t currentStartBit;
    uint8_t startByteInValue;
    uint8_t startByteInData;
    int8_t shift;
    uint8_t lengthInBytesIncreased = 0;
    uint8_t startBitInByte;

    startBitInByte = startBit % 8;
    currentStartBit = (length - 1) % 8;
    startByteInData = startBit / 8;
    shift = startBitInByte - currentStartBit;
    if (shift < 0)
    {
        shift += 8;
        lengthInBytesIncreased = 1;
    }
    value <<= shift;
    startByteInValue = (length - 1) / 8 + lengthInBytesIncreased;
    for (int i = startByteInValue; i >= 0; i--)
    {
        data[startByteInData + startByteInValue - i] |= (uint8_t)(value >> (i * 8));
    }
}


void printPayloadHex(const uint8_t payload[8]) {
    std::puts("Payload (HEX):");
    for (int i = 0; i < 8; ++i) {
        std::printf("  Byte %d: 0x%02X\n", i, payload[i]);
    }
}

void CommandSender::sendControlCommand(CANInterface& can, const DataModel& data) {
    uint8_t payload[8] = {0};

    // 1. KL15On (1 бит, старт с 8)
    PackSignalToBytes(payload, data.Kl_15 ? 1 : 0, 8, 1);

    // 2. MCUDesiredTorque (11 бит, -1023 смещение, 1.0 масштаб), старт с 7
    int32_t torque_raw = 0;
    if(data.MotorCtrl == 1){
        torque_raw = static_cast<int32_t>((data.M_desired + 1023.0f));
        PackSignalToBytes(payload, torque_raw, 7, 11);
    }
    else{
        int32_t speed = data.M_desired / 15.675;
        torque_raw = static_cast<int32_t>((speed + 1023.0f));
        PackSignalToBytes(payload, torque_raw, 7, 11);
    }

    // 3. SurgeDamperState (2 бита, старт с 10)
    PackSignalToBytes(payload, data.SurgeDamperState & 0x03, 10, 2);

    // 4. BrakePedalStatus (1 бит, старт с 11)
    PackSignalToBytes(payload, data.Brake_active ? 1 : 0, 11, 1);

    // 5. RequestedState (4 бита, старт с 47)
    PackSignalToBytes(payload, data.MotorCtrl & 0x0F, 47, 4);

    // 6. TCSActive (1 бит, старт с 23)
    PackSignalToBytes(payload, data.TCS_active ? 1 : 0, 23, 1);

    // 7. ActualGear (3 бита, старт с 22)
    PackSignalToBytes(payload, data.GearCtrl & 0x07, 22, 3);

    // 8. Counter
    static uint8_t counter = 0;
    PackSignalToBytes(payload, counter++ & 0x0F, 51, 4);

    // 9. Checksum (заглушка)
    PackSignalToBytes(payload, 0, 63, 8);

    can.send(0x046, payload, 8);

    if (std::getenv("WS_LOG_CAN")) {
        std::printf("[LOG] ControlCommand:\n");
        std::printf("[CAN TX] ID=0x046 DLC=8 "
       "DATA=%02X %02X %02X %02X %02X %02X %02X %02X | "
       "KL15=%d Ms=%.1f torque_raw=%d Surge=%u Brake=%u "
       "ReqState=%u TCS=%u Gear=%u Cnt=%u Crc=%u\n",
       (unsigned)payload[0], (unsigned)payload[1], (unsigned)payload[2], (unsigned)payload[3],
       (unsigned)payload[4], (unsigned)payload[5], (unsigned)payload[6], (unsigned)payload[7],
       data.Kl_15 ? 1 : 0,
       data.M_desired,
       torque_raw,
       (unsigned)(data.SurgeDamperState & 0x03),
       data.Brake_active ? 1 : 0,
       (unsigned)(data.MotorCtrl & 0x0F),
       data.TCS_active ? 1 : 0,
       (unsigned)(data.GearCtrl & 0x07),
       (unsigned)((uint8_t)((counter - 1) & 0x0F)),  // значение, которое реально упаковали
       0u                                           // checksum сейчас заглушка = 0
);
        for (int i = 0; i < 8; ++i)
            std::printf("payload[%d] = 0x%02X\n", i, payload[i]);
    }
}

void CommandSender::sendLimitCommand(CANInterface& can, const DataModel& data) {
    uint8_t payload[8] = {0};

    // 1. MinTorqueLimit (11 бит, смещение -1023, масштаб 1.0), старт с 7
    int32_t min_raw = static_cast<int32_t>((data.M_min + 1023.0f));
    PackSignalToBytes(payload, min_raw, 7, 11);

    // 2. MaxTorqueGradient (14 бит, масштаб 32.0), старт с 12
    int32_t grad_raw = static_cast<int32_t>(data.M_grad_max / 32.0f);
    PackSignalToBytes(payload, grad_raw, 12, 14);

    // 3. MaxTorqueLimit (11 бит, -1023, масштаб 1.0), старт с 30
    int32_t max_raw = static_cast<int32_t>((data.M_max + 1023.0f));
    PackSignalToBytes(payload, max_raw, 30, 11);

    // 4. TrqThresholdDampgCtl (8 бит, масштаб 0.1), старт с 35
    int32_t threshold_raw = static_cast<int32_t>(data.TrqThresholdDampgCtl / 0.1f);
    PackSignalToBytes(payload, threshold_raw, 35, 8);

    // 5. Counter
    static uint8_t counter = 0;
    PackSignalToBytes(payload, counter++ & 0x0F, 51, 4);

    // 6. Checksum (заглушка)
    PackSignalToBytes(payload, 0, 63, 8);

    // 7. Max Speed
    int32_t max_speed_raw = (int32_t)(data.n_max / 100.0f + 0.5f);
    if (max_speed_raw < 0) max_speed_raw = 0;
    if (max_speed_raw > 31) max_speed_raw = 31;
    PackSignalToBytes(payload, (uint32_t)max_speed_raw, 43, 5);


    can.send(0x047, payload, 8);

    if (std::getenv("WS_LOG_CAN")) {
        std::printf("[LOG] LimitCommand:\n");
        std::cout << data.M_min << "     " << data.M_max << std::endl;
        for (int i = 0; i < 8; ++i)
            std::printf("payload[%d] = 0x%02X\n", i, payload[i]);
    }
}


void CommandSender::sendTorqueCommand(CANInterface& can, DataModel& data) {
    uint8_t payload[8] = {0};

    // 1. VCU_IdCommand: 13 бит, 0.1 масштаб, -320 смещение, старт с 7 бита
    int32_t id_raw = static_cast<int32_t>((data.Isd + 3200.0f) * 10.0f);
    PackSignalToBytes(payload, id_raw, 7, 16);

    // 2. VCU_IqCommand: 13 бит, 0.1 масштаб, -320 смещение, старт с 23 бита
    int32_t iq_raw = static_cast<int32_t>((data.Isq + 3200.0f) * 10.0f);
    PackSignalToBytes(payload, iq_raw, 23, 16);

    std::cout << "Id: " << id_raw << "    Iq: " << iq_raw << std::endl;

    // 3. VCU_CurrentCommandEnable: 1 бит, старт с 39
    PackSignalToBytes(payload, (data.En_Is) ? 1 : 0, 39, 1);

    // 4. MessageCounter_300: 4 бита, старт с 51
    static uint8_t counter = 0;
    PackSignalToBytes(payload, counter++ & 0x0F, 51, 4);

    // 5. Checksum_300: 8 бит, старт с 63 (заглушка)
    PackSignalToBytes(payload, 0, 63, 8);

    can.send(0x300, payload, 8);

    if (std::getenv("WS_LOG_CAN")) {
        std::printf("[LOG] TorqueCommand: Isd=%.2f Iq=%.2f\n", data.Isd, data.Isq);
        for (int i = 0; i < 8; ++i)
            std::printf("payload[%d] = 0x%02X\n", i, payload[i]);
    }
}




