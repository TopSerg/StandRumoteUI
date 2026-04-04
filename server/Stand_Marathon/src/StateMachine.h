//Stand_Marathon/src/StateMachine.h
#pragma once
#include <cstdint>
#include <chrono>
#include "DataModel.h"
#include "CANInterface.h"
#include "ConfigManager.h"
#include "MarathonLogic.h"

enum class State { Idle, Init, Read2, Stop, Save_Cfg, Read_Cfg };

class StateMachine {
public:
    StateMachine(DataModel& model, CANInterface& can, ConfigManager& cfg);

    void setState(State newState);
    void update(); // вызывать часто (каждые 1–5 мс)

private:
    DataModel& data;
    CANInterface& canInterface;
    ConfigManager& config;
    State currentState = State::Idle;

    bool isOverSpeed = false;

    using clock = std::chrono::steady_clock;
    clock::time_point t0_ = clock::now();
    clock::time_point t_ctrl_  = t0_;
    clock::time_point t_limit_ = t0_;
    clock::time_point t_curr_  = t0_;

    // Периоды сообщений
    static constexpr std::chrono::milliseconds PERIOD_CTRL  {10};   // 0x046
    static constexpr std::chrono::milliseconds PERIOD_LIMIT {20};  // 0x047
    static constexpr std::chrono::milliseconds PERIOD_CURR  {10};   // 0x300

    // Хелперы по состояниям
    void handleIdle();
    void handleInit();
    CANMessage handleRead2();
    void handleStop();
    void handleSaveCfg();
    void handleReadCfg();

    // Периодические отправки
    void periodicTx();
};
