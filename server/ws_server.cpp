// ws_server.cpp
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10 для Boost.Asio/Beast (убирает предупреждение)
#endif

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <type_traits>

// мои заголовки
#include "DataModel.h"
#include "StateMachine.h"
#include "CommandSender.h"
#include "ConfigManager.h"
#include "CANInterface.h"

#include <fstream>
#include <mutex>
#include <cstdlib>

// твой wrapper
#include "YokogawaDL850E.h"

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
using json = nlohmann::json;

DataModel     model;
CANInterface  can;
ConfigManager config(model.configFilePath);
StateMachine  sm(model, can, config);

// утилита: присвоить поле из JSON, если ключ есть
template<class T>
inline void set_if_present(const json& j, const char* key, T& target) {
    if (j.contains(key) && !j.at(key).is_null()) {
        target = j.at(key).get<T>();
    }
}

static bool         g_log_can   = false;
static std::mutex   g_log_mutex;
static std::ofstream g_log_file;

static void init_can_log()
{
    const char* env = std::getenv("WS_LOG_CAN");
    if (!env) return;

    std::string v(env);
    if (v == "1" || v == "true" || v == "TRUE") {
        g_log_can = true;
        g_log_file.open("ws_can_log.jsonl", std::ios::app);
        if (!g_log_file) {
            std::cerr << "[WS_LOG_JSON] cannot open ws_can_log.jsonl" << std::endl;
            g_log_can = false;
        } else {
            std::cerr << "[WS_LOG_JSON] logging CAN frames to ws_can_log.jsonl" << std::endl;
        }
    }
}

static void log_can_json(const json& j)
{
    if (!g_log_can) return;
    std::lock_guard<std::mutex> lk(g_log_mutex);
    if (!g_log_file.is_open()) return;
    g_log_file << j.dump() << std::endl;
}

// применить параметры управления (для "SendControl")
static void apply_control_fields(const json& j) {
    set_if_present(j, "MotorCtrl",    model.MotorCtrl);
    set_if_present(j, "GearCtrl",     model.GearCtrl);
    set_if_present(j, "Kl_15",        model.Kl_15);
    set_if_present(j, "Brake_active", model.Brake_active);
    set_if_present(j, "TCS_active",   model.TCS_active);

    // ✳ ДОБАВЛЕНО:
    set_if_present(j, "En_Is",        model.En_Is);

    // Если вы используете отдельный сетпоинт скорости — лучше model.ns_setpoint.
    // Если его нет — временно кладём в model.ns (но это смешивает измерение и задание):
    set_if_present(j, "ns",           model.M_desired);

    //set_if_present(j, "M_desired",           model.M_desired);
}


// применить лимиты (для "SendLimits")
static void apply_limit_fields(const json& j) {
    set_if_present(j, "M_max",       model.M_max);
    set_if_present(j, "M_min",       model.M_min);
    set_if_present(j, "M_grad_max",  model.M_grad_max);
    set_if_present(j, "n_max",       model.n_max);
}

// применить Id/Iq и прочее удалённое управление (для "SendTorque")
static void apply_torque_fields(const json& j) {
    set_if_present(j, "En_Is", model.En_Is);
    set_if_present(j, "Isd",    model.Isd);
    set_if_present(j, "Isq",    model.Isq);
}

// Сериализация DataModel в JSON
std::string serializeData() {
    json j;
    j["Ms"] = model.Ms;
    j["ns"] = model.ns;
    j["Idc"] = model.Idc;
    j["Isd"] = model.Isd;
    j["Isq"] = model.Isq;
    j["Udc"] = model.Udc;
    j["Kl_15"] = model.Kl_15;
    j["Brake_active"] = model.Brake_active;
    j["TCS_active"] = model.TCS_active;
    j["MCU_IGBTTempU"] = model.MCU_IGBTTempU;
    j["MCU_TempCurrStr"] = model.MCU_TempCurrStr;

    // ➕ Новые поля из MCU_CurrentVoltage (0x4F6)
    j["Ud"] = model.Ud;
    j["Uq"] = model.Uq;
    j["Id"] = model.Id;
    j["Iq"] = model.Iq;

    // ➕ Новые поля из MCU_FluxParams (0x4F7)
    j["ZVFlux"] = model.ZVFlux;
    j["ZVTheta"] = model.ZVTheta;
    j["ZVTemperature"] = model.ZVTemperature;
    j["ZVRs"] = model.ZVRs;
    j["ZVTimeStamp"] = model.ZVTimeStamp;
    j["ZVThetaCorr"] = model.ZVThetaCorr;

    return j.dump();
}


// Добавим в серверный код функцию для отправки CAN-сообщений
void sendCANFrame(websocket::stream<tcp::socket>& ws, const std::string& direction, 
                 uint32_t id, const std::vector<uint8_t>& data, uint8_t len, 
                 uint32_t flags, double timestamp) {
    json can_msg;
    can_msg["type"] = "can_frame";
    can_msg["direction"] = direction;
    can_msg["id"] = id;
    
    for (size_t i = 0; i < data.size(); ++i) {
        can_msg["data" + std::to_string(i)] = data[i];
    }
    
    can_msg["len"] = len;
    can_msg["flags"] = flags;
    can_msg["ts"] = timestamp;
    
    try {
        ws.write(boost::asio::buffer(can_msg.dump()));
    } catch (std::exception const& e) {
        std::cerr << "[Error] Failed to send CAN frame: " << e.what() << std::endl;
    }
}

void handleCommand(const json& j) {
    std::string cmd = j.value("cmd", "");


    apply_torque_fields(j);
    apply_limit_fields(j);
    apply_control_fields(j);
    if (cmd == "Init") {
        sm.setState(State::Init);
    } else if (cmd == "Stop") {
        sm.setState(State::Stop);
    } else if (cmd == "Read2") {
        sm.setState(State::Read2);
    } else if (cmd == "SaveCfg") {
        sm.setState(State::Save_Cfg);
    } else if (cmd == "SendControl") {
        apply_control_fields(j);
        CommandSender::sendControlCommand(can, model);
    } else if (cmd == "SendLimits") {
        apply_limit_fields(j);
        CommandSender::sendLimitCommand(can, model);
    } else if (cmd == "SendTorque") {
        std::cout << "ABOBA" << std::endl;
        apply_torque_fields(j);
        CommandSender::sendTorqueCommand(can, model);
    } else {
        std::cerr << "[Warn] Unknown command: " << cmd << std::endl;
    }
}


using clock1 = std::chrono::steady_clock;
clock1::time_point t01 = clock1::now();
static constexpr std::chrono::milliseconds PERIOD_CTRL1  {500};   // 0x046

// Модифицируем функцию do_session для отправки CAN-сообщений
void do_session(tcp::socket socket) {
    try {
        auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
        ws->accept();
        std::cout << "[WS] Client connected" << std::endl;

        std::atomic<bool> running{true};

        std::thread updater([ws, &running]() {
            while (running) {
                try {
                    sm.update();
                    const auto now1 = clock1::now();
                    if(now1 - t01 > PERIOD_CTRL1){
                        std::string data = serializeData();
                        ws->write(boost::asio::buffer(data));
                        t01 = now1;
                    }

                    /*std::vector<uint8_t> can_data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
                    sendCANFrame(*ws, "rx", 0x123, can_data, can_data.size(), 0,
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0);*/

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                } catch (const std::exception& e) {
                    std::cerr << "[Updater error] " << e.what() << std::endl;
                    break;
                }
            }
        });

        try {
            for (;;) {
                boost::beast::flat_buffer buffer;
                ws->read(buffer);
                std::string msg = boost::beast::buffers_to_string(buffer.data());
                auto j = json::parse(msg);

                const json jei = j;

                handleCommand(j);

                log_can_json(jei);
            }
        } catch (const std::exception& e) {
            std::cerr << "[Reader error] " << e.what() << std::endl;
        }

        running = false;
        if (updater.joinable()) updater.join();

    } catch (const std::exception& e) {
        std::cerr << "[Session error] " << e.what() << std::endl;
    }
}


int main() {
    // Загрузка конфигурации и перевод в исходное состояние
    config.load(model);
    sm.setState(State::Idle);

    init_can_log();

    try {
        boost::asio::io_context ioc;
        tcp::acceptor acceptor{ioc, {tcp::v4(), 9000}};
        std::cout << "WebSocket server running at ws://0.0.0.0:9000" << std::endl;

        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(&do_session, std::move(socket)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << std::endl;
    }
}