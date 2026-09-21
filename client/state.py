# state.py
import tkinter as tk
from collections import deque

# ---------- Глобальные настройки UI ----------
APP_FONT = ("Segoe UI", 10)
MONO_FONT = ("Cascadia Mono", 9)  # или "Consolas"
PAD = 8

# ---------- Константы и алиасы телеметрии ----------
# Канонические имена совпадают с JSON-ключами ws_server.cpp.
TELEM_COLUMNS = [
    "ts",
    "ns", "Ms",
    "Udc", "Idc", "MCU_Isd", "MCU_Isq",
    "Ud", "Uq", "Id", "Iq",
    "IdCommandEcho", "IqCommandEcho", "CurrentCommandAgeMs",
    "PwmEnabled", "PiSaturation", "CurrentCommandEnabled",
    "CurrentCommandWatchdogExpired", "FaultReason", "Torque_meter",
    "Flux", "Theta", "ThetaCorr", "Temperature", "Rs", "TimeStamp",
    "MCU_IGBTTempU", "MCU_IGBTTempV", "MCU_IGBTTempW", "MCU_IGBTTempMax",
    "MCU_TempCurrStr", "MCU_TempCurrStr1", "MCU_TempCurrStr2", "MCU_TempCurrCool",
    "M_desired", "M_max", "M_min", "M_grad_max", "n_max",
    "Isd", "Isq", "Kl_15", "En_Is", "En_rem", "Brake_active", "TCS_active",
    "MotorCtrl", "GearCtrl", "SurgeDamperState", "MCU_RequestedState",
    "MCU_ActualTorqueValid", "MCU_ActualSpeedValid", "MCU_MessageCounter7A",
    "MCU_OfsAl", "MCU_bDmpCActv",
    "MCU_stGateDrv", "MCU_DmpCTrqCurr", "MCU_VCUWorkMode", "MCU_SW_ver",
    "ResolverSine", "ResolverCosine", "ResolverAmplitude",
    "ResolverTheta", "ResolverThetaCorr", "FluxPositionError",
    "ResolverThetaCorrection", "ResolverElectricalSpeed",
    "ResolverCalibrationStatus", "ResolverCalibrationAckSequence",
    "ResolverCalibrationCommand", "ResolverCalibrationError",
    "ResolverCalibrationState", "CanMode",
]

# key -> (понятное имя, единица, число знаков после запятой).
# Этот словарь — единственный источник названий в UI.
FIELD_SPECS = {
    "ns": ("Motor speed", "rpm", 0),
    "Ms": ("Actual torque", "N·m", 1),
    "Udc": ("DC-link voltage", "V", 1),
    "Idc": ("Motor current (MCU_IsCurr)", "A", 1),
    "MCU_Isd": ("Measured d-axis current", "A", 1),
    "MCU_Isq": ("Measured q-axis current", "A", 1),
    "Ud": ("d-axis voltage Ud", "V", 2),
    "Uq": ("q-axis voltage Uq", "V", 2),
    "Id": ("Id_measured (FOC)", "A", 2),
    "Iq": ("Iq_measured (FOC)", "A", 2),
    "IdCommandEcho": ("Id_cmd (MCU echo)", "A", 2),
    "IqCommandEcho": ("Iq_cmd (MCU echo)", "A", 2),
    "CurrentCommandAgeMs": ("Current command age", "ms", 0),
    "PwmEnabled": ("PWM enabled", "", None),
    "PiSaturation": ("PI saturation", "", None),
    "CurrentCommandEnabled": ("Current command enabled", "", None),
    "CurrentCommandWatchdogExpired": ("Current command watchdog", "", None),
    "FaultReason": ("Fault reason", "", None),
    "Flux": ("Estimated flux", "", 4),
    "Theta": ("Electrical angle", "rad", 4),
    "ThetaCorr": ("Corrected electrical angle", "rad", 4),
    "Temperature": ("Estimated motor temperature", "°C", 1),
    "Rs": ("Estimated phase resistance", "Ω", 5),
    "TimeStamp": ("MCU sample counter", "", 0),
    "MCU_IGBTTempU": ("IGBT temperature U", "°C", 1),
    "MCU_IGBTTempV": ("IGBT temperature V", "°C", 1),
    "MCU_IGBTTempW": ("IGBT temperature W", "°C", 1),
    "MCU_IGBTTempMax": ("IGBT temperature max", "°C", 1),
    "MCU_TempCurrStr": ("Motor/stator temperature", "°C", 1),
    "MCU_TempCurrStr1": ("Stator temperature sensor 1", "°C", 1),
    "MCU_TempCurrStr2": ("Stator temperature sensor 2", "°C", 1),
    "MCU_TempCurrCool": ("Coolant/heatsink temperature", "°C", 1),
    "M_desired": ("Active torque/speed setpoint", "mode dependent", 2),
    "Isd": ("Commanded d-axis current", "A", 2),
    "Isq": ("Commanded q-axis current", "A", 2),
    "M_min": ("Minimum torque limit", "N·m", 1),
    "M_max": ("Maximum torque limit", "N·m", 1),
    "M_grad_max": ("Maximum torque gradient", "N·m/s", 0),
    "n_max": ("Maximum speed", "rpm", 0),
    "Kl_15": ("KL15 enabled", "", None),
    "En_Is": ("Current-command mode", "", None),
    "En_rem": ("Remote control enabled", "", None),
    "Brake_active": ("Brake pedal active", "", None),
    "TCS_active": ("TCS active", "", None),
    "MotorCtrl": ("Requested MCU mode", "code", 0),
    "GearCtrl": ("Requested gear", "code", 0),
    "SurgeDamperState": ("Surge damper state", "code", 0),
    "MCU_RequestedState": ("MCU requested state", "code", 0),
    "MCU_OfsAl": ("MCU offset angle", "deg", 3),
    "MCU_bDmpCActv": ("Damping control active", "", None),
    "MCU_stGateDrv": ("Gate-driver state", "code", 0),
    "MCU_DmpCTrqCurr": ("Damping torque", "N·m", 2),
    "MCU_VCUWorkMode": ("MCU work mode", "code", 0),
    "MCU_SW_ver": ("MCU software version", "", None),
    "MCU_ActualTorqueValid": ("Actual torque valid", "", None),
    "MCU_ActualSpeedValid": ("Actual speed valid", "", None),
    "MCU_MessageCounter7A": ("MCU status counter 0x7A", "", 0),
}

CONTROL_MONITOR_FIELDS = (
    "ns", "Ms", "Udc", "Idc", "MCU_Isd", "MCU_Isq",
    "IdCommandEcho", "IqCommandEcho", "CurrentCommandAgeMs", "PwmEnabled", "PiSaturation", "FaultReason",
    "MCU_IGBTTempMax", "MCU_TempCurrStr", "MCU_TempCurrCool",
    "M_min", "M_max", "n_max",
)
CURRENT_VOLTAGE_FIELDS = ("Ud", "Uq", "Id", "Iq")
FLUX_FIELDS = ("Flux", "Theta", "ThetaCorr", "Temperature", "Rs", "TimeStamp")

INDICATION_GROUPS = (
    ("Measured drive values", ("ns", "Ms", "Udc", "Idc", "MCU_Isd", "MCU_Isq")),
    ("FOC current and voltage", CURRENT_VOLTAGE_FIELDS),
    ("Safety / command acknowledgement", ("IdCommandEcho", "IqCommandEcho", "CurrentCommandAgeMs", "PwmEnabled", "PiSaturation", "CurrentCommandEnabled", "CurrentCommandWatchdogExpired", "FaultReason")),
    ("Flux estimator", FLUX_FIELDS),
    ("Temperatures", (
        "MCU_IGBTTempU", "MCU_IGBTTempV", "MCU_IGBTTempW", "MCU_IGBTTempMax",
        "MCU_TempCurrStr", "MCU_TempCurrStr1", "MCU_TempCurrStr2", "MCU_TempCurrCool",
    )),
    ("MCU status", (
        "MCU_OfsAl", "MCU_bDmpCActv", "MCU_stGateDrv", "MCU_DmpCTrqCurr",
        "MCU_VCUWorkMode", "MCU_ActualTorqueValid", "MCU_ActualSpeedValid",
        "MCU_MessageCounter7A", "MCU_SW_ver",
    )),
    ("Active commands and limits", (
        "M_desired", "Isd", "Isq", "M_min", "M_max", "M_grad_max", "n_max",
        "Kl_15", "En_Is", "En_rem", "Brake_active", "TCS_active",
        "MotorCtrl", "GearCtrl", "SurgeDamperState", "MCU_RequestedState",
    )),
)

# Параметры для онлайн-расчётов Ld/Lq
DEFAULT_RS_OHMS = 0.05       # Rs по умолчанию, если не приходит в телеметрии
DEFAULT_POLE_PAIRS = None    # Число пар полюсов, если не приходит (например, 4)

# Алиасы полей JSON на случай разных имён
FIELD_ALIASES = {
    "Ud": ["Ud", "u_d", "U_d"],
    "Uq": ["Uq", "u_q", "U_q"],
    "Id": ["Id", "i_d", "I_d"],
    "Iq": ["Iq", "i_q", "I_q"],
    "Theta": ["Theta", "ZVTheta"],
    "Flux": ["Flux", "ZVFlux", "E_back"],
    "Temperature": ["Temperature", "ZVThemperature", "ZVTemperature"],
    "Rs": ["Rs", "ZVRs"],
    "TimeStamp": ["TimeStamp", "ZVTimeStamp"],
    "ThetaCorr": ["ThetaCorr", "ZVThetaCorr"],
    "ResolverSine": ["ResolverSine"],
    "ResolverCosine": ["ResolverCosine"],
    "ResolverAmplitude": ["ResolverAmplitude"],
    "ResolverTheta": ["ResolverTheta"],
    "ResolverThetaCorr": ["ResolverThetaCorr"],
    "FluxPositionError": ["FluxPositionError", "ResolverCalibrationError"],
    "ResolverThetaCorrection": ["ResolverThetaCorrection", "ResolverCalibrationCommand"],
    "ResolverElectricalSpeed": ["ResolverElectricalSpeed"],
}

# Маппинг коробки передач (по DBC VcuActualGear)
GEAR_MAP = {"D": 4, "R": 3, "N": 2}
REV_GEAR_MAP = {v: k for k, v in GEAR_MAP.items()}
MOTOR_MODE_MAP = {
    "currents": 1,
    "speed": 4,
    "torque": 1,  # новый режим "torque" считаем токовым
}

# Сколько точек держим в буферах для графиков/карт
TREND_CAP = 3000


class AppState:
    """
    Единое хранилище состояния приложения:
    - tk.Variable (для привязки Entry/Scale/Label и т.п.)
    - буферы deque для трендов/карт
    - параметры логирования
    Ничего не знает о сети и GUI-компонентах (widgets) — только данные.
    """
    def __init__(self, root: tk.Misc):
        # --- Соединение/статус ---
        self.conn_var = tk.StringVar(master=root, value="disabled")
        self.conn_color = tk.StringVar(master=root, value="#d72c20")  # красный

        # --- Поля для “панели индикации” (создаются во view)
        self.entry_vars: dict[str, tk.StringVar] = {}

        # --- CAN поля (Rx/Tx), по 12 строк как в исходнике ---
        self.can_rx_data = [tk.StringVar(master=root) for _ in range(12)]
        self.can_tx_data = [tk.StringVar(master=root) for _ in range(12)]

        # --- Логбук телеметрии ---
        self.log_enabled = tk.BooleanVar(master=root, value=True)
        self.log_rows: list[dict] = []   # список словарей
        self.max_rows: int = 5000        # ограничение на длину буфера/таблицы
        self.signal_catalog: list[dict] = []
        self.selected_rx_signals: set[str] = set()
        self.selected_tx_signals: set[str] = set()
        self.latest_dbc_signals: list[dict] = []
        self.dynamic_log_columns: list[str] = []

        # --- Режим/передача и включения ---
        self.gear_var = tk.StringVar(master=root, value="N")       # "D" / "R" / "N"
        self.mode_var = tk.StringVar(master=root, value="currents")  # "currents" или "speed"
        self.En_Is_var = tk.IntVar(master=root, value=0)
        self.control_armed_var = tk.BooleanVar(master=root, value=False)
        self.control_arm_status_var = tk.StringVar(master=root, value="DISARMED")
        self.torque_meter_var = tk.StringVar(master=root, value="")

        # --- Параметры управления / лимиты (строки как в исходнике) ---
        self.Id_var = tk.StringVar(master=root, value="0.0")
        self.Iq_var = tk.StringVar(master=root, value="0.0")
        self.M_min_var = tk.StringVar(master=root, value="-50.0")
        self.M_max_var = tk.StringVar(master=root, value="400.0")
        self.M_grad_max_var = tk.StringVar(master=root, value="50")
        self.n_max_var = tk.StringVar(master=root, value="500")

        # --- Скалярные значения для индикации/управления ---
        self.speed_var = tk.DoubleVar(master=root, value=0.0)   # ns (rpm)
        self.torque_var = tk.DoubleVar(master=root, value=0.0)  # Ms (N·m)

        # --- Auto calibration (автоматическая калибровка) ---
        self.auto_delay_s_var = tk.DoubleVar(master=root, value=0.5)  # задержка между точками, сек
        self.auto_status_var = tk.StringVar(master=root, value="idle")
        self.auto_points_var = tk.StringVar(master=root, value="0")   # сколько точек загружено
        self.json_period_ms_var = tk.StringVar(master=root, value="500")
        self.resolver_mode_var = tk.StringVar(master=root, value="STOPPED")
        self.resolver_sine_var = tk.StringVar(master=root, value="—")
        self.resolver_cosine_var = tk.StringVar(master=root, value="—")
        self.resolver_amplitude_var = tk.StringVar(master=root, value="—")
        self.resolver_theta_var = tk.StringVar(master=root, value="—")
        self.resolver_theta_corr_var = tk.StringVar(master=root, value="—")
        self.resolver_capture_count_var = tk.StringVar(master=root, value="0")
        self.resolver_sine_offset_var = tk.StringVar(master=root, value="—")
        self.resolver_cosine_offset_var = tk.StringVar(master=root, value="—")
        self.resolver_sine_amplitude_var = tk.StringVar(master=root, value="—")
        self.resolver_cosine_amplitude_var = tk.StringVar(master=root, value="—")
        self.resolver_gain_ratio_var = tk.StringVar(master=root, value="—")
        self.resolver_flux_error_var = tk.StringVar(master=root, value="—")
        self.resolver_theta_correction_var = tk.StringVar(master=root, value="—")
        self.resolver_electrical_speed_var = tk.StringVar(master=root, value="—")
        self.resolver_auto_state_var = tk.StringVar(master=root, value="idle")
        self.resolver_flux_valid_var = tk.StringVar(master=root, value="no")
        self.resolver_command_active_var = tk.StringVar(master=root, value="no")
        self.resolver_ack_sequence_var = tk.StringVar(master=root, value="—")
        self.resolver_command_var = tk.StringVar(master=root, value="—")
        self.resolver_loop_error_var = tk.StringVar(master=root, value="—")
        self.resolver_status_count_var = tk.StringVar(master=root, value="0")
        self.resolver_command_sequence_var = tk.StringVar(master=root, value="—")
        self.resolver_enable_command_var = tk.StringVar(master=root, value="no")
        self.resolver_active_var = tk.StringVar(master=root, value="no")
        self.resolver_converged_var = tk.StringVar(master=root, value="no")
        self.resolver_auto_gain_var = tk.StringVar(master=root, value="0.20")
        self.resolver_auto_tolerance_var = tk.StringVar(master=root, value="0.010")
        self.resolver_auto_max_step_var = tk.StringVar(master=root, value="0.020")
        self.resolver_sine_min = None
        self.resolver_sine_max = None
        self.resolver_cosine_min = None
        self.resolver_cosine_max = None
        self.resolver_samples = deque(maxlen=TREND_CAP * 4)
        self.resolver_last_sample_count = 0
        self.resolver_last_unwrapped_theta = None
        self.resolver_start_marker = None
        self.resolver_end_marker = None
        self.resolver_unwrapped_theta_var = tk.StringVar(master=root, value="—")
        self.resolver_cycle_count_var = tk.StringVar(master=root, value="—")
        self.resolver_marker_status_var = tk.StringVar(master=root, value="no markers")
        self.resolver_ellipse_center_var = tk.StringVar(master=root, value="—")
        self.resolver_ellipse_axes_var = tk.StringVar(master=root, value="—")
        self.resolver_ellipse_rotation_var = tk.StringVar(master=root, value="—")
        self.resolver_nonorthogonality_var = tk.StringVar(master=root, value="—")
        self.resolver_fit_status_var = tk.StringVar(master=root, value="not fitted")

        # --- Буферы трендов (все как в gui_ws.py) ---
        self.trend_ts = deque(maxlen=TREND_CAP)    # datetime для оси X
        self.trend_ns = deque(maxlen=TREND_CAP)
        self.trend_Ms = deque(maxlen=TREND_CAP)
        self.trend_Idc = deque(maxlen=TREND_CAP)
        self.trend_Isd = deque(maxlen=TREND_CAP)
        self.trend_Ud = deque(maxlen=TREND_CAP)
        self.trend_Uq = deque(maxlen=TREND_CAP)
        self.trend_Id = deque(maxlen=TREND_CAP)
        self.trend_Iq = deque(maxlen=TREND_CAP)
        self.trend_theta_ts = deque(maxlen=TREND_CAP)
        self.trend_theta = deque(maxlen=TREND_CAP)
        self.trend_theta_corr = deque(maxlen=TREND_CAP)
        self.trend_flux_error = deque(maxlen=TREND_CAP)
        self.trend_Isq = deque(maxlen=TREND_CAP)

        # --- Буферы карт (Ld(Id), Lq(Iq), а также Torque/Power vs RPM) ---
        self.map_Id = deque(maxlen=TREND_CAP)      # X для Ld(Id)
        self.map_Ld = deque(maxlen=TREND_CAP)      # Y для Ld(Id)
        self.map_Iq = deque(maxlen=TREND_CAP)      # X для Lq(Iq)
        self.map_Lq = deque(maxlen=TREND_CAP)      # Y для Lq(Iq)

        self.map_rpm = deque(maxlen=TREND_CAP)     # X для мом./мощн. от оборотов (rpm)
        self.map_ns = self.map_rpm
        self.map_Ms = deque(maxlen=TREND_CAP)      # момент (Н·м)
        self.map_Pmech = deque(maxlen=TREND_CAP)   # мех. мощность (кВт)
        self.map_Pelec = deque(maxlen=TREND_CAP)   # эл. мощность (кВт)
