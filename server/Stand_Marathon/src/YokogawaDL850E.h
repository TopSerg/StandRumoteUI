// YokogawaDL850E.h
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

/*
  Minimal wrapper for Yokogawa DL850E FreeRun API (ScAPI) over LAN.

  Requirements (Windows):
    - ScAPI.h in include path
    - Link with ScAPI64.lib (x64) or ScAPI.lib (x86)
    - ScAPI64.dll + tmctl64.dll must be near the exe (or in PATH)

  Transport:
    - DL850E connected via LAN using VXI-11 (SC_WIRE_LAN)

  Notes:
    - ScAPI does not allow multiple simultaneous connections to the same instrument.
*/

class YokogawaDL850E final {
public:
    struct Waveform {
        int channel = 1;         // 1..16
        int subChannel = 0;      // 0 (no sub) or 1..60 for logic modules

        double baseSamplingRateHz = 0.0;
        int channelSamplingRatio = 1;   // base / ratio = channel rate (approx)
        double channelSamplingRateHz = 0.0;

        int bits = 0;            // from ScGetChannelBits
        int dataSizeBits = 0;    // from ScGetLatchAcqData

        double gain = 1.0;       // from ScGetChannelGain
        double offset = 0.0;     // from ScGetChannelOffset

        std::string startTime;   // from ScGetStartTime (string format from instrument)

        std::vector<int32_t> raw;     // sign-extended integer samples
        std::vector<double>  scaled;  // raw*gain + offset
    };

    YokogawaDL850E();
    ~YokogawaDL850E();

    YokogawaDL850E(const YokogawaDL850E&) = delete;
    YokogawaDL850E& operator=(const YokogawaDL850E&) = delete;

    // Connect/disconnect
    bool connectLan(const std::string& ipOrHost);   // e.g. "192.168.0.10"
    void disconnect();
    bool isConnected() const;

    // Basic control/query (SCPI strings)
    bool set(const std::string& command);           // e.g. ":STOP"
    bool query(const std::string& message, std::string& outReply);

    // Convenience: read *IDN?
    bool identify(std::string& outIdn);

    // FreeRun control helpers
    bool start();
    bool stop();
    bool latch();

    // Sample counters relative to last latch
    bool getLatchCount(int64_t& outCount);          // samples from LATCH position
    bool getLatchIntervalCount(int64_t& outCount);  // samples between previous and current LATCH

    // Capture waveform after LATCH.
    // If maxSamples <= 0 -> will try to capture "all available since LATCH", but will clamp by hardLimitSamples.
    bool getLatchWaveform(int channel,
                          int subChannel,
                          Waveform& out,
                          int64_t maxSamples = 200000,
                          int64_t hardLimitSamples = 2000000);

    // Save waveform to CSV: index,time_s,raw,scaled
    static bool saveWaveformCsv(const Waveform& wf, const std::string& filePath, std::string* err = nullptr);

    const std::string& lastError() const { return lastError_; }

private:
    // Internal helpers
    bool ensureApiInit();
    void apiExitIfNeeded();

    bool fetchChannelMeta(int channel, int subChannel, Waveform& out);
    static int bytesForBits(int bits);
    static int32_t signExtend(uint32_t v, int bits);
    static std::string trimReply(const std::string& s);

private:
    mutable std::mutex mtx_;
    long handle_ = 0; // ScHandle (typedef long)

    std::string lastError_;

    // Process-wide API init refcount
    static std::mutex apiMtx_;
    static int apiRefCount_;
};
