#pragma once

#include "DataModel.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct CANMessage;

struct DbcSignalDef {
    uint32_t messageId = 0;
    std::string messageName;
    std::string signalName;
    uint8_t startBit = 0;
    uint8_t length = 0;
    double factor = 1.0;
    double offset = 0.0;
    double minValue = 0.0;
    double maxValue = 0.0;
};

struct DbcRxSignal {
    DbcSignalDef def;
    std::function<void(DataModel&, double)> set;
};

struct DbcTxSignal {
    DbcSignalDef def;
    std::function<double(const DataModel&)> get;
};

struct DbcTxMessage {
    uint32_t messageId = 0;
    uint8_t dlc = 8;
    std::vector<DbcTxSignal> signals;
};

class DbcSignalCache {
public:
    static DbcSignalCache& instance();

    const std::vector<DbcRxSignal>* rxSignals(uint32_t messageId) const;
    const DbcTxMessage* txMessage(const std::string& commandName) const;
    bool initialized() const;

private:
    DbcSignalCache();

    void loadDbc();
    void buildDefaultSelection();
    const DbcSignalDef* findSignal(const std::string& signalName) const;
    void selectRx(const std::string& signalName, std::function<void(DataModel&, double)> set);
    void selectTx(
        const std::string& commandName,
        const std::string& signalName,
        std::function<double(const DataModel&)> get);

    bool initialized_ = false;
    std::unordered_map<std::string, DbcSignalDef> signalsByName_;
    std::unordered_map<uint32_t, std::vector<DbcRxSignal>> rxByMessageId_;
    std::unordered_map<std::string, DbcTxMessage> txByCommand_;
};

uint32_t unpackDbcSignal(const uint8_t* data, uint8_t startBit, uint8_t length);
void packDbcSignal(uint8_t* data, uint32_t value, uint8_t startBit, uint8_t length);

