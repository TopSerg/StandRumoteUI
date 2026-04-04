//Stand_Marathon/src/CommandSender.h
#pragma once
#include "CANInterface.h"
#include "DataModel.h"

class CommandSender {
public:
    static void sendControlCommand(CANInterface& can, const DataModel& data);
    static void sendLimitCommand(CANInterface& can, const DataModel& data);
    static void sendTorqueCommand(CANInterface& can, DataModel& data);
};
