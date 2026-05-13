//Stand_Marathon/src/StateMachine.cpp
#include "StateMachine.h"
#include "CommandSender.h"
#include <iostream>
#include <iomanip>

StateMachine::StateMachine(DataModel& model, CANInterface& can, ConfigManager& cfg)
    : data(model), canInterface(can), config(cfg) {}

void StateMachine::setState(State newState) {
    currentState = newState;
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

    // Не слать ничего в Stop/Idle
    if (currentState == State::Stop || currentState == State::Idle) return;

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
    if (currentState != State::Idle){
        periodicTx();
    }

    if (std::getenv("WS_LOG_CAN")){
        std::cout << "En_is: " << data.En_Is << "   En_rem: " << data.En_rem << "   Kl_15: " << data.Kl_15 << std::endl;
    }

    // ДАЛЕЕ — обработка текущего состояния
    switch (currentState) {
        case State::Idle:     handleIdle(); break;
        case State::Init:     handleInit(); break;
        case State::Read2:    handleRead2(); break;
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



void StateMachine::handleIdle() {
    //  просто ждём
    std::cout << "[STATE] Idle\n";
}

void StateMachine::handleInit() {
    std::cout << "[STATE] Init\n";
    std::cout << "CAN Initialized\n";
    setState(State::Read2);
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
    std::cout << canInterface.receive(msg) << std::endl;
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
