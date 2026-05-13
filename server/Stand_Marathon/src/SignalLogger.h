#pragma once

#include "DbcSignalCache.h"

#include <cstdint>
#include <mutex>
#include <fstream>
#include <string>
#include <vector>

struct LoggedSignalSample {
    std::string direction;
    uint32_t messageId = 0;
    std::string messageName;
    std::string signalName;
    uint32_t raw = 0;
    double physical = 0.0;
};

class SignalLogger {
public:
    static SignalLogger& instance();

    void log(
        const char* direction,
        const DbcSignalDef& def,
        uint32_t raw,
        double physical,
        bool selected);
    std::vector<LoggedSignalSample> selectedSamples() const;

private:
    SignalLogger();

    mutable std::mutex mutex_;
    std::ofstream csv_;
    std::vector<LoggedSignalSample> selectedSamples_;
};
