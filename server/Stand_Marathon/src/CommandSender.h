//Stand_Marathon/src/CommandSender.h
#pragma once
#include "CANInterface.h"
#include "DataModel.h"
#include <string>

class CommandSender {
public:
    static void sendControlCommand(CANInterface& can, const DataModel& data);
    static void sendLimitCommand(CANInterface& can, const DataModel& data);
    static void sendTorqueCommand(CANInterface& can, DataModel& data);
    static void sendResolverCalibrationCommand(CANInterface& can, const DataModel& data, bool enable);
    static void sendSafeDisable(CANInterface& can, DataModel& data);

private:
    static bool sendCachedCommand(CANInterface& can, const DataModel& data, const std::string& commandName);
};
