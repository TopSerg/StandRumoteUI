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
#include <atomic>

// мои заголовки
#include "DataModel.h"
#include "StateMachine.h"
#include "CommandSender.h"
#include "ConfigManager.h"
#include "CANInterface.h"
#include "DbcSignalCache.h"
#include "SignalLogger.h"

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
static std::atomic<int> g_json_period_ms{500};

static bool env_enabled(const char* name)
{
    const char* env = std::getenv(name);
    if (!env) return false;
    std::string v(env);
    return v == "1" || v == "true" || v == "TRUE";
}

static void init_can_log()
{
    if (env_enabled("WS_LOG_CAN")) {
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

static void log_ws_tx(const std::string& data)
{
    if (!env_enabled("WS_LOG_JSON_TX") && !env_enabled("WS_LOG_TX")) return;
    std::cout << "[WS TX] " << data << std::endl;
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
    j["M_max"] = model.M_max;
    j["M_min"] = model.M_min;
    j["M_grad_max"] = model.M_grad_max;
    j["n_max"] = model.n_max;
    j["M_desired"] = model.M_desired;
    j["Kl_15"] = model.Kl_15;
    j["En_Is"] = model.En_Is;
    j["En_rem"] = model.En_rem;
    j["Brake_active"] = model.Brake_active;
    j["TCS_active"] = model.TCS_active;
    j["MotorCtrl"] = model.MotorCtrl;
    j["GearCtrl"] = model.GearCtrl;
    j["SurgeDamperState"] = model.SurgeDamperState;
    j["MCU_RequestedState"] = model.MCU_RequestedState;
    j["MCU_IGBTTempU"] = model.MCU_IGBTTempU;
    j["MCU_IGBTTempV"] = model.MCU_IGBTTempV;
    j["MCU_IGBTTempW"] = model.MCU_IGBTTempW;
    j["MCU_IGBTTempMax"] = model.MCU_IGBTTempMax;
    j["MCU_TempCurrStr"] = model.MCU_TempCurrStr;
    j["MCU_TempCurrCool"] = model.MCU_TempCurrCool;
    j["MCU_OfsAl"] = model.MCU_OfsAl;
    j["MCU_Isd"] = model.MCU_Isd;
    j["MCU_Isq"] = model.MCU_Isq;
    j["MCU_bDmpCActv"] = model.MCU_bDmpCActv;
    j["MCU_stGateDrv"] = model.MCU_stGateDrv;
    j["MCU_DmpCTrqCurr"] = model.MCU_DmpCTrqCurr;
    j["MCU_VCUWorkMode"] = model.MCU_VCUWorkMode;

    // ➕ Новые поля из MCU_CurrentVoltage (0x4F6)
    j["Ud"] = model.Ud;
    j["Uq"] = model.Uq;
    j["Id"] = model.Id;
    j["Iq"] = model.Iq;

    // ➕ Новые поля из MCU_FluxParams (0x4F7)
    j["Flux"] = model.ZVFlux;
    j["Temperature"] = model.ZVTemperature;
    j["Rs"] = model.ZVRs;
    j["motorRs"] = model.ZVRs;
    j["ZVFlux"] = model.ZVFlux;
    j["ZVTheta"] = model.ZVTheta;
    j["ZVTemperature"] = model.ZVTemperature;
    j["ZVRs"] = model.ZVRs;
    j["ZVTimeStamp"] = model.ZVTimeStamp;
    j["ZVThetaCorr"] = model.ZVThetaCorr;
    j["ResolverSine"] = model.ResolverSine;
    j["ResolverCosine"] = model.ResolverCosine;
    j["ResolverAmplitude"] = model.ResolverAmplitude;
    j["ResolverTheta"] = model.ResolverTheta;
    j["ResolverThetaCorr"] = model.ResolverThetaCorr;
    j["FluxPositionError"] = model.FluxPositionError;
    j["ResolverThetaCorrection"] = model.ResolverThetaCorrection;
    j["ResolverElectricalSpeed"] = model.ResolverElectricalSpeed;
    j["ResolverCalibrationStatus"] = model.ResolverCalibrationStatus;
    j["ResolverCalibrationAckSequence"] = model.ResolverCalibrationAckSequence;
    j["ResolverCalibrationCommand"] = sm.resolverCalibrationCommand();
    j["ResolverCalibrationError"] = sm.resolverCalibrationError();
    j["ResolverCalibrationActive"] = sm.resolverCalibrationActive();
    j["ResolverCalibrationConverged"] = sm.resolverCalibrationConverged();
    j["ResolverCalibrationState"] = sm.resolverCalibrationStatus();
    j["can_mode"] = sm.stateName();
    j["can_rx_only"] = sm.isRxOnly();
    j["json_period_ms"] = g_json_period_ms.load();
    j["dbc_signals"] = json::array();
    DbcSignalCache& dbcCache = DbcSignalCache::instance();
    for (const auto& pair : model.dbcSignals) {
        const DbcRuntimeSignalValue& value = pair.second;
        if (!dbcCache.isRxSelected(value.signalName)) {
            continue;
        }
        j["dbc_signals"].push_back({
            {"direction", "RX"},
            {"message_id", value.messageId},
            {"message_name", value.messageName},
            {"signal_name", value.signalName},
            {"raw", value.raw},
            {"physical", value.physical},
        });
    }
    for (const LoggedSignalSample& sample : SignalLogger::instance().selectedSamples()) {
        if (sample.direction == "RX") {
            continue;
        }
        j["dbc_signals"].push_back({
            {"direction", sample.direction},
            {"message_id", sample.messageId},
            {"message_name", sample.messageName},
            {"signal_name", sample.signalName},
            {"raw", sample.raw},
            {"physical", sample.physical},
        });
    }

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

static json signal_catalog_json()
{
    json response;
    response["type"] = "signal_catalog";
    response["signals"] = json::array();
    for (const DbcSignalSelectionEntry& entry : DbcSignalCache::instance().selectionCatalog()) {
        response["signals"].push_back({
            {"direction", entry.tx ? "tx" : "rx"},
            {"selected", entry.selected},
            {"command", entry.commandName},
            {"message_id", entry.def.messageId},
            {"message_name", entry.def.messageName},
            {"signal_name", entry.def.signalName},
            {"start_bit", entry.def.startBit},
            {"length", entry.def.length},
            {"factor", entry.def.factor},
            {"offset", entry.def.offset},
        });
    }
    return response;
}

void handleCommand(const json& j, std::vector<json>& responses) {
    std::string cmd = j.value("cmd", "");
    if (cmd == "Init") {
        sm.setState(State::Init);
    } else if (cmd == "StartResolverCalibration" || cmd == "ResolverRx") {
        sm.setState(State::ResolverRxInit);
    } else if (cmd == "StartResolverAutoCalibration") {
        const float gain = j.value("gain", 0.2f);
        const float tolerance = j.value("tolerance", 0.01f);
        const float maxStep = j.value("max_step", 0.02f);
        std::string reason;
        const bool ok = sm.startResolverAutoCalibration(gain, tolerance, maxStep, reason);
        responses.push_back({
            {"type", ok ? "resolver_calibration_started" : "command_rejected"},
            {"cmd", cmd},
            {"reason", reason}
        });
    } else if (cmd == "StopResolverAutoCalibration") {
        sm.stopResolverAutoCalibration();
        responses.push_back({
            {"type", "resolver_calibration_stopped"},
            {"cmd", cmd}
        });
    } else if (cmd == "Stop") {
        sm.setState(State::Stop);
    } else if (cmd == "Read2") {
        sm.setState(State::Read2);
    } else if (cmd == "SaveCfg") {
        sm.setState(State::Save_Cfg);
    } else if (cmd == "SendControl") {
        if (!can.isTransmitEnabled()) {
            responses.push_back({
                {"type", "command_rejected"},
                {"cmd", cmd},
                {"reason", "CAN application TX is not enabled in the current mode"}
            });
            return;
        }
        apply_control_fields(j);
        CommandSender::sendControlCommand(can, model);
    } else if (cmd == "SendLimits") {
        if (!can.isTransmitEnabled()) {
            responses.push_back({
                {"type", "command_rejected"},
                {"cmd", cmd},
                {"reason", "CAN application TX is not enabled in the current mode"}
            });
            return;
        }
        apply_limit_fields(j);
        CommandSender::sendLimitCommand(can, model);
    } else if (cmd == "SendTorque") {
        if (!can.isTransmitEnabled()) {
            responses.push_back({
                {"type", "command_rejected"},
                {"cmd", cmd},
                {"reason", "CAN application TX is not enabled in the current mode"}
            });
            return;
        }
        apply_torque_fields(j);
        CommandSender::sendTorqueCommand(can, model);
    } else if (cmd == "SetJsonPeriod") {
        int period_ms = j.value("period_ms", 500);
        if (period_ms < 1) {
            period_ms = 1;
        }
        g_json_period_ms.store(period_ms);
        std::cout << "[WS] JSON period set to " << period_ms << " ms" << std::endl;
    } else if (cmd == "GetSignalCatalog") {
        responses.push_back(signal_catalog_json());
    } else if (cmd == "SetSignalSelection") {
        std::vector<std::string> rx;
        std::vector<std::string> tx;
        if (j.contains("rx") && j["rx"].is_array()) {
            rx = j["rx"].get<std::vector<std::string>>();
        }
        if (j.contains("tx") && j["tx"].is_array()) {
            tx = j["tx"].get<std::vector<std::string>>();
        }
        DbcSignalCache::instance().setSelection(rx, tx);
        responses.push_back(signal_catalog_json());
    } else {
        std::cerr << "[Warn] Unknown command: " << cmd << std::endl;
    }
}


using clock1 = std::chrono::steady_clock;

// Модифицируем функцию do_session для отправки CAN-сообщений
void do_session(tcp::socket socket) {
    try {
        auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
        ws->accept();
        std::cout << "[WS] Client connected" << std::endl;

        std::atomic<bool> running{true};
        auto ws_write_mutex = std::make_shared<std::mutex>();

        {
            const std::string payload = signal_catalog_json().dump();
            std::lock_guard<std::mutex> writeLock(*ws_write_mutex);
            ws->write(boost::asio::buffer(payload));
        }

        std::thread updater([ws, ws_write_mutex, &running]() {
            auto last_json_send = clock1::now();
            while (running) {
                try {
                    sm.update();
                    const auto now1 = clock1::now();
                    const auto period = std::chrono::milliseconds(g_json_period_ms.load());
                    if(now1 - last_json_send >= period){
                        std::string data = serializeData();
                        log_ws_tx(data);
                        std::lock_guard<std::mutex> writeLock(*ws_write_mutex);
                        ws->write(boost::asio::buffer(data));
                        last_json_send = now1;
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

                std::vector<json> responses;
                handleCommand(j, responses);
                for (const json& response : responses) {
                    const std::string payload = response.dump();
                    std::lock_guard<std::mutex> writeLock(*ws_write_mutex);
                    ws->write(boost::asio::buffer(payload));
                }

                log_can_json(jei);
            }
        } catch (const std::exception& e) {
            std::cerr << "[Reader error] " << e.what() << std::endl;
        }

        running = false;
        if (updater.joinable()) updater.join();
        sm.setState(State::Stop);
        can.stop();
        std::cout << "[WS] Client disconnected, CAN commands stopped" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Session error] " << e.what() << std::endl;
        sm.setState(State::Stop);
        can.stop();
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
