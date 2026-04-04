// YokogawaDL850E.cpp

#include "YokogawaDL850E.h"

#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <limits>

#ifdef _WIN32
// FreeRun API header from the vendor package.
// Put DL850E_FREERUNAPI_v1000/vc/ into your include dirs.
#include "ScAPI.h"
#endif

std::mutex YokogawaDL850E::apiMtx_;
int YokogawaDL850E::apiRefCount_ = 0;

YokogawaDL850E::YokogawaDL850E() = default;

YokogawaDL850E::~YokogawaDL850E() {
    disconnect();
    apiExitIfNeeded();
}

bool YokogawaDL850E::ensureApiInit() {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(apiMtx_);
    if (apiRefCount_ == 0) {
        const int rc = ScInit();
        if (rc != SC_SUCCESS) {
            lastError_ = "ScInit() failed";
            return false;
        }
    }
    ++apiRefCount_;
    return true;
#else
    lastError_ = "YokogawaDL850E: ScAPI is supported only on Windows in this wrapper.";
    return false;
#endif
}

void YokogawaDL850E::apiExitIfNeeded() {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(apiMtx_);
    if (apiRefCount_ > 0) {
        --apiRefCount_;
        if (apiRefCount_ == 0) {
            (void)ScExit();
        }
    }
#endif
}

bool YokogawaDL850E::connectLan(const std::string& ipOrHost) {
    std::lock_guard<std::mutex> lk(mtx_);

#ifdef _WIN32
    if (handle_ != 0) {
        lastError_ = "Already connected";
        return false;
    }
    if (!ensureApiInit()) return false;

    ScHandle h = 0;
    // ScOpenInstrument takes char* (non-const), so we must provide a writable buffer.
    std::vector<char> addr(ipOrHost.begin(), ipOrHost.end());
    addr.push_back('\0');

    const int rc = ScOpenInstrument(SC_WIRE_LAN, addr.data(), &h);
    if (rc != SC_SUCCESS || h == 0) {
        lastError_ = "ScOpenInstrument(SC_WIRE_LAN) failed for " + ipOrHost;
        apiExitIfNeeded();
        return false;
    }

    handle_ = static_cast<long>(h);
    return true;
#else
    (void)ipOrHost;
    lastError_ = "connectLan(): not supported on this platform";
    return false;
#endif
}

void YokogawaDL850E::disconnect() {
    std::lock_guard<std::mutex> lk(mtx_);
#ifdef _WIN32
    if (handle_ != 0) {
        (void)ScCloseInstrument(static_cast<ScHandle>(handle_));
        handle_ = 0;
        // keep API init until destructor (refcount managed there)
    }
#endif
}

bool YokogawaDL850E::isConnected() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return handle_ != 0;
}

bool YokogawaDL850E::set(const std::string& command) {
    std::lock_guard<std::mutex> lk(mtx_);
#ifdef _WIN32
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }
    std::vector<char> cmd(command.begin(), command.end());
    cmd.push_back('\0');
    const int rc = ScSetControl(static_cast<ScHandle>(handle_), cmd.data());
    if (rc != SC_SUCCESS) {
        lastError_ = "ScSetControl failed: " + command;
        return false;
    }
    return true;
#else
    (void)command;
    lastError_ = "set(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::query(const std::string& message, std::string& outReply) {
    std::lock_guard<std::mutex> lk(mtx_);
#ifdef _WIN32
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }

    std::vector<char> msg(message.begin(), message.end());
    msg.push_back('\0');

    // Typical replies are short; 8 KB is safe for most SCPI text responses.
    std::vector<char> buff(8192, 0);
    int recvLen = 0;

    const int rc = ScQueryMessage(static_cast<ScHandle>(handle_), msg.data(),
                                  buff.data(), static_cast<int>(buff.size()), &recvLen);
    if (rc != SC_SUCCESS) {
        lastError_ = "ScQueryMessage failed: " + message;
        return false;
    }
    if (recvLen < 0) recvLen = 0;
    outReply.assign(buff.data(), buff.data() + recvLen);
    outReply = trimReply(outReply);
    return true;
#else
    (void)message; (void)outReply;
    lastError_ = "query(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::identify(std::string& outIdn) {
    return query("*idn?", outIdn);
}

bool YokogawaDL850E::start() {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(mtx_);
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }
    const int rc = ScStart(static_cast<ScHandle>(handle_));
    if (rc != SC_SUCCESS) { lastError_ = "ScStart failed"; return false; }
    return true;
#else
    lastError_ = "start(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::stop() {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(mtx_);
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }
    const int rc = ScStop(static_cast<ScHandle>(handle_));
    if (rc != SC_SUCCESS) { lastError_ = "ScStop failed"; return false; }
    return true;
#else
    lastError_ = "stop(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::latch() {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(mtx_);
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }
    const int rc = ScLatchData(static_cast<ScHandle>(handle_));
    if (rc != SC_SUCCESS) { lastError_ = "ScLatchData failed"; return false; }
    return true;
#else
    lastError_ = "latch(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::getLatchCount(int64_t& outCount) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(mtx_);
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }
    __int64 cnt = 0;
    const int rc = ScGetLatchCount(static_cast<ScHandle>(handle_), &cnt);
    if (rc != SC_SUCCESS) { lastError_ = "ScGetLatchCount failed"; return false; }
    outCount = static_cast<int64_t>(cnt);
    return true;
#else
    (void)outCount;
    lastError_ = "getLatchCount(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::getLatchIntervalCount(int64_t& outCount) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(mtx_);
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }
    __int64 cnt = 0;
    const int rc = ScGetLatchIntervalCount(static_cast<ScHandle>(handle_), &cnt);
    if (rc != SC_SUCCESS) { lastError_ = "ScGetLatchIntervalCount failed"; return false; }
    outCount = static_cast<int64_t>(cnt);
    return true;
#else
    (void)outCount;
    lastError_ = "getLatchIntervalCount(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::fetchChannelMeta(int channel, int subChannel, Waveform& out) {
#ifdef _WIN32
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }

    out.channel = channel;
    out.subChannel = subChannel;

    // Start time (instrument format)
    {
        std::vector<char> tb(128, 0);
        const int rc = ScGetStartTime(static_cast<ScHandle>(handle_), tb.data());
        if (rc == SC_SUCCESS) out.startTime = trimReply(std::string(tb.data()));
    }

    // Base sampling rate
    {
        double base = 0.0;
        const int rc = ScGetBaseSamplingRate(static_cast<ScHandle>(handle_), &base);
        if (rc == SC_SUCCESS) out.baseSamplingRateHz = base;
    }

    // Channel sampling ratio
    {
        int ratio = 1;
        const int rc = ScGetChannelSamplingRatio(static_cast<ScHandle>(handle_), channel, &ratio);
        if (rc == SC_SUCCESS && ratio > 0) out.channelSamplingRatio = ratio;
    }

    if (out.baseSamplingRateHz > 0.0 && out.channelSamplingRatio > 0) {
        out.channelSamplingRateHz = out.baseSamplingRateHz / static_cast<double>(out.channelSamplingRatio);
    }

    // Bits / gain / offset for scaling
    {
        int bits = 0;
        const int rc = ScGetChannelBits(static_cast<ScHandle>(handle_), channel, subChannel, &bits);
        if (rc == SC_SUCCESS) out.bits = bits;
    }
    {
        double g = 1.0;
        const int rc = ScGetChannelGain(static_cast<ScHandle>(handle_), channel, subChannel, &g);
        if (rc == SC_SUCCESS) out.gain = g;
    }
    {
        double off = 0.0;
        const int rc = ScGetChannelOffset(static_cast<ScHandle>(handle_), channel, subChannel, &off);
        if (rc == SC_SUCCESS) out.offset = off;
    }
    return true;
#else
    (void)channel; (void)subChannel; (void)out;
    return false;
#endif
}

int YokogawaDL850E::bytesForBits(int bits) {
    if (bits <= 0) return 2; // safe default for analog
    return (bits + 7) / 8;
}

int32_t YokogawaDL850E::signExtend(uint32_t v, int bits) {
    if (bits <= 0 || bits >= 32) return static_cast<int32_t>(v);
    const uint32_t m = 1u << (bits - 1);
    // Two's complement sign-extend
    return static_cast<int32_t>((v ^ m) - m);
}

std::string YokogawaDL850E::trimReply(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == '\r' || s[b] == '\n' || s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == '\r' || s[e - 1] == '\n' || s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\0')) --e;
    return s.substr(b, e - b);
}

bool YokogawaDL850E::getLatchWaveform(int channel,
                                     int subChannel,
                                     Waveform& out,
                                     int64_t maxSamples,
                                     int64_t hardLimitSamples) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(mtx_);
    if (handle_ == 0) { lastError_ = "Not connected"; return false; }

    out = Waveform{};
    if (!fetchChannelMeta(channel, subChannel, out)) {
        lastError_ = "Failed to fetch channel meta";
        return false;
    }

    int64_t available = 0;
    {
        __int64 cnt = 0;
        const int rc = ScGetLatchCount(static_cast<ScHandle>(handle_), &cnt);
        if (rc != SC_SUCCESS) { lastError_ = "ScGetLatchCount failed"; return false; }
        available = static_cast<int64_t>(cnt);
    }

    int64_t want = maxSamples;
    if (want <= 0) want = available;
    if (want <= 0) { lastError_ = "No samples available after LATCH"; return false; }
    if (hardLimitSamples > 0 && want > hardLimitSamples) want = hardLimitSamples;

    const int bits = (out.bits > 0) ? out.bits : 16;
    const int bytesPerSample = bytesForBits(bits);

    // Allocate buffer big enough for "want" samples
    const int64_t bufBytes64 = want * static_cast<int64_t>(bytesPerSample);
    if (bufBytes64 > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        lastError_ = "Requested buffer too large";
        return false;
    }

    std::vector<char> buff(static_cast<size_t>(bufBytes64));
    int dataCount = 0;
    int dataSize = 0;

    const int rc = ScGetLatchAcqData(static_cast<ScHandle>(handle_),
                                     channel, subChannel,
                                     buff.data(), static_cast<int>(buff.size()),
                                     &dataCount, &dataSize);
    if (rc != SC_SUCCESS) {
        lastError_ = "ScGetLatchAcqData failed (ch=" + std::to_string(channel) + ")";
        return false;
    }
    if (dataCount <= 0) {
        lastError_ = "ScGetLatchAcqData returned 0 samples";
        return false;
    }
    out.dataSizeBits = dataSize;

    const int bytesPerSample2 = bytesForBits(dataSize > 0 ? dataSize : bits);
    out.raw.resize(static_cast<size_t>(dataCount));
    out.scaled.resize(static_cast<size_t>(dataCount));

    // Parse as little-endian packed integers, then sign-extend to 32-bit.
    const uint8_t* p = reinterpret_cast<const uint8_t*>(buff.data());
    for (int i = 0; i < dataCount; ++i) {
        uint32_t u = 0;
        // Copy up to 4 bytes
        for (int b = 0; b < bytesPerSample2 && b < 4; ++b) {
            u |= (static_cast<uint32_t>(p[i * bytesPerSample2 + b]) << (8u * b));
        }

        int32_t s;
        if (dataSize >= 32) {
            // Interpret as 32-bit signed
            int32_t tmp = 0;
            std::memcpy(&tmp, &u, sizeof(tmp));
            s = tmp;
        } else {
            const uint32_t mask = (dataSize > 0) ? ((1u << dataSize) - 1u) : 0xFFFFFFFFu;
            u &= mask;
            s = signExtend(u, dataSize > 0 ? dataSize : bits);
        }

        out.raw[static_cast<size_t>(i)] = s;
        out.scaled[static_cast<size_t>(i)] = static_cast<double>(s) * out.gain + out.offset;
    }

    return true;
#else
    (void)channel; (void)subChannel; (void)out; (void)maxSamples; (void)hardLimitSamples;
    lastError_ = "getLatchWaveform(): not supported on this platform";
    return false;
#endif
}

bool YokogawaDL850E::saveWaveformCsv(const Waveform& wf, const std::string& filePath, std::string* err) {
    std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        if (err) *err = "Failed to open file: " + filePath;
        return false;
    }

    const double fs = (wf.channelSamplingRateHz > 0.0) ? wf.channelSamplingRateHz : wf.baseSamplingRateHz;
    const double dt = (fs > 0.0) ? (1.0 / fs) : 0.0;

    ofs << "index,time_s,raw,scaled\n";
    const size_t n = wf.raw.size();
    for (size_t i = 0; i < n; ++i) {
        ofs << i << "," << (dt * static_cast<double>(i)) << ","
            << wf.raw[i] << "," << wf.scaled[i] << "\n";
    }
    return true;
}
