//Stand_Marathon/src/StateMachine.h
#pragma once
#include <cstdint>
#include <chrono>
#include <atomic>
#include <string>
#include "DataModel.h"
#include "CANInterface.h"
#include "ConfigManager.h"
#include "MarathonLogic.h"
#include "ResolverCalibrationController.h"

enum class State { Idle, Init, Read2, ResolverRxInit, ResolverRx, Stop, Save_Cfg, Read_Cfg };

class StateMachine {
public:
    StateMachine(DataModel& model, CANInterface& can, ConfigManager& cfg);

    void setState(State newState);
    void update(); // вызывать часто (каждые 1–5 мс)
    bool isRxOnly() const;
    const char* stateName() const;
    bool startResolverAutoCalibration(float gain, float tolerance, float maxStep, std::string& reason);
    void stopResolverAutoCalibration(const std::string& reason = "stopped");
    bool resolverCalibrationActive() const { return resolverCalibration_.active(); }
    bool resolverCalibrationConverged() const { return resolverCalibration_.converged(); }
    float resolverCalibrationCommand() const { return resolverCalibration_.command(); }
    float resolverCalibrationError() const { return resolverCalibration_.lastError(); }
    const std::string& resolverCalibrationStatus() const { return resolverCalibration_.status(); }

private:
    DataModel& data;
    CANInterface& canInterface;
    ConfigManager& config;
    std::atomic<State> currentState{State::Idle};

    bool isOverSpeed = false;

    using clock = std::chrono::steady_clock;
    clock::time_point t0_ = clock::now();
    clock::time_point t_ctrl_  = t0_;
    clock::time_point t_limit_ = t0_;
    clock::time_point t_curr_  = t0_;
    clock::time_point t_cal_tx_ = t0_;
    clock::time_point t_cal_status_ = t0_;

    ResolverCalibrationController resolverCalibration_;
    uint64_t resolverCalibrationStatusCount_ = 0;
    uint8_t resolverCalibrationSequence_ = 0;

    // Периоды сообщений
    static constexpr std::chrono::milliseconds PERIOD_CTRL  {10};   // 0x046
    static constexpr std::chrono::milliseconds PERIOD_LIMIT {20};  // 0x047
    static constexpr std::chrono::milliseconds PERIOD_CURR  {10};   // 0x300
    static constexpr std::chrono::milliseconds PERIOD_CAL   {50};   // 0x301 keepalive
    static constexpr std::chrono::milliseconds CAL_STATUS_TIMEOUT {300};

    // Хелперы по состояниям
    void handleIdle();
    void handleInit();
    CANMessage handleRead2();
    void handleResolverRxInit();
    CANMessage handleResolverRx();
    void handleStop();
    void handleSaveCfg();
    void handleReadCfg();
    void updateResolverCalibration();
    bool sendResolverCalibrationCommand(bool enable);

    // Периодические отправки
    void periodicTx();
};
