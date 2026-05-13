//Stand_Marathon/src/MarathonLogic.cpp
#include "MarathonLogic.h"
#include <iostream>
#include <cstring>
#include <iomanip>

namespace {

constexpr bool kLogDbcSelectedSignals = false;

}

void printCANMessage(const CANMessage& msg, std::ostream& os, bool showDec) {
    os << "[CAN RX] ID=0x" << std::uppercase << std::hex << msg.id
       << " (" << std::dec << msg.id << ")"
       << " DLC=" << (int)msg.length
       << " Timestamp=" << msg.timestamp
       << "\n   Data: ";

    for (int i = 0; i < msg.length; ++i) {
        os << "0x" << std::uppercase << std::hex
           << std::setw(2) << std::setfill('0') << (int)msg.data[i];
        if (showDec) {
            os << "(" << std::dec << (int)msg.data[i] << ")";
        }
        os << " ";
    }
    os << std::dec << "\n";
}

uint32_t UnpackSignalFromBytes(const uint8_t* data, uint8_t startBit, uint8_t length)
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

void MarathonLogic::updateFromCAN(const CANMessage& msg, DataModel& data) {
    if (std::getenv("WS_LOG_CAN_RX")) {
    std::cout << "I`m here\n";
        printCANMessage(msg, std::cout, 0);
    }

    // DBC RX auto parsing is intentionally disabled here.
    // The runtime receive path must use the static switch below.

    switch (msg.id) {
        case 0x7a: { // MCU_VCU_1 (BO_ 122)
            float actualTorque = static_cast<int32_t>(UnpackSignalFromBytes(msg.data, 7, 11)) - 1024;
            float udcCurr = UnpackSignalFromBytes(msg.data, 12, 10); // No offset
            float isCurr = static_cast<int32_t>(UnpackSignalFromBytes(msg.data, 18, 11)) - 1024;
            int16_t actualSpeed = static_cast<int32_t>(UnpackSignalFromBytes(msg.data, 39, 16)) - 32768;

            data.Ms = actualTorque;
            data.Udc = udcCurr;
            data.Idc = isCurr;
            data.ns = actualSpeed;

            std::cout << "[RX] Ms=" << data.Ms << " Ns=" << data.ns
                      << " Udc=" << data.Udc << " Idc=" << data.Idc << std::endl;
            break;
        }

        case 0x7b: { // MCU_Temperature1 (BO_ 123)
            data.MCU_IGBTTempU   = static_cast<int8_t>(UnpackSignalFromBytes(msg.data, 7, 8)) - 50;
            data.MCU_IGBTTempV   = static_cast<int8_t>(UnpackSignalFromBytes(msg.data, 15, 8)) - 50;
            data.MCU_IGBTTempW   = static_cast<int8_t>(UnpackSignalFromBytes(msg.data, 23, 8)) - 50;
            data.MCU_IGBTTempMax = static_cast<int8_t>(UnpackSignalFromBytes(msg.data, 31, 8)) - 50;

            std::cout << "[RX] IGBT Temps: U=" << (int)data.MCU_IGBTTempU
                      << " V=" << (int)data.MCU_IGBTTempV
                      << " W=" << (int)data.MCU_IGBTTempW << std::endl;
            break;
        }

        case 0x7c: { // MCU_Temperature2 (BO_ 124)
            data.MCU_TempCurrCool = static_cast<int8_t>(UnpackSignalFromBytes(msg.data, 7, 8)) - 50;
            data.MCU_TempCurrStr  = static_cast<uint8_t>(UnpackSignalFromBytes(msg.data, 31, 8)) - 50;

            std::cout << "[RX] Stator Temp=" << (int)data.MCU_TempCurrStr
                      << " Coolant=" << (int)data.MCU_TempCurrCool << std::endl;
            break;
        }

        case 0x2c5: { // MCU_DeratingStatus (BO_ 709)
            data.M_max = static_cast<int32_t>(UnpackSignalFromBytes(msg.data, 11, 11)) - 1024;
            data.M_min = static_cast<int32_t>(UnpackSignalFromBytes(msg.data, 16, 11)) - 1024;
            // M_grad_max не существует в DBC, пропускаем

            std::cout << "[RX] Limits: M_max=" << data.M_max
                      << " M_min=" << data.M_min << std::endl;
            break;
        }

        case 0x2c6: { // MCU_FailureCode (BO_ 710)
            uint8_t failCode = UnpackSignalFromBytes(msg.data, 7, 3);
            std::cout << "[RX] Error Level (MCU_FailCode1): " << (int)failCode << std::endl;
            break;
        }

        case 0x49a: { // MCU_SoftwareNumber (BO_ 1178)
            uint16_t codeVer = UnpackSignalFromBytes(msg.data, 7, 16);
            data.MCU_SW_ver = "v" + std::to_string(codeVer);
            std::cout << "[RX] MCU_SW_ver=" << data.MCU_SW_ver << std::endl;
            break;
        }

        case 0x7f: { // MCU_Theta (BO_ 127)
            const uint16_t rawZVTimeStamp  =
                static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 7, 16));
        
            const uint16_t rawZVTheta =
                static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 23, 16));
        
            const uint16_t rawZVThetaCorr =
                static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 39, 16));
        
            data.ZVTimeStamp = rawZVTimeStamp;
            data.ZVTheta     = static_cast<float>(rawZVTheta) * 0.0001f;
            data.ZVThetaCorr = static_cast<float>(rawZVThetaCorr) * 0.0001f;
        
            std::cout << "[RX] Theta: "
                      << "TimeStamp=" << data.ZVTimeStamp
                      << " ZVTheta=" << data.ZVTheta
                      << " ZVThetaCorr=" << data.ZVThetaCorr
                      << std::endl;
            break;
        }
        
        case 0x80: { // MCU_FluxParams (BO_ 128)
            const uint16_t rawZVTemperature =
                static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 7, 9));
        
            const uint16_t rawZVFlux =
                static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 14, 16));
        
            const uint16_t rawZVRs =
                static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 30, 16));
        
            data.ZVTemperature = static_cast<float>(rawZVTemperature);
            data.ZVFlux        = static_cast<float>(rawZVFlux) * 0.0001f;
            data.ZVRs          = static_cast<float>(rawZVRs) * 0.0001f;
        
            std::cout << "[RX] FluxParams: "
                      << "ZVTemperature=" << data.ZVTemperature
                      << " ZVFlux=" << data.ZVFlux
                      << " ZVRs=" << data.ZVRs
                      << std::endl;
            break;
        }

        case 0x7e: { // MCU_CurrentVoltage (BO_ 126)
            data.Ud = static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 7, 16)) * 0.1f - 3276.0f;
            data.Uq = static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 23, 16)) * 0.1f - 3276.0f;
            data.Id = static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 39, 16)) * 0.1f - 3276.0f;
            data.Iq = static_cast<uint16_t>(UnpackSignalFromBytes(msg.data, 55, 16)) * 0.1f - 3276.0f;

            std::cout << "[RX] CurrentVoltage: Ud=" << data.Ud
                      << " Uq=" << data.Uq
                      << " Id=" << data.Id
                      << " Iq=" << data.Iq << std::endl;
            break;
        }

        case 0x7d: { // MCU_Status (BO_ 125)
            // SG_ MCU_OfsAl       : 7|12@0+  (0.0878906, 0)
            data.MCU_OfsAl = static_cast<float>(UnpackSignalFromBytes(msg.data, 7, 12)) * 0.0878906f;

            // SG_ MCU_Isd         : 11|12@0+ (0.5, -1023.5)
            {
                uint32_t raw = UnpackSignalFromBytes(msg.data, 11, 12);
                data.MCU_Isd = static_cast<float>(raw) * 0.5f - 1023.5f;
            }

            // SG_ MCU_Isq         : 31|12@0+ (0.5, -1023.5)
            {
                uint32_t raw = UnpackSignalFromBytes(msg.data, 31, 12);
                data.MCU_Isq = static_cast<float>(raw) * 0.5f - 1023.5f;
            }

            // SG_ MCU_bDmpCActv   : 35|1@0+  (1, 0)
            data.MCU_bDmpCActv = static_cast<uint8_t>(UnpackSignalFromBytes(msg.data, 35, 1));

            // SG_ MCU_stGateDrv   : 34|2@0+  (1, 0)
            data.MCU_stGateDrv = static_cast<uint8_t>(UnpackSignalFromBytes(msg.data, 34, 2));

            // SG_ MCU_DmpCTrqCurr : 55|8@0+  (0.2, -25)  "Nm"
            {
                uint32_t raw = UnpackSignalFromBytes(msg.data, 55, 8);
                data.MCU_DmpCTrqCurr = static_cast<float>(raw) * 0.2f - 25.0f;
            }

            // SG_ MCU_VCUWorkMode : 59|4@0+  (1, 0)
            data.MCU_VCUWorkMode = static_cast<uint8_t>(UnpackSignalFromBytes(msg.data, 59, 4));

            std::cout << "[RX] MCU_Status: "
                      << "OfsAl=" << data.MCU_OfsAl
                      << " Isd=" << data.MCU_Isd
                      << " Isq=" << data.MCU_Isq
                      << " bDmpCActv=" << (int)data.MCU_bDmpCActv
                      << " stGateDrv=" << (int)data.MCU_stGateDrv
                      << " DmpCTrqCurr=" << data.MCU_DmpCTrqCurr
                      << " WorkMode=" << (int)data.MCU_VCUWorkMode
                      << std::endl;
            break;
        }



        default:
            std::cout << "[RX] Unknown ID: 0x" << std::hex << msg.id << std::dec << std::endl;
            break;
    }
}
