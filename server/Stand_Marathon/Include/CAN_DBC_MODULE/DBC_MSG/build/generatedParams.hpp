#include "VcuMCU01.hpp"
#include "VcuMCU02.hpp"
#include "VcutoMCUCurrentCommand.hpp"
#include "McuVCU1.hpp"
#include "McuTemperature1.hpp"
#include "McuTemperature2.hpp"
#include "McuStatus.hpp"
#include "McuCurrentVoltage.hpp"
#include "McuDeratingStatus.hpp"
#include "McuFailureCode.hpp"

VcuMCU01 vcuMCU01;
VcuMCU02 vcuMCU02;
VcutoMCUCurrentCommand vcutoMCUCurrentCommand;
McuVCU1 mcuVCU1;
McuTemperature1 mcuTemperature1;
McuTemperature2 mcuTemperature2;
McuStatus mcuStatus;
McuCurrentVoltage mcuCurrentVoltage;
McuDeratingStatus mcuDeratingStatus;
McuFailureCode mcuFailureCode;

IMsgConv* rxArr[] = {&vcuMCU01, &vcuMCU02, &vcutoMCUCurrentCommand};
IMsgConv* txArr[] = {&mcuVCU1, &mcuTemperature1, &mcuTemperature2, &mcuStatus, &mcuCurrentVoltage, &mcuDeratingStatus, &mcuFailureCode}; 