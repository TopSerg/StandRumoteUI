//Stand_Marathon/src/CANInterface.h
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <windows.h>
#include "PCANBasic.h"
#include "canDBCtask.h"

struct CANMessage {
    uint32_t id;
    uint8_t data[8];
    uint8_t length;
    uint32_t timestamp;
};

class CANInterface {
public:
    explicit CANInterface();
    ~CANInterface();

    bool init(uint16_t channel, uint32_t baudrate, uint32_t flags);
    void stop();

    //uint8_t send(dbc_can_tx_message_type* message);
    bool send(uint32_t id, const uint8_t* data, uint8_t length);
    bool receive(CANMessage& msg);

    enum BUSNUMBER{
        BUS1 = 1,
        BUS2 = 2,
        BUS3 = 3,
        BUS4 = 4,
    };

private:
    TPCANHandle handle;
    bool initialized;

    std::string getErrorText(TPCANStatus error);
};
