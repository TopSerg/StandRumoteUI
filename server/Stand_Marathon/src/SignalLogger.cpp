#include "SignalLogger.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace {

std::string csvEscape(const std::string& value)
{
    if (value.find_first_of(";\"\n\r") == std::string::npos) {
        return value;
    }
    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped += ch;
        }
    }
    escaped += '"';
    return escaped;
}

std::string timestampNow()
{
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return out.str();
}

} // namespace

SignalLogger& SignalLogger::instance()
{
    static SignalLogger logger;
    return logger;
}

SignalLogger::SignalLogger() = default;

void SignalLogger::log(
    const char* direction,
    const DbcSignalDef& def,
    uint32_t raw,
    double physical,
    bool selected)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string ts = timestampNow();
    std::cout << "[DBC " << direction << "] "
              << "id=0x" << std::hex << std::uppercase << def.messageId << std::dec
              << " " << def.signalName
              << " raw=" << raw
              << " physical=" << physical
              << " selected=" << (selected ? 1 : 0)
              << std::endl;

    auto it = std::find_if(selectedSamples_.begin(), selectedSamples_.end(),
        [&](const LoggedSignalSample& sample) {
            return sample.direction == direction && sample.signalName == def.signalName;
        });

    if (selected) {
        LoggedSignalSample sample;
        sample.direction = direction;
        sample.messageId = def.messageId;
        sample.messageName = def.messageName;
        sample.signalName = def.signalName;
        sample.raw = raw;
        sample.physical = physical;
        if (it == selectedSamples_.end()) {
            selectedSamples_.push_back(std::move(sample));
        } else {
            *it = std::move(sample);
        }
    } else if (it != selectedSamples_.end()) {
        selectedSamples_.erase(it);
    }

    if (!csv_.is_open()) {
        csv_.open("dbc_signals.csv", std::ios::app);
        if (csv_ && csv_.tellp() == 0) {
            csv_ << "timestamp;direction;message_id;message_name;signal_name;raw;physical;selected\n";
        }
    }
    if (!csv_) {
        return;
    }
    csv_ << csvEscape(ts) << ';'
         << csvEscape(direction) << ';'
         << def.messageId << ';'
         << csvEscape(def.messageName) << ';'
         << csvEscape(def.signalName) << ';'
         << raw << ';'
         << physical << ';'
         << (selected ? 1 : 0)
         << '\n';
    csv_.flush();
}

void SignalLogger::captureSelectedPayload(
    const char* direction,
    uint32_t messageId,
    const uint8_t* payload,
    uint8_t payloadLength)
{
    if (!direction || !payload) {
        return;
    }

    DbcSignalCache& cache = DbcSignalCache::instance();
    const bool tx = std::string(direction) == "TX";
    std::vector<LoggedSignalSample> decoded;
    for (const DbcSignalDef& def : cache.messageSignals(messageId)) {
        const bool selected = tx
            ? cache.isTxSelected(def.signalName)
            : cache.isRxSelected(def.signalName);
        if (!selected || def.length == 0 || def.length > 32) {
            continue;
        }

        const uint8_t currentStartBit = static_cast<uint8_t>((def.length - 1) % 8);
        const uint8_t startBitInByte = static_cast<uint8_t>(def.startBit % 8);
        const uint8_t extraByte = startBitInByte < currentStartBit ? 1 : 0;
        const uint8_t lastByte = static_cast<uint8_t>(
            def.startBit / 8 + (def.length - 1) / 8 + extraByte);
        if (lastByte >= payloadLength) {
            continue;
        }

        const uint32_t raw = unpackDbcSignal(payload, def.startBit, def.length);
        decoded.push_back(LoggedSignalSample{
            direction,
            def.messageId,
            def.messageName,
            def.signalName,
            raw,
            static_cast<double>(raw) * def.factor + def.offset,
        });
    }

    std::lock_guard<std::mutex> lock(mutex_);
    for (LoggedSignalSample& sample : decoded) {
        auto current = std::find_if(selectedSamples_.begin(), selectedSamples_.end(),
            [&](const LoggedSignalSample& value) {
                return value.direction == sample.direction &&
                    value.signalName == sample.signalName;
            });
        if (current == selectedSamples_.end()) {
            selectedSamples_.push_back(std::move(sample));
        } else {
            *current = std::move(sample);
        }
    }
}

std::vector<LoggedSignalSample> SignalLogger::selectedSamples() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return selectedSamples_;
}
