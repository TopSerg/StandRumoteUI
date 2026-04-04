//Stand_Marathon/src/main.cpp
#include <iostream>
#include "DataModel.h"
#include "ConfigManager.h"
#include "CANInterface.h"
#include "StateMachine.h"
#include "CommandSender.h"
#include "canDBCtask.h"
#include "DbcDispatcher.h"

CANInterface can;

static uint8_t send(dbc_can_tx_message_type* message){
    bool ok =  true;//can.send(message->message_id, message->data, message->dlc);
    std::cout << "message id: " << message->message_id << std::endl;
    for (int i = 0; i < message->dlc; i++)
    {
        std::cout << message->data[i] << std::endl;
    }
    
    return ok ? CAN_TX_STATUS_SUCCESS_DEFAULT : CAN_TX_STATUS_NO_MAILBOX_DEFAULT;
}

int main() {
    can.init(can.BUS1, 0, 123);
    DataModel model;
    ConfigManager config(model.configFilePath);
    StateMachine sm(model, can, config);

    config.load(model); // Загружаем конфиг при старте
    sm.setState(State::Init);

    dbc_init(&ReceiveDispatcher, &SendDispatcher, &send);

    while (true) {
        sm.update();
        // Симуляция команд
        char cmd;
        std::cout << "Command (r=read2, s=save, q=quit): ";
        std::cin >> cmd;
        if (cmd == 'r') sm.setState(State::Read2);
        if (cmd == 'c') CommandSender::sendControlCommand(can, model);
        if (cmd == 'l') CommandSender::sendLimitCommand(can, model);
        if (cmd == 't') CommandSender::sendTorqueCommand(can, model);
        if (cmd == 'q') break;
        dbc_process();  
    }

    sm.setState(State::Stop);
    sm.update();

    return 0;
}
