//Stand_Marathon/src/DataModel.h
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

struct DbcRuntimeSignalValue {
    uint32_t messageId = 0;
    std::string messageName;
    std::string signalName;
    uint32_t raw = 0;
    double physical = 0.0f;
};

struct DataModel {
    // === CAN настройки ===
    uint16_t canBoard = 0;
    uint16_t canChannel = 1;
    uint32_t canBaud = 1000000;
    uint32_t canFlags = 0;
    uint32_t amask = 0x00000000;
    uint32_t acode = 0x00000032;

    // === Управление ===
    bool Kl_15 = true;            // Зажигание
    bool En_rem = true;           // Удаленное управление
    bool En_Is = false;           // Current control is always disabled at startup
    uint8_t Dampf_U = 0;           // Демпф. U
    bool Brake_active = false;     // Тормоз
    bool TCS_active = false;       // TCS
    uint8_t MotorCtrl = 1;         // Режим управления
    uint8_t GearCtrl = 4;          // Коробка передач
    // === Добавить в секцию "Управление" ===
    float M_desired = 0.0f;           // Желаемый момент (Nm)
    uint8_t SurgeDamperState = 2;     // Состояние демпфера
    uint8_t MCU_RequestedState = 0;   // MCU requested state


    // === Ограничения ===
    float M_max = 450.0f;           // [Нм]
    float M_min = -450.0;          // [Нм]
    float M_grad_max = 4200.0f;    // [Нм/с]
    float dM_damp_Ctrl = 0.1f;     // Порог демпфирования
    float i_R = 9.78f;             // Передаточное число
    float n_max = 1000.0f;         // Макс. скорость [об/мин]
    float T_Str_max = 85.0f;       // Макс. температура статора

    // === Реальные значения ===
    float Ms = 0.0f;               // Момент
    float ns = 0.0f;               // Скорость
    float Idc = 0.0f;              // Ток DC
    float Isd = 0.0f;              // d-компонента
    float Isq = 0.0f;              // q-компонента
    float Udc = 0.0f;              // Напряжение DC
    float Pe_dc = 0.0f;            // Мощность DC

    // === Температуры ===
    float MCU_IGBTTempU = 0.0f;
    float MCU_IGBTTempV = 0.0f;
    float MCU_IGBTTempW = 0.0f;
    float MCU_IGBTTempMax = 0.0f;
    float MCU_TempCurrStr = 0.0f;
    float MCU_TempCurrStr1 = 0.0f;
    float MCU_TempCurrStr2 = 0.0f;
    float MCU_TempCurrCool = 0.0f;

    float TrqThresholdDampgCtl = 0.0f;

    // === Статусы ===
    uint8_t MCU_Status = 0;        // MCU-Status
    uint8_t M_Status = 0;          // М-статус
    uint8_t n_Status = 0;          // n-статус
    uint8_t MCU_stGateDrv = 0;     // Gate driver status
    uint8_t TempStatus = 0;        // Температурный статус

    // === Версии ПО ===
    std::string MCU_SW_ver;
    std::string MCU_calib_SW_ver;
    std::string MCU_Hv_SW_ver;
    std::string MCU_CPLD_SW_ver;
    std::string MCU_HW_ver;

    // === Прочее ===
    std::string configFilePath = "config/Config.ini";

    float Emf = 0.0f;
    float Welectrical = 0.0f;
    float motorRs = 0.0f;
    float Wmechanical = 0.0f;
    float ZVFlux = 0.0f;
    float ZVTheta = 0.0f;
    float ZVTemperature = 0.0f;
    float ZVRs = 0.0f;
    float ZVTimeStamp = 0.0f;
    float ZVThetaCorr = 0.0f;

    // === Resolver calibration telemetry (CAN 0x081, 50 Hz) ===
    float ResolverSine = 0.0f;
    float ResolverCosine = 0.0f;
    float ResolverAmplitude = 0.0f;
    float ResolverTheta = 0.0f;
    float ResolverThetaCorr = 0.0f;

    // === Resolver electrical-zero calibration (CAN 0x082/0x301) ===
    float ResolverFluxPositionError = 0.0f;
    float ResolverAppliedCorrection = 0.0f;
    float ResolverElectricalSpeed = 0.0f;
    uint8_t ResolverCalibrationStatus = 0;
    uint8_t ResolverCalibrationAckSequence = 0;
    bool ResolverSpeedValid = false;
    bool ResolverCorrectionActive = false;
    bool ResolverSignalsReady = false;
    bool ResolverCalibrationCommandEnabled = false;
    float ResolverCalibrationCommand = 0.0f;
    uint8_t ResolverCalibrationCommandSequence = 0;

    // === Current-command CAN diagnostics (CAN 0x2C6) ===
    uint8_t McuCANFault = 0;
    bool CurrentCommandTimeoutFault = false;
    bool CurrentCommandCounterFault = false;

    float Ud = 0;
    float Uq = 0;
    float Id = 0;
    float Iq = 0;

    // --- MCU_Status (BO_ 125) ---
    float   MCU_OfsAl        = 0.0f;  // [deg] scale 0.0878906
    float   MCU_Isd          = 0.0f;  // [A]   scale 0.5,  offset -1023.5
    float   MCU_Isq          = 0.0f;  // [A]   scale 0.5,  offset -1023.5
    uint8_t MCU_bDmpCActv    = 0;     // flag
    // MCU_stGateDrv уже есть в секции статусов
    float   MCU_DmpCTrqCurr  = 0.0f;  // [Nm]  scale 0.2,  offset -25
    uint8_t MCU_VCUWorkMode  = 0;     // [0..15]

    std::unordered_map<std::string, DbcRuntimeSignalValue> dbcSignals;
};
