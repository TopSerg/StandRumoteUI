//Stand_Marathon/src/StateMachine.cpp
#include "StateMachine.h"
#include "CommandSender.h"
#include "DbcSignalCache.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <cmath>

StateMachine::StateMachine(DataModel& model, CANInterface& can, ConfigManager& cfg)
    : data(model), canInterface(can), config(cfg) {}

void StateMachine::setState(State newState) {
    const State previousState = currentState.load();
    if ((previousState == State::ResolverRxInit || previousState == State::ResolverRx) &&
        newState != State::ResolverRxInit && newState != State::ResolverRx &&
        resolverCalibration_.active()) {
        stopResolverAutoCalibration("left_rx_only");
    }
    if (newState == State::ResolverRxInit || newState == State::ResolverRx) {
        canInterface.setTransmitEnabled(false);
    } else if (newState == State::Read2) {
        canInterface.setTransmitEnabled(true);
    } else {
        canInterface.setTransmitEnabled(false);
    }
    currentState.store(newState);
}

bool StateMachine::isRxOnly() const {
    const State state = currentState.load();
    return state == State::ResolverRxInit || state == State::ResolverRx;
}

const char* StateMachine::stateName() const {
    switch (currentState.load()) {
        case State::Idle: return "idle";
        case State::Init: return "init";
        case State::Read2: return "normal";
        case State::ResolverRxInit: return "resolver_rx_init";
        case State::ResolverRx: return "resolver_rx";
        case State::Stop: return "stop";
        case State::Save_Cfg: return "save_cfg";
        case State::Read_Cfg: return "read_cfg";
    }
    return "unknown";
}

// ТОЛЬКО тут решаем, когда слать сообщения
void StateMachine::periodicTx() {
    using namespace std::chrono;

    const auto now = clock::now();

    auto tp = system_clock::now();                // time_point
    auto s  = floor<seconds>(tp);                 // округляем вниз до секунд (C++17/20)
    std::time_t t = system_clock::to_time_t(s);   // в time_t (UTC-основан)
    std::cout << std::put_time(std::gmtime(&t), "%F %T") << "Z\n";   // 2025-10-15 12:34:56Z

    data.Brake_active = false;
    data.Kl_15 = true;
    data.TCS_active = false;

    // data.MCU_RequestedState = 1;
    // data.GearCtrl = 4;

    // Передача разрешена только в обычном режиме управления.
    if (currentState.load() != State::Read2) return;

    // 0x046 (control) — каждые 20 мс
    if (now - t_ctrl_ >= PERIOD_CTRL) {
        //  посылать управление только при включенном зажигании
        CommandSender::sendControlCommand(canInterface, data); // 0x046
        t_ctrl_ = now;
    }

    // 0x047 (limits) — каждые 100 мс
    if (now - t_limit_ >= PERIOD_LIMIT) {
        CommandSender::sendLimitCommand(canInterface, data);        // 0x047
        t_limit_ = now;
    }

    // 0x300 (Id/Iq команда) — каждые 10 мс
    if (now - t_curr_ >= PERIOD_CURR) {
        CommandSender::sendTorqueCommand(canInterface, data);   // 0x300
        t_curr_ = now;
    }
}

void StateMachine::update() {
    // СНАЧАЛА — периодические отправки
    if (currentState.load() == State::Read2){
        periodicTx();
    }

    if (std::getenv("WS_LOG_CAN")){
        std::cout << "En_is: " << data.En_Is << "   En_rem: " << data.En_rem << "   Kl_15: " << data.Kl_15 << std::endl;
    }

    // ДАЛЕЕ — обработка текущего состояния
    switch (currentState.load()) {
        case State::Idle:     handleIdle(); break;
        case State::Init:     handleInit(); break;
        case State::Read2:    handleRead2(); break;
        case State::ResolverRxInit: handleResolverRxInit(); break;
        case State::ResolverRx: handleResolverRx(); break;
        case State::Stop:     handleStop(); break;
        case State::Save_Cfg: handleSaveCfg(); break;
        case State::Read_Cfg: handleReadCfg(); break;
    }

    if(data.ns > data.n_max){
        isOverSpeed = true;
    }

    if (isOverSpeed)
    {
        data.M_desired = 0;
        data.Isd = 0;
        data.Isq = 0;
    }

    if(data.ns == 0){
        isOverSpeed = false;
    }
    
}

void StateMachine::handleResolverRxInit() {
    std::cout << "[STATE] Resolver RX init (application TX is blocked)\n";
    canInterface.stop();
    canInterface.setTransmitEnabled(false);
    if (canInterface.init(data.canChannel, data.canBaud, data.canFlags)) {
        setState(State::ResolverRx);
    } else {
        std::cerr << "CAN RX-only init failed!\n";
        setState(State::Stop);
    }
}

CANMessage StateMachine::handleResolverRx() {
    CANMessage msg = handleRead2();
    updateResolverCalibration();
    return msg;
}

bool StateMachine::startResolverAutoCalibration(
    float gain,
    float tolerance,
    float maxStep,
    std::string& reason)
{
    if (currentState.load() != State::ResolverRx) {
        reason = "Start RX-only first";
        return false;
    }
    if (data.ResolverCalibrationStatusCount == 0) {
        reason = "No MCU calibration status frame 0x082; flash matching controller firmware";
        return false;
    }

    resolverCalibration_.start(data.ResolverThetaCorrection, gain, tolerance, maxStep);
    resolverCalibrationStatusCount_ = data.ResolverCalibrationStatusCount;
    t_cal_status_ = clock::now();
    t_cal_tx_ = clock::now() - PERIOD_CAL;
    reason = "started";
    sendResolverCalibrationCommand(true);
    return true;
}

void StateMachine::stopResolverAutoCalibration(const std::string& reason)
{
    if (resolverCalibration_.active()) {
        sendResolverCalibrationCommand(false);
    }
    resolverCalibration_.stop(reason);
    data.ResolverCalibrationEnableCommand = 0;
}

bool StateMachine::sendResolverCalibrationCommand(bool enable)
{
    constexpr uint32_t kCommandId = 0x301;
    constexpr float kOffset = -3.2768f;
    constexpr float kFactor = 0.0001f;

    uint8_t payload[8] = {0};
    const float correction = std::clamp(resolverCalibration_.command(), -3.14159265f, 3.14159265f);
    const uint32_t rawCorrection = static_cast<uint32_t>(std::lround((correction - kOffset) / kFactor));
    const uint8_t sequence = resolverCalibrationSequence_++;

    packDbcSignal(payload, rawCorrection, 7, 16);
    packDbcSignal(payload, enable ? 1U : 0U, 23, 8);
    packDbcSignal(payload, sequence, 31, 8);
    packDbcSignal(payload, 0xCA1BU, 47, 16);

    data.ResolverThetaCorrectionCommand = correction;
    data.ResolverCalibrationEnableCommand = enable ? 1 : 0;
    data.ResolverCalibrationCommandSequence = sequence;
    const bool sent = canInterface.sendResolverCalibration(kCommandId, payload, 8);
    if (sent) t_cal_tx_ = clock::now();
    return sent;
}

void StateMachine::updateResolverCalibration()
{
    if (!resolverCalibration_.active()) return;

    const auto now = clock::now();
    if (data.ResolverCalibrationStatusCount != resolverCalibrationStatusCount_) {
        resolverCalibrationStatusCount_ = data.ResolverCalibrationStatusCount;
        t_cal_status_ = now;
        const bool fluxValid = (data.ResolverCalibrationStatus & 0x01U) != 0;
        resolverCalibration_.processSample(data.FluxPositionError, fluxValid);
    }

    if (now - t_cal_status_ > CAL_STATUS_TIMEOUT) {
        stopResolverAutoCalibration("telemetry_timeout");
        return;
    }

    if (now - t_cal_tx_ >= PERIOD_CAL) {
        sendResolverCalibrationCommand(true);
    }
}



void StateMachine::handleIdle() {
    //  просто ждём
    std::cout << "[STATE] Idle\n";
}

void StateMachine::handleInit() {
    std::cout << "[STATE] Init\n";
    canInterface.stop();
    // инициализируем канал параметрами из DataModel (после загрузки INI)
    if (canInterface.init(data.canChannel, data.canBaud, data.canFlags)) { // корректнее, чем хардкод:contentReference[oaicite:1]{index=1}
        std::cout << "CAN Initialized\n";
        setState(State::Read2);
    } else {
        std::cerr << "CAN Init failed!\n";
        setState(State::Stop);
    }
}

CANMessage StateMachine::handleRead2() {
    CANMessage msg;
    while (canInterface.receive(msg)) {
        MarathonLogic::updateFromCAN(msg, data);
    }

    return msg;
}

void StateMachine::handleStop() {
    std::cout << "[STATE] Stop\n";
    canInterface.stop();
    std::cout << "CAN stopped\n";
    setState(State::Idle);
}

void StateMachine::handleSaveCfg() {
    std::cout << "[STATE] Save_Cfg\n";
    config.save(data);
    std::cout << "Config saved\n";
    setState(State::Idle);
}

void StateMachine::handleReadCfg() {
    std::cout << "[STATE] Read_Cfg\n";
    config.load(data);
    std::cout << "Config loaded\n";
    setState(State::Idle);
}
