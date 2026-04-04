#include <stdint.h>
#include "CANDBCUtils.h"

float SaturateSignalFloat(float signal, float minValue, float maxValue)
{
    if (signal < minValue)
        signal = minValue;
    if (signal > maxValue)
        signal = maxValue;

    return signal;
}

uint8_t SaturateSignalByte(uint8_t signal, uint8_t minValue, uint8_t maxValue)
{
    if (signal < minValue)
        signal = minValue;
    if (signal > maxValue)
        signal = maxValue;
    return signal;
}

uint32_t SaturateSignalInteger(int signal, int minValue, int maxValue)
{
    if (signal < minValue)
        signal = minValue;
    if (signal > maxValue)
        signal = maxValue;
    return signal;
}

uint8_t IsSignalInRangeFloat(float signal, float minValue, float maxValue)
{
    if (signal >= minValue && signal <= maxValue)
        return 1;

    return 0;
}

uint8_t IsSignalInRangeByte(uint8_t signal, uint8_t minValue, uint8_t maxValue)
{
    if (signal >= minValue && signal <= maxValue)
        return 1;

    return 0;
}

uint32_t IsSignalInRangeInteger(uint32_t signal, uint32_t minValue, uint32_t maxValue)
{
    if (signal >= minValue && signal <= maxValue)
        return 1;

    return 0;
}

/*uint8_t IsCRCValid(dbc_can_rx_message_type* packet)
{
    crc_data_reset();
    CRC->dt = (packet->message_id << 24)
              | (packet->data[0] << 16)
              | (packet->data[1] << 8)
              | (packet->data[2]);
    CRC->dt = (packet->data[3] << 24)
              | (packet->data[4] << 16)
              | (packet->data[5] << 8)
              | (packet->data[6]);
    if (packet->data[7] == (uint8_t)(CRC->dt))
        return 1;

    return 0;
}

// Стандартный CRC-8 (полином x^8 + x^2 + x + 1 = 0x07)
static uint8_t crc8_calc(uint8_t* data, uint8_t length)
{
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}


uint8_t IsCRCValid(dbc_can_rx_message_type* packet)
{
    uint8_t buffer[11];

    // Формируем буфер: ID (3 байта) + data[0..6]
    buffer[0] = (packet->message_id >> 16) & 0xFF;
    buffer[1] = (packet->message_id >> 8) & 0xFF;
    buffer[2] = packet->message_id & 0xFF;

    for (int i = 0; i < 7; i++) {
        buffer[3 + i] = packet->data[i];
    }

    uint8_t crc = crc8_calc(buffer, 10);  // 3 байта ID + 7 байт данных
    return (crc == packet->data[7]) ? 1 : 0;
}*/

uint8_t RolingCounterDeltaValue(uint8_t lastValue, uint8_t newValue)
{
    if (lastValue == MESSAGECOUNTER_NOT_INIT)
        return 0;
    int delta = newValue - lastValue;
    // TODO need better know of counter (+16 or +15)?
    if (delta <= 0)
        delta += 16;
    return delta;
}

void PackSignalToBytes(uint8_t* data, uint32_t value, uint8_t startBit, uint8_t length)
{
    uint8_t currentStartBit;
    uint8_t startByteInValue;
    uint8_t startByteInData;
    uint8_t shift;
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
        data[startByteInData + startByteInValue  - i] |= (uint8_t)(value >> i * 8);
    }
}

uint32_t UnpackSignalFromBytes(uint8_t* data, uint8_t startBit, uint8_t length)
{
    uint32_t result = 0;

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
    startByteInValue = (length - 1) / 8 + lengthInBytesIncreased;
    for (int i = startByteInValue; i >= 0; i--)
    {
        result <<= 8;
        result |= data[startByteInData + startByteInValue - i];
    }
    result >>= shift;
    result &= 0xffffffff >> (32 - length);
    return result;
}

void ClearCANDataField(dbc_can_tx_message_type* transmitPacket)
{
    int i;
    for (i = 0; i < 8; i++)
        transmitPacket->data[i] = 0;
}

void PackSignalToCANPacket(dbc_can_tx_message_type* transmitPacket, uint32_t signalValue, uint8_t startBit, uint8_t length)
{
    PackSignalToBytes(transmitPacket->data, signalValue, startBit, length);
}

uint32_t UnpackSignalFromCANPacket(dbc_can_rx_message_type* receivedPacket, uint8_t startBit, uint8_t length)
{
    return UnpackSignalFromBytes(receivedPacket->data, startBit, length);
}