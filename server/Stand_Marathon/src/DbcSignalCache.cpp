#include "DbcSignalCache.h"

#include "CANInterface.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>

namespace {

std::string normalizeName(std::string name)
{
    std::replace(name.begin(), name.end(), 'T', 'T');
    return name;
}

std::ifstream openDbcFile()
{
    const char* paths[] = {
        "KAMA_FP_EPT_0615.dbc",
        "..\\KAMA_FP_EPT_0615.dbc",
        "..\\..\\KAMA_FP_EPT_0615.dbc",
        "..\\..\\..\\KAMA_FP_EPT_0615.dbc",
        "server\\KAMA_FP_EPT_0615.dbc",
    };

    for (const char* path : paths) {
        std::ifstream file(path);
        if (file) {
            std::cout << "[DBC] loaded " << path << std::endl;
            return file;
        }
    }

    return {};
}

double clampPhysical(double value, const DbcSignalDef& def)
{
    if (def.maxValue > def.minValue) {
        value = (std::max)(def.minValue, (std::min)(def.maxValue, value));
    }
    return value;
}

uint32_t physicalToRaw(double value, const DbcSignalDef& def)
{
    value = clampPhysical(value, def);
    double raw = (value - def.offset) / def.factor;
    if (!std::isfinite(raw)) {
        raw = 0.0;
    }

    const double rounded = std::round(raw);
    const uint64_t maxRaw = def.length >= 32 ? 0xffffffffULL : ((1ULL << def.length) - 1ULL);
    if (rounded < 0.0) {
        return 0;
    }
    if (rounded > static_cast<double>(maxRaw)) {
        return static_cast<uint32_t>(maxRaw);
    }
    return static_cast<uint32_t>(rounded);
}

} // namespace

uint32_t unpackDbcSignal(const uint8_t* data, uint8_t startBit, uint8_t length)
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

void packDbcSignal(uint8_t* data, uint32_t value, uint8_t startBit, uint8_t length)
{
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
    value <<= shift;
    startByteInValue = (length - 1) / 8 + lengthInBytesIncreased;
    for (int i = startByteInValue; i >= 0; i--)
    {
        data[startByteInData + startByteInValue - i] |= (uint8_t)(value >> (i * 8));
    }
}

DbcSignalCache& DbcSignalCache::instance()
{
    static DbcSignalCache cache;
    return cache;
}

DbcSignalCache::DbcSignalCache()
{
    loadDbc();
    buildDefaultSelection();
}

bool DbcSignalCache::initialized() const
{
    return initialized_;
}

const std::vector<DbcRxSignal>* DbcSignalCache::rxSignals(uint32_t messageId) const
{
    auto it = rxByMessageId_.find(messageId);
    if (it == rxByMessageId_.end()) {
        return nullptr;
    }
    return &it->second;
}

const DbcTxMessage* DbcSignalCache::txMessage(const std::string& commandName) const
{
    auto it = txByCommand_.find(commandName);
    if (it == txByCommand_.end()) {
        return nullptr;
    }
    return &it->second;
}

void DbcSignalCache::loadDbc()
{
    std::ifstream dbc = openDbcFile();
    if (!dbc) {
        std::cerr << "[DBC] cannot open KAMA_FP_EPT_0615.dbc" << std::endl;
        return;
    }

    std::regex boRegex(R"(^\s*BO_\s+(\d+)\s+(\S+)\s*:\s+(\d+)\s+(\S+))");
    std::regex sgRegex(R"(^\s*SG_\s+(\S+)\s*:\s*(\d+)\|(\d+)@([01])([+-])\s+\(([-+0-9.eE]+),([-+0-9.eE]+)\)\s+\[([-+0-9.eE]+)\|([-+0-9.eE]+)\])");

    std::string line;
    uint32_t currentMessageId = 0;
    std::string currentMessageName;
    bool inMessage = false;

    while (std::getline(dbc, line)) {
        std::smatch match;
        if (std::regex_search(line, match, boRegex)) {
            currentMessageId = static_cast<uint32_t>(std::stoul(match[1].str()));
            currentMessageName = match[2].str();
            inMessage = true;
            continue;
        }

        if (!inMessage || !std::regex_search(line, match, sgRegex)) {
            continue;
        }

        DbcSignalDef def;
        def.messageId = currentMessageId;
        def.messageName = currentMessageName;
        def.signalName = normalizeName(match[1].str());
        def.startBit = static_cast<uint8_t>(std::stoul(match[2].str()));
        def.length = static_cast<uint8_t>(std::stoul(match[3].str()));
        def.factor = std::stod(match[6].str());
        def.offset = std::stod(match[7].str());
        def.minValue = std::stod(match[8].str());
        def.maxValue = std::stod(match[9].str());

        signalsByName_[def.signalName] = def;
    }

    initialized_ = !signalsByName_.empty();
    std::cout << "[DBC] cached " << signalsByName_.size() << " signals" << std::endl;
}

const DbcSignalDef* DbcSignalCache::findSignal(const std::string& signalName) const
{
    auto it = signalsByName_.find(signalName);
    if (it == signalsByName_.end()) {
        std::cerr << "[DBC] signal not found: " << signalName << std::endl;
        return nullptr;
    }
    return &it->second;
}

void DbcSignalCache::selectRx(const std::string& signalName, std::function<void(DataModel&, double)> set)
{
    const DbcSignalDef* def = findSignal(signalName);
    if (!def) {
        return;
    }
    rxByMessageId_[def->messageId].push_back(DbcRxSignal{*def, std::move(set)});
}

void DbcSignalCache::selectTx(
    const std::string& commandName,
    const std::string& signalName,
    std::function<double(const DataModel&)> get)
{
    const DbcSignalDef* def = findSignal(signalName);
    if (!def) {
        return;
    }

    DbcTxMessage& msg = txByCommand_[commandName];
    msg.messageId = def->messageId;
    msg.dlc = 8;
    msg.signals.push_back(DbcTxSignal{*def, std::move(get)});
}

void DbcSignalCache::buildDefaultSelection()
{
    if (!initialized_) {
        return;
    }

    selectRx("MCU_ActualTorque", [](DataModel& d, double v) { d.Ms = static_cast<float>(v); });
    selectRx("MCU_UdcCurr", [](DataModel& d, double v) { d.Udc = static_cast<float>(v); });
    selectRx("MCU_IsCurr", [](DataModel& d, double v) { d.Idc = static_cast<float>(v); });
    selectRx("MCU_ActualSpeed", [](DataModel& d, double v) { d.ns = static_cast<float>(v); });
    selectRx("MCU_IGBTTempU", [](DataModel& d, double v) { d.MCU_IGBTTempU = static_cast<float>(v); });
    selectRx("MCU_IGBTTempV", [](DataModel& d, double v) { d.MCU_IGBTTempV = static_cast<float>(v); });
    selectRx("MCU_IGBTTempW", [](DataModel& d, double v) { d.MCU_IGBTTempW = static_cast<float>(v); });
    selectRx("MCU_IGBTTempMax", [](DataModel& d, double v) { d.MCU_IGBTTempMax = static_cast<float>(v); });
    selectRx("MCU_TempCurrCool", [](DataModel& d, double v) { d.MCU_TempCurrCool = static_cast<float>(v); });
    selectRx("MCU_TempCurrStr", [](DataModel& d, double v) { d.MCU_TempCurrStr = static_cast<float>(v); });
    selectRx("MCU_OfsAl", [](DataModel& d, double v) { d.MCU_OfsAl = static_cast<float>(v); });
    selectRx("MCU_Isd", [](DataModel& d, double v) { d.MCU_Isd = static_cast<float>(v); });
    selectRx("MCU_Isq", [](DataModel& d, double v) { d.MCU_Isq = static_cast<float>(v); });
    selectRx("MCU_bDmpCActv", [](DataModel& d, double v) { d.MCU_bDmpCActv = static_cast<uint8_t>(v); });
    selectRx("MCU_stGateDrv", [](DataModel& d, double v) { d.MCU_stGateDrv = static_cast<uint8_t>(v); });
    selectRx("MCU_DmpCTrqCurr", [](DataModel& d, double v) { d.MCU_DmpCTrqCurr = static_cast<float>(v); });
    selectRx("MCU_VCUWorkMode", [](DataModel& d, double v) { d.MCU_VCUWorkMode = static_cast<uint8_t>(v); });
    selectRx("Ud", [](DataModel& d, double v) { d.Ud = static_cast<float>(v); });
    selectRx("Uq", [](DataModel& d, double v) { d.Uq = static_cast<float>(v); });
    selectRx("Id", [](DataModel& d, double v) { d.Id = static_cast<float>(v); });
    selectRx("Iq", [](DataModel& d, double v) { d.Iq = static_cast<float>(v); });
    selectRx("ZVTimeStamp", [](DataModel& d, double v) { d.ZVTimeStamp = static_cast<float>(v); });
    selectRx("ZVTHetha", [](DataModel& d, double v) { d.ZVTheta = static_cast<float>(v); });
    selectRx("ZVTHethaCorr", [](DataModel& d, double v) { d.ZVThetaCorr = static_cast<float>(v); });
    selectRx("ZVTemperature", [](DataModel& d, double v) { d.ZVTemperature = static_cast<float>(v); });
    selectRx("ZVFlux", [](DataModel& d, double v) { d.ZVFlux = static_cast<float>(v); });
    selectRx("ZVRs", [](DataModel& d, double v) { d.ZVRs = static_cast<float>(v); });
    selectRx("MCU_TrqAbsMax", [](DataModel& d, double v) { d.M_max = static_cast<float>(v); });
    selectRx("MCU_TrqAbsMin", [](DataModel& d, double v) { d.M_min = static_cast<float>(v); });

    selectTx("SendControl", "VCU_KL15On", [](const DataModel& d) { return d.Kl_15 ? 1.0 : 0.0; });
    selectTx("SendControl", "VCU_MCUDesiredTorque", [](const DataModel& d) {
        return d.MotorCtrl == 1 ? d.M_desired : d.M_desired / 15.675;
    });
    selectTx("SendControl", "VCU_MCUSurgeDamperState", [](const DataModel& d) { return d.SurgeDamperState; });
    selectTx("SendControl", "VCU_BrakepedalStatus", [](const DataModel& d) { return d.Brake_active ? 1.0 : 0.0; });
    selectTx("SendControl", "VCU_MCURequestedState", [](const DataModel& d) { return d.MotorCtrl & 0x0F; });
    selectTx("SendControl", "VCU_TCSActive", [](const DataModel& d) { return d.TCS_active ? 1.0 : 0.0; });
    selectTx("SendControl", "VCU_ActualGear", [](const DataModel& d) { return d.GearCtrl & 0x07; });
    selectTx("SendControl", "MessageCounter_046", [](const DataModel&) { static uint8_t c = 0; return c++ & 0x0F; });
    selectTx("SendControl", "Checksum_046", [](const DataModel&) { return 0.0; });

    selectTx("SendLimits", "VCU_MinTorqueLimit", [](const DataModel& d) { return d.M_min; });
    selectTx("SendLimits", "VCU_MaxTorqueGradient", [](const DataModel& d) { return d.M_grad_max; });
    selectTx("SendLimits", "VCU_MaxTorqueLimit", [](const DataModel& d) { return d.M_max; });
    selectTx("SendLimits", "VCU_TrqThresholdDampgCtl", [](const DataModel& d) { return d.TrqThresholdDampgCtl; });
    selectTx("SendLimits", "MessageCounter_047", [](const DataModel&) { static uint8_t c = 0; return c++ & 0x0F; });
    selectTx("SendLimits", "Checksum_047", [](const DataModel&) { return 0.0; });
    selectTx("SendLimits", "VCU_MaxSpeed", [](const DataModel& d) { return d.n_max; });

    selectTx("SendTorque", "VCU_IdCommand", [](const DataModel& d) { return d.Isd; });
    selectTx("SendTorque", "VCU_IqCommand", [](const DataModel& d) { return d.Isq; });
    selectTx("SendTorque", "VCU_CurrentCommandEnable", [](const DataModel& d) { return d.En_Is ? 1.0 : 0.0; });
    selectTx("SendTorque", "MessageCounter_300", [](const DataModel&) { static uint8_t c = 0; return c++ & 0x0F; });
    selectTx("SendTorque", "Checksum_300", [](const DataModel&) { return 0.0; });
}
