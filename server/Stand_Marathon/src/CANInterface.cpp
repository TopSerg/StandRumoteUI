//Stand_Marathon/src/CANInterface.cpp
#include "CANInterface.h"
#include <iostream>

#include <cstdio>
#include <string>

static inline bool WS_CAN_LOG() {
    static int flag = -1;
    if (flag < 0) {
        const char* v = std::getenv("WS_LOG_CAN");
        flag = (v && *v == '1') ? 1 : 0;
    }
    return flag == 1;
}

static inline std::string to_hex(const uint8_t* data, int len) {
    static const char* H = "0123456789ABCDEF";
    std::string s; s.reserve(len * 3);
    for (int i = 0; i < len; ++i) {
        uint8_t b = data[i];
        s.push_back(H[b >> 4]);
        s.push_back(H[b & 0x0F]);
        if (i + 1 < len) s.push_back(' ');
    }
    return s;
}

CANInterface::CANInterface() : handle(PCAN_NONEBUS) {}

CANInterface::~CANInterface() {
    stop();
}

bool CANInterface::init(uint16_t channel, uint32_t baudrate, uint32_t flags) {
    // ✅ Маппинг канала PCAN
    switch (channel) {
        case 1: handle = PCAN_USBBUS1; break;
        case 2: handle = PCAN_USBBUS2; break;
        case 3: handle = PCAN_USBBUS3; break;
        case 4: handle = PCAN_USBBUS4; break;
        default:
            std::cerr << "Invalid channel, defaulting to PCAN_USBBUS1" << std::endl;
            handle = PCAN_USBBUS1;
            break;
    }

    // ✅ Маппинг скорости
    TPCANBaudrate pcanBaud = PCAN_BAUD_500K; // дефолт
    std::cout << baudrate << std::endl;
    if (baudrate == 1000000) pcanBaud = PCAN_BAUD_1M;
    else if (baudrate == 500000) pcanBaud = PCAN_BAUD_500K;
    else if (baudrate == 250000) pcanBaud = PCAN_BAUD_250K;
    else if (baudrate == 125000) pcanBaud = PCAN_BAUD_125K;
    else if (baudrate == 50000)  pcanBaud = PCAN_BAUD_50K;
    else if (baudrate == 20000)  pcanBaud = PCAN_BAUD_20K;
    else if (baudrate == 10000)  pcanBaud = PCAN_BAUD_10K;

    pcanBaud = PCAN_BAUD_1M;

    // ✅ Игнорируем flags, если они не нужны
    (void)flags;

    // ✅ Инициализация
    TPCANStatus status = CAN_Initialize(handle, pcanBaud);
    if (status != PCAN_ERROR_OK) {
        std::cerr << "CAN init failed: " << getErrorText(status) << std::endl;
        return false;
    }

    TPCANStatus flt = PCAN_FILTER_OPEN;
    status = CAN_SetValue(handle, PCAN_MESSAGE_FILTER, &flt, sizeof(flt));
    if (status != PCAN_ERROR_OK) {
        std::cerr << "Filter open failed: " << getErrorText(status) << std::endl;
    }

    initialized = true;
    std::cout << "[INFO] CAN initialized on channel " << channel
              << " at baud " << baudrate << std::endl;
    return true;
}



void CANInterface::stop() {
    if (initialized) {
        CAN_Uninitialize(handle);
        initialized = false;
    }
}

/*uint8_t CANInterface::send(dbc_can_tx_message_type* message) {
    if (!initialized) return false;

    TPCANMsg msg{};
    msg.ID = message->message_id;
    msg.MSGTYPE = PCAN_MESSAGE_STANDARD;
    msg.LEN = message->dlc;
    std::copy(message->data, message->data + message->dlc, msg.DATA);

    TPCANStatus status = CAN_Write(handle, &msg);
    if (status != PCAN_ERROR_OK) {
        std::cerr << "CAN send failed: " << getErrorText(status) << std::endl;
        return false;
    }
    return true;
}*/

bool CANInterface::send(uint32_t id, const uint8_t* data, uint8_t length) {
    if (!initialized) return false;

    TPCANMsg msg{};
    msg.ID = id;
    msg.MSGTYPE = PCAN_MESSAGE_STANDARD;
    msg.LEN = length;
    std::copy(data, data + length, msg.DATA);

    if (WS_CAN_LOG()) {
        std::printf("[CAN TX] ID=0x%03X DLC=%d DATA=%s\n",
                    (unsigned)id, (int)length, to_hex(data, length).c_str());
    }
    
    TPCANStatus status = CAN_Write(handle, &msg);
    if (status != PCAN_ERROR_OK) {
        if (WS_CAN_LOG()) std::printf("[CAN TX][ERR] status=0x%08X\n", (unsigned)status);
        return false;
    }
    return true;
}

bool CANInterface::receive(CANMessage& msg) {
    if (!initialized) return false;

    TPCANMsg pcanMsg;
    TPCANTimestamp ts;

    TPCANStatus status = CAN_Read(handle, &pcanMsg, &ts);
    if (status != PCAN_ERROR_OK) return false;

    msg.id = pcanMsg.ID;
    msg.length = pcanMsg.LEN;
    std::copy(pcanMsg.DATA, pcanMsg.DATA + msg.length, msg.data);
    msg.timestamp = ts.micros + 1000 * ts.millis + 1000000 * ts.millis_overflow;

    return true;
}

std::string CANInterface::getErrorText(TPCANStatus error) {
    char buffer[256] = {};
    if (CAN_GetErrorText(error, 0x09, buffer) == PCAN_ERROR_OK) {
        return std::string(buffer);
    } else {
        return "Unknown error";
    }
}
