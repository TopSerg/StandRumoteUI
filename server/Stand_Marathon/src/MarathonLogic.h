//Stand_Marathon/src/MarathonLogic.h
#pragma once
#include "DataModel.h"
#include "CANInterface.h"

class MarathonLogic {
public:
    static void updateFromCAN(const CANMessage& msg, DataModel& data);
};
