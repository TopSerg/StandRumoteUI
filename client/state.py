# state.py
import tkinter as tk
from collections import deque

# ---------- Глобальные настройки UI ----------
APP_FONT = ("Segoe UI", 10)
MONO_FONT = ("Cascadia Mono", 9)  # или "Consolas"
PAD = 8

# ---------- Константы и алиасы телеметрии ----------
# Порядок колонок логбука (как в исходном коде)
TELEM_COLUMNS = [
    "ts",
    "ns", "Ms",
    "Udc", "Idc", "Isd", "Isq",
    "Ud", "Uq", "Id", "Iq",
    "Flux", "Theta", "Temperature", "StatorTemperature",
    "Rs", "TimeStamp", "ThetaCorr",
    "MCU_IGBTTempU", "MCU_IGBTTempV", "MCU_IGBTTempW", "MCU_IGBTTempMax",
    "MCU_TempCurrStr", "MCU_TempCurrCool",
    "M_max", "M_min",
    "MCU_OfsAl", "MCU_Isd", "MCU_Isq", "MCU_bDmpCActv",
    "MCU_stGateDrv", "MCU_DmpCTrqCurr", "MCU_VCUWorkMode",
    "ResolverSine", "ResolverCosine", "ResolverAmplitude",
    "ResolverTheta", "ResolverThetaCorr", "FluxPositionError",
    "ResolverThetaCorrection", "ResolverElectricalSpeed",
    "ResolverCalibrationState", "CanMode",
]

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
        self.En_Is_var = tk.IntVar(master=root, value=1)           # включение токов (используется в SendControl/SendTorque)

        # --- Параметры управления / лимиты (строки как в исходнике) ---
        self.Id_var = tk.StringVar(master=root, value="-0.5")
        self.Iq_var = tk.StringVar(master=root, value="0.0")
        self.M_min_var = tk.StringVar(master=root, value="-50.0")
        self.M_max_var = tk.StringVar(master=root, value="400.0")
        self.M_grad_max_var = tk.StringVar(master=root, value="50")
        self.n_max_var = tk.StringVar(master=root, value="1000")

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
        self.resolver_auto_gain_var = tk.StringVar(master=root, value="0.20")
        self.resolver_auto_tolerance_var = tk.StringVar(master=root, value="0.010")
        self.resolver_auto_max_step_var = tk.StringVar(master=root, value="0.020")
        self.resolver_sine_min = None
        self.resolver_sine_max = None
        self.resolver_cosine_min = None
        self.resolver_cosine_max = None

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
