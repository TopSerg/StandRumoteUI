#include "DbcSignalCache.h"
#include "SignalLogger.h"

#include <cassert>
#include <cmath>
#include <cstdint>

int main()
{
    DbcSignalCache& cache = DbcSignalCache::instance();
    assert(cache.initialized());
    assert(cache.setSelection({"MCU_ActualTorque"}, {}));

    uint8_t payload[8]{};
    packDbcSignal(payload, 1100, 7, 11);   // 1100 - 1024 = 76 Nm
    packDbcSignal(payload, 33768, 39, 16); // 33768 - 32768 = 1000 rpm
    packDbcSignal(payload, 1, 52, 1);
    packDbcSignal(payload, 1, 53, 1);

    DataModel model;
    cache.decodeSelectedRx(122, payload, 8, model);

    assert(model.dbcSignals.count("MCU_ActualTorque") == 1);
    assert(model.dbcSignals.count("MCU_ActualSpeed") == 1);
    assert(std::fabs(model.dbcSignals.at("MCU_ActualTorque").physical - 76.0) < 1e-9);
    assert(std::fabs(model.dbcSignals.at("MCU_ActualSpeed").physical - 1000.0) < 1e-9);
    assert(model.dbcSignals.at("MCU_ActTrqValid").physical == 1.0);
    assert(model.dbcSignals.at("MCU_ActualSpeedValid").physical == 1.0);

    // A short frame must never make the decoder read beyond the received DLC.
    assert(cache.setSelection({"ZVFlux"}, {}));
    cache.decodeSelectedRx(128, payload, 1, model);
    assert(model.dbcSignals.count("ZVFlux") == 0);

    // Selected TX samples are also exposed to the WebSocket Signals/logbook UI.
    assert(cache.setSelection({}, {"VCU_MCUDesiredTorque"}));
    uint8_t txPayload[8]{};
    packDbcSignal(txPayload, 1035, 7, 11); // 1035 - 1023 = 12 Nm
    SignalLogger::instance().captureSelectedPayload("TX", 70, txPayload, 8);
    const auto txSamples = SignalLogger::instance().selectedSamples();
    bool foundTorque = false;
    for (const LoggedSignalSample& sample : txSamples) {
        if (sample.signalName == "VCU_MCUDesiredTorque") {
            foundTorque = std::fabs(sample.physical - 12.0) < 1e-3;
        }
    }
    assert(foundTorque);

    // Commissioning safety acknowledgement frame 0x083 is present in the
    // catalog and uses the same Motorola bit ordering as the other MCU frames.
    assert(cache.setSelection({"IdCommandEcho", "IqCommandEcho", "CurrentCommandAgeMs", "McuSafetyFlags", "McuFaultReason"}, {}));
    uint8_t safetyPayload[8]{};
    packDbcSignal(safetyPayload, 32100, 7, 16);  //  - -? physical 10 A with offset -3200
    packDbcSignal(safetyPayload, 32050, 23, 16); // 5 A
    packDbcSignal(safetyPayload, 37, 39, 16);
    packDbcSignal(safetyPayload, 0x15, 55, 8);
    packDbcSignal(safetyPayload, 5, 63, 8);
    cache.decodeSelectedRx(131, safetyPayload, 8, model);
    assert(std::fabs(model.dbcSignals.at("IdCommandEcho").physical - 10.0) < 1e-6);
    assert(std::fabs(model.dbcSignals.at("IqCommandEcho").physical - 5.0) < 1e-6);
}
