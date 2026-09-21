#pragma once

#include "DataModel.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_set>
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

struct DbcSignalSelectionEntry {
    DbcSignalDef def;
    bool selected = false;
    bool tx = false;
    std::string commandName;
};

class DbcSignalCache {
public:
    static DbcSignalCache& instance();

    const std::vector<DbcRxSignal>* rxSignals(uint32_t messageId) const;
    const DbcTxMessage* txMessage(const std::string& commandName) const;
    bool initialized() const;
    std::vector<DbcSignalSelectionEntry> selectionCatalog() const;
    std::vector<DbcSignalDef> messageSignals(uint32_t messageId) const;
    void decodeSelectedRx(
        uint32_t messageId,
        const uint8_t* payload,
        uint8_t payloadLength,
        DataModel& data) const;
    bool setSelection(const std::vector<std::string>& rxNames, const std::vector<std::string>& txNames);
    bool isRxSelected(const std::string& signalName) const;
    bool isTxSelected(const std::string& signalName) const;

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
    mutable std::mutex mutex_;
    std::unordered_map<std::string, DbcSignalDef> signalsByName_;
    std::unordered_map<uint32_t, std::vector<DbcSignalDef>> defsByMessageId_;
    std::unordered_map<uint32_t, std::vector<DbcRxSignal>> allRxByMessageId_;
    std::unordered_map<std::string, DbcTxMessage> allTxByCommand_;
    std::unordered_set<std::string> selectedRx_;
    std::unordered_set<std::string> selectedTx_;
};

uint32_t unpackDbcSignal(const uint8_t* data, uint8_t startBit, uint8_t length);
void packDbcSignal(uint8_t* data, uint32_t value, uint8_t startBit, uint8_t length);
