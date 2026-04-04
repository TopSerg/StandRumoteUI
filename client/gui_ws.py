import tkinter as tk
from tkinter import Tk, ttk, Text, StringVar, Entry, Frame
from tkinter.scrolledtext import ScrolledText  
import json
import time
from datetime import datetime
import csv
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from collections import deque
import math

# ---------- Глобальные настройки UI ----------
APP_FONT = ("Segoe UI", 10)
MONO_FONT = ("Cascadia Mono", 9)  # или "Consolas"
PAD = 8

# === Ld/Lq online compute config ===
DEFAULT_RS_OHMS = 0.05  # поставьте ваш Rs по умолчанию, если не приходит из телеметрии
DEFAULT_POLE_PAIRS = 4  # можно задать число пар полюсов, если не приходит (например, 4)

# Поля, из которых пробуем взять значения (alias-ы на случай разных имён в JSON)
FIELD_ALIASES = {
    "Ud": ["Ud", "u_d", "U_d"],
    "Uq": ["Uq", "u_q", "U_q"],
    "Id": ["Id", "i_d", "I_d"],
    "Iq": ["Iq", "i_q", "I_q"],
    "Welectrical": ["Welectrical", "omega_e", "w_e"],
    "Wmechanical": ["Wmechanical", "omega_m", "w_m"],
    "motorEmfCalc": ["motorEmfCalc", "emf", "E_back"],
    "Rs": ["motorRs", "Rs", "R_s", "motorParams.motorRs"],
    "polePairs": ["polePairs", "p", "poles_pairs"],
}

# ——— Маппинг коробки передач (по DBC VcuActualGear) ———
GEAR_MAP = {"D": 4, "R": 3, "N": 2}
REV_GEAR_MAP = {v: k for k, v in GEAR_MAP.items()}

map_rpm = []

# ——— Маппинг режима управления двигателем (MotorCtrl) ———
# 1 — токовый режим (Id/Iq), 2 — режим по скорости/частоте (ns)
MOTOR_MODE_MAP = {
    "currents": 1,
    "speed": 2,
}

def init_style(dark=False):
    style = ttk.Style()
    # cross-platform базовая тема
    base_theme = "clam" if "clam" in style.theme_names() else style.theme_use()
    style.theme_use(base_theme)

    if dark:
        bg = "#1f2227"; fg = "#e6e6e6"; acc = "#3a7afe"; sub = "#9aa0a6"; frame = "#2a2f36"
    else:
        bg = "#f6f7fb"; fg = "#202124"; acc = "#215df0"; sub = "#5f6368"; frame = "#ffffff"

    # Общий фон окна
    style.configure(".", font=APP_FONT)
    style.configure("TFrame", background=bg)
    style.configure("TLabelframe", background=bg)
    style.configure("TLabelframe.Label", background=bg, foreground=fg, font=("Segoe UI Semibold", 10))
    style.configure("TLabel", background=bg, foreground=fg)
    style.configure("TButton", padding=(10, 6))
    style.configure("Accent.TButton", foreground="white", background=acc)
    style.map("Accent.TButton", background=[("active", acc)])

    style.configure("Toolbar.TFrame", background=frame)
    style.configure("Card.TFrame", background=frame, relief="groove", borderwidth=1)

    # Treeview (CAN-таблица)
    style.configure("Treeview", font=MONO_FONT, background=frame, fieldbackground=frame, foreground=fg, rowheight=22)
    style.configure("Treeview.Heading", font=("Segoe UI Semibold", 9))
    return style


import socketserver, threading, time
from network import WSClient
WS_URL = "ws://127.0.0.1:9000"  # при необходимости поменять
#WS_URL = "ws://192.168.0.19:9000"

def make_focusable_scale(scale, var, step=1.0):
    def on_click(event):
        global active_scale
        active_scale = (scale, var, step)
        scale.focus_set()
    scale.bind("<Button-1>", on_click)

def on_arrow_key(event):
    global active_scale
    if active_scale is None:
        return
    scale, var, step = active_scale
    value = var.get()
    if event.keysym == "Up":
        var.set(value + step)
    elif event.keysym == "Down":
        var.set(value - step)
        
active_scale = None  # текущий выбранный ползунок

def create_gui():
    root = tk.Tk()
    root.title("StandRemoteGUI")
    root.geometry("1080x740+100+100")
    try:
        root.state("zoomed")
    except Exception:
        pass
    style = init_style(dark=False)  # dark=True для тёмной темы

    # Индикатор статуса (цветная «пилюля»)
    conn_var = tk.StringVar(value="disabled")
    conn_color = tk.StringVar(value="#d72c20")  # красный

    toolbar = ttk.Frame(root, style="Toolbar.TFrame")
    toolbar.pack(fill="x")


    # --- КНОПКА ОТПРАВКИ ВСЕГО ---
    def send_all():
        # сначала всегда шлём актуальные лимиты
        send_limits_now()

        # аккуратно читаем выбранную передачу (если уже добавляли рамку передач)
        def _gear_code_or_none():
            try:
                return GEAR_MAP.get(gear_var.get(), 2)  # 2 = N
            except Exception:
                return None

        mode = mode_var.get()
        motor_code = MOTOR_MODE_MAP.get(mode, 1)
        gear_code = _gear_code_or_none()

        if mode == "currents":
            # --- РЕЖИМ ТОКОВ: шлём только Id/Iq (плюс служебный SendControl) ---
            try:
                isd = float(Id_var.get() or 0.0)
                isq = float(Iq_var.get() or 0.0)
            except Exception:
                ui_log("[UI] Некорректные Id/Iq — проверьте поля", "ERR")
                return

            # включаем токовый режим
            ctrl = {
                "cmd": "SendControl",
                "En_Is": True,
                "Kl_15": True,
            }
            if gear_code is not None:
                ctrl["GearCtrl"] = int(gear_code)
            if motor_code is not None:
                ctrl["MotorCtrl"] = int(motor_code)
                ctrl["ReqState"] = int(motor_code)

            client.send_json_threadsafe(ctrl)

            # отправляем только токи
            client.send_json_threadsafe({
                "cmd": "SendTorque",
                "En_Is": True,
                "Isd": isd,
                "Isq": isq,
            })
            ui_log(
                f"[UI] ▶ Отправлено: режим Токи (Id/Iq={isd:.2f}/{isq:.2f})"
                + (f", Gear={gear_var.get()}({gear_code})" if gear_code is not None else "")
            )

        else:
            # --- РЕЖИМ ОБОРОТОВ: шлём только ns (со слайдера Speed) ---
            try:
                ns = float(speed_var.get() or 0.0)
            except Exception:
                ui_log("[UI] Некорректное значение ns — проверьте поле/ползунок", "ERR")
                return

            ctrl = {
                "cmd": "SendControl",
                "En_Is": False,
                "Kl_15": True,
                "ns": ns,   # только скорость
            }
            if gear_code is not None:
                ctrl["GearCtrl"] = int(gear_code)
            if motor_code is not None:
                ctrl["MotorCtrl"] = int(motor_code)
                ctrl["ReqState"] = int(motor_code)

            client.send_json_threadsafe(ctrl)
            ui_log(
                f"[UI] ▶ Отправлено: режим Частота (ns={ns:.0f})"
                + (f", Gear={gear_var.get()}({gear_code})" if gear_code is not None else "")
            )

    # Кнопка в тулбаре
    ttk.Button(toolbar, text="Отправить", style="Accent.TButton", command=send_all)\
    .pack(side="left", padx=4, pady=PAD)

    def pill(parent, textvar, colorvar):
        wrap = tk.Frame(parent, bg=style.lookup("Toolbar.TFrame", "background"))
        dot = tk.Canvas(wrap, width=10, height=10, highlightthickness=0, bg=style.lookup("Toolbar.TFrame", "background"))
        oval = dot.create_oval(2,2,8,8, fill=colorvar.get(), outline="")
        lbl = ttk.Label(wrap, textvariable=textvar)
        dot.grid(row=0, column=0, padx=(0,4))
        lbl.grid(row=0, column=1)
        # обновление цвета
        def upd(*_):
            dot.itemconfig(oval, fill=colorvar.get())
        colorvar.trace_add("write", upd)
        return wrap

    # Крупные кнопки
    ttk.Button(toolbar, text="▶ Start", width=14, style="Accent.TButton",
               command=lambda: client.send_cmd_threadsafe("Init")).pack(side="left", padx=(PAD, 4), pady=PAD)
    ttk.Button(toolbar, text="■ Stop", width=14,
               command=lambda: client.send_cmd_threadsafe("Stop")).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="↺ Reset", width=14,
               command=lambda: client.send_cmd_threadsafe("Read2")).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="💾 Save", width=14,
               command=lambda: client.send_cmd_threadsafe("SaveCfg")).pack(side="left", padx=4, pady=PAD)

    # Индикатор соединения справа
    pill(toolbar, conn_var, conn_color).pack(side="right", padx=PAD, pady=PAD)

    
    # Коллбеки для WS с безопасным обновлением из главного потока
    log_box = tk.Text(root, height=10, wrap="word")
    def ui_log(*parts):
        msg = " ".join(str(p) for p in parts).strip()
        log_box.insert("end", msg + "\n")
        log_box.see("end")

    last_telemetry = {}
    telemetry_lock = threading.RLock()

    def _update_telemetry(obj):
        if not isinstance(obj, dict):
            return
        with telemetry_lock:
            # ключи, которые реально шлёт сервер: Ms, ns, Isd, Isq, Udc и т.д.
            for k in ("Ms", "ns", "Isd", "Isq", "Udc", "efficiency", "eta", "eff"):
                if k in obj:
                    last_telemetry[k] = obj[k]

    # Глобальные переменные для CAN данных
    can_rx_data = [StringVar() for _ in range(12)]  # 12 полей для Rx CAN
    can_tx_data = [StringVar() for _ in range(12)]  # 12 полей для Tx CAN

    # Журнал телеметрии
    log_enabled = tk.BooleanVar(value=True)
    log_rows = []  # список словарей
    max_rows = 5000  # ограничение на длину буфера/таблицы

    # Колонки журнала (порядок)
    TELEM_COLUMNS = [
        "ts",
        "ns", "Ms",
        "Idc", "Isd",
        "Ud", "Uq", "Id", "Iq",
        "Emf", "Welectrical", "motorRs", "Wmechanical",
    ]

    # === Ld/Lq online compute config ===
    DEFAULT_RS_OHMS = 0.05          # Ваш Rs по умолчанию, если не приходит в телеметрии
    DEFAULT_POLE_PAIRS = None       # Число пар полюсов, если не приходит (например, 4)

    # Алиасы названий полей на случай разных имён в JSON
    FIELD_ALIASES = {
        "Ud": ["Ud", "u_d", "U_d"],
        "Uq": ["Uq", "u_q", "U_q"],
        "Id": ["Id", "i_d", "I_d"],
        "Iq": ["Iq", "i_q", "I_q"],
        "Welectrical": ["Welectrical", "omega_e", "w_e"],
        "Wmechanical": ["Wmechanical", "omega_m", "w_m"],
        "motorEmfCalc": ["motorEmfCalc", "emf", "E_back"],
        "Rs": ["motorRs", "Rs", "R_s", "motorParams.motorRs"],
        "polePairs": ["polePairs", "p", "poles_pairs"],
    }

    def _get_float(data: dict, key: str) -> float | None:
        names = FIELD_ALIASES.get(key, [key])
        for name in names:
            if name in data and data[name] is not None:
                try:
                    return float(data[name])
                except Exception:
                    pass
        return None


    # Буферы для трендов (последние N точек)
    TREND_CAP = 3000  # сколько точек держим на графиках
    trend_ts   = deque(maxlen=TREND_CAP)  # datetime для оси X
    trend_ns   = deque(maxlen=TREND_CAP)
    trend_Ms   = deque(maxlen=TREND_CAP)
    trend_Idc  = deque(maxlen=TREND_CAP)
    trend_Isd  = deque(maxlen=TREND_CAP)
    trend_Ud   = deque(maxlen=TREND_CAP)
    trend_Uq   = deque(maxlen=TREND_CAP)
    trend_Id   = deque(maxlen=TREND_CAP)
    trend_Iq   = deque(maxlen=TREND_CAP)

    # --- NEW: буферы для карт (maps) ---
    map_Id   = deque(maxlen=TREND_CAP)   # X для Ld(Id)
    map_Ld   = deque(maxlen=TREND_CAP)   # Y для Ld(Id)
    map_Iq   = deque(maxlen=TREND_CAP)   # X для Lq(Iq)
    map_Lq   = deque(maxlen=TREND_CAP)   # Y для Lq(Iq)

    map_ns   = deque(maxlen=TREND_CAP)   # X для мом./мощн. от оборотов (rpm)
    map_Ms   = deque(maxlen=TREND_CAP)   # момент (Н·м)
    map_Pmech= deque(maxlen=TREND_CAP)   # мех. мощность (кВт)
    map_Pelec= deque(maxlen=TREND_CAP)   # эл. мощность (кВт) — опционально



    # ==== КНОПОЧНЫЕ ХЭНДЛЕРЫ ====
    def send_control_now():
        """Применить режим и ключевые флаги (и, если режим 'Частота', то ns)."""

        mode = mode_var.get()
        motor_code = MOTOR_MODE_MAP.get(mode, 1)

        if mode == "speed":
            try:
                ns = float(speed_var.get() or 0.0)
            except Exception:
                ui_log("[UI] ns: некорректное значение", "ERR")
                return

            payload = {
                "cmd": "SendControl",
                "En_Is": False,
                "Kl_15": True,
                "ns": ns,
                "MotorCtrl": motor_code,
            }
            client.send_json_threadsafe(payload)
            ui_log(
                f"[UI] SendControl: Частота (ns={ns:.0f}, MotorCtrl={motor_code})",
                "UI",
            )

        else:  # "currents"
            payload = {
                "cmd": "SendControl",
                "En_Is": True,
                "Kl_15": True,
                "MotorCtrl": motor_code,
            }
            client.send_json_threadsafe(payload)
            ui_log(
                f"[UI] SendControl: Токи (En_Is=1, Kl_15=0, MotorCtrl={motor_code})",
                "UI",
            )



    def send_limits_now():
        """Отправить лимиты (M_min/M_max/M_grad_max/n_max)."""
        def _pf(v, name, as_int=False):
            try:
                return (int(float(v.get())) if as_int else float(v.get()))
            except Exception:
                ui_log(f"[UI] {name}: некорректное значение", "ERR")
                raise

        try:
            payload = {
                "cmd": "SendLimits",
                "M_min": _pf(M_min_var, "M_min"),
                "M_max": _pf(M_max_var, "M_max"),
                "M_grad_max": _pf(M_grad_max_var, "M_grad_max", as_int=True),
                "n_max": _pf(n_max_var, "n_max", as_int=True),
            }
            print(f"DEBUG: M_min= {payload['M_min']}, M_max= {payload['M_max']}")

        except Exception:
            return

        client.send_json_threadsafe(payload)
        ui_log("[UI] SendLimits отправлен", "UI")


    def send_torque_now():
        """Отправить Id/Iq (всегда с En_Is=True, чтобы зафиксировать токовый режим)."""
        try:
            Id = float(Id_var.get() or 0.0)
            Iq = float(Iq_var.get() or 0.0)
        except Exception:
            ui_log("[UI] Id/Iq: incorrect values", "ERR"); return

        client.send_json_threadsafe({
            "cmd": "SendTorque",
            "En_Is": True,
            "Isd": Id,
            "Isq": Iq
        })
        ui_log(f"[UI] SendTorque: Id={Id:.2f}, Iq={Iq:.2f}", "UI")


    def set_mode_from_ui():
        """Кнопка 'Применить режим' — вызывает ту же логику, что и радиокнопки."""
        # просто переиспользуем текущий режим
        if mode_var.get() == "speed":
            # включаем частоту
            send_control_now()
        else:
            # включаем токи + сразу текущие Id/Iq (как мы делали при переключении)
            send_control_now()
            send_torque_now()


    def on_message(msg):
        root.after(0, lambda: ui_log(f"[RX] {msg}"))
        try:
            data = json.loads(msg)
        except json.JSONDecodeError:
            root.after(0, lambda: ui_log("❌ Couldn't parse JSON"))
            return

        def _ui_work():
            ui_log(f"[RX] {msg}")
            if data.get("type") == "can_frame":
                handle_can_frame(data)
            elif any(k in data for k in ["Ms", "ns", "Isd", "Udc", "Ud", "Uq", "Id", "Iq"]):
                handle_model_data(data)   # теперь точно в UI-потоке
            else:
                ui_log("⚠ Unknown message type")

        root.after(0, _ui_work)
    
    def handle_can_frame(frame_data):
        direction = frame_data.get("direction", "")

        if direction == "rx":
            for i in range(8):
                if f"data{i}" in frame_data:
                    can_rx_data[i+1].set(str(frame_data[f"data{i}"]))
            can_rx_data[0].set(str(frame_data.get("id", "")))
            can_rx_data[9].set(str(frame_data.get("len", "")))
            can_rx_data[10].set(str(frame_data.get("flags", "")))
            can_rx_data[11].set(str(frame_data.get("ts", "")))

        elif direction == "tx":
            for i in range(8):
                if f"data{i}" in frame_data:
                    can_tx_data[i+1].set(str(frame_data[f"data{i}"]))
            can_tx_data[0].set(str(frame_data.get("id", "")))
            can_tx_data[9].set(str(frame_data.get("len", "")))
            can_tx_data[10].set(str(frame_data.get("flags", "")))
            can_tx_data[11].set(str(frame_data.get("ts", "")))

    # ===== журнал телеметрии =====
    def log_telemetry_row(data):
        """Собрать строку журнала, добавить в буфер и в таблицу."""
        if not log_enabled.get():
            return

        row = {}
        row["ts"] = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        for k in TELEM_COLUMNS:
            if k == "ts":
                continue
            if k in data:
                row[k] = data[k]
        # если ничего нового нет — не шумим
        if len(row) <= 1:
            return

        log_rows.append(row)
        if len(log_rows) > max_rows:
            del log_rows[0]
            kids = telem_tree.get_children()
            if kids:
                telem_tree.delete(kids[0])

        # --- NEW: карты Ld/Lq при наличии ---
        if "Id" in data and "Ld" in data:
            try:
                map_Id.append(float(data["Id"]))
                map_Ld.append(float(data["Ld"]))
            except Exception:
                pass

        if "Iq" in data and "Lq" in data:
            try:
                map_Iq.append(float(data["Iq"]))
                map_Lq.append(float(data["Lq"]))
            except Exception:
                pass

        # --- NEW: момент/мощность от оборотов ---
        # rpm = ns; мех. угл. скорость Wmechanical (рад/с) приходит в данных (если есть)
        rpm = None
        if "ns" in data:
            try: rpm = float(data["ns"])
            except Exception: rpm = None

        Ms = None
        if "Ms" in data:
            try: Ms = float(data["Ms"])
            except Exception: Ms = None

        Wm = None
        if "Wmechanical" in data:
            try: Wm = float(data["Wmechanical"])
            except Exception: Wm = None

        if rpm is not None:
            map_ns.append(rpm)
            # момент
            if Ms is not None:
                map_Ms.append(Ms)
            # P_mech = Ms * Wmechanical (Вт) -> кВт
            if Ms is not None and Wm is not None:
                map_Pmech.append(Ms * Wm / 1000.0)

        # P_elec = Ud*Id + Uq*Iq (Вт) -> кВт, если все есть
        if all(k in data for k in ("Ud","Uq","Id","Iq")):
            try:
                Ud = float(data["Ud"]); Uq = float(data["Uq"])
                Idv = float(data["Id"]); Iqv = float(data["Iq"])
                map_Pelec.append((Ud*Idv + Uq*Iqv)/1000.0)
                # чтобы оси X совпадали — если rpm неизвестен, подкинем NaN (не рисуется)
                if "ns" not in data:
                    map_ns.append(float("nan"))
                    map_Ms.append(float("nan"))
                    map_Pmech.append(float("nan"))
            except Exception:
                pass

                # ===== Ld/Lq online calc -> карты Ld(Id) / Lq(Iq) =====
        Ud  = _get_float(data, "Ud")
        Uq  = _get_float(data, "Uq")
        Idv = _get_float(data, "Id")
        Iqv = _get_float(data, "Iq")
        we  = _get_float(data, "Welectrical")
        wm  = _get_float(data, "Wmechanical")
        emf = _get_float(data, "motorEmfCalc")
        Rs  = _get_float(data, "Rs") or DEFAULT_RS_OHMS
        pp  = _get_float(data, "polePairs") or DEFAULT_POLE_PAIRS

        # если нет электрической скорости, но есть мех. и пары полюсов — восстановим
        if we is None and (wm is not None) and (pp is not None):
            we = wm * pp

        # потокосцепление из ЭДС и электрической скорости
        psi_f = None
        if emf is not None and we not in (None, 0.0):
            psi_f = emf / we

        # --- Lq из формулы: Lq = (Ud - Rs*Id) / (we*Iq)
        if all(v is not None for v in (Ud, Idv, Iqv, we, Rs)) and Iqv not in (None, 0.0) and we != 0.0:
            try:
                Lq_val = (Ud - Rs * Idv) / (we * Iqv)
                map_Iq.append(Iqv)      # X: Iq
                map_Lq.append(Lq_val)   # Y: Lq
            except Exception:
                pass

        # --- Ld из формулы: Ld = (Uq - Rs*Iq - we*psi_f) / (we*Id)
        if all(v is not None for v in (Uq, Idv, Iqv, we, Rs, psi_f)) and Idv not in (None, 0.0) and we != 0.0:
            try:
                Ld_val = (Uq - Rs * Iqv - we * psi_f) / (we * Idv)
                map_Id.append(Idv)      # X: Id
                map_Ld.append(Ld_val)   # Y: Ld
            except Exception:
                pass

        # ===== Момент/мощность от оборотов -> карты Torque/Power vs RPM =====
        # RPM из ns, либо из wm (рад/с) -> rpm = wm * 60 / (2*pi)
        rpm = None
        if "ns" in data:
            try:
                rpm = float(data["ns"])
            except Exception:
                rpm = None
        if rpm is None and wm is not None:
            try:
                rpm = wm * 60.0 / (2.0 * math.pi)
            except Exception:
                rpm = None

        Ms = None
        if "Ms" in data:
            try:
                Ms = float(data["Ms"])
            except Exception:
                Ms = None

        if rpm is not None:
            map_rpm.append(rpm)
            if Ms is not None:
                map_Ms.append(Ms)
            if Ms is not None and wm is not None:
                map_Pmech.append(Ms * wm / 1000.0)   # Вт -> кВт

        if all(v is not None for v in (Ud, Uq, Idv, Iqv)):
            map_Pelec.append((Ud * Idv + Uq * Iqv) / 1000.0)  # Вт -> кВт



        telem_tree.insert("", "end", values=[row.get(k, "") for k in TELEM_COLUMNS])

         # === подпитаем тренды ===
        now = datetime.now()
        trend_ts.append(now)
        if "ns" in data:  trend_ns.append(float(data["ns"]))
        if "Ms" in data:  trend_Ms.append(float(data["Ms"]))
        if "Idc" in data: trend_Idc.append(float(data["Idc"]))
        if "Isd" in data: trend_Isd.append(float(data["Isd"]))
        if "Ud" in data:  trend_Ud.append(float(data["Ud"]))
        if "Uq" in data:  trend_Uq.append(float(data["Uq"]))
        if "Id" in data:  trend_Id.append(float(data["Id"]))
        if "Iq" in data:  trend_Iq.append(float(data["Iq"]))

    def _update_trends():
        # ось X — секунды относительно последней точки
        if not trend_ts:
            root.after(500, _update_trends)
            return
        t0 = trend_ts[-1]
        xs = [(t - t0).total_seconds() for t in trend_ts]  # идут отрицательные числа (в прошлое)

        # обновляем данные линий
        if trend_ns:
            l_ns.set_data(xs[-len(trend_ns):], list(trend_ns))
            ax1.relim(); ax1.autoscale_view()

        if trend_Ms:
            l_ms.set_data(xs[-len(trend_Ms):], list(trend_Ms))
            ax2.relim(); ax2.autoscale_view()

        if trend_Idc:
            l_idc.set_data(xs[-len(trend_Idc):], list(trend_Idc))
        if trend_Isd:
            l_isd.set_data(xs[-len(trend_Isd):], list(trend_Isd))
        ax3.relim(); ax3.autoscale_view()

        # внизу рисуем Id/Iq и Ud/Uq одним графиком: если нет одних — будут другие
        if trend_Id:
            l_id.set_data(xs[-len(trend_Id):], list(trend_Id))
        if trend_Iq:
            l_iq.set_data(xs[-len(trend_Iq):], list(trend_Iq))
        if trend_Ud:
            l_ud.set_data(xs[-len(trend_Ud):], list(trend_Ud))
        if trend_Uq:
            l_uq.set_data(xs[-len(trend_Uq):], list(trend_Uq))
        ax4.relim(); ax4.autoscale_view()

        # едва заметные подписи оси X
        for ax in (ax1, ax2, ax3, ax4):
            ax.set_xlabel("seconds from now")

        canvas.draw_idle()
        root.after(500, _update_trends)  # ~2 FPS; можно 200мс для плавнее    

    def handle_model_data(data):
        field_map = {
            # Параметры стенда
            "ns": "Speed rotation",
            "Ms": "Torque (Ms)",
            "Idc": "direct current (Idc)",
            "Isd": "Stator current d (Isd)",
            "MCU_IGBTTempU": "IGBT temperature",
            "MCU_TempCurrStr": "Stator temperature",

            # MCU Current & Voltage
            "Ud": "Ud",
            "Uq": "Uq",
            "Id": "Id",
            "Iq": "Iq",

            # MCU Flux Parameters
            "Emf": "Emf",
            "Welectrical": "Welectrical",
            "motorRs": "motorRs",
            "Wmechanical": "Wmechanical",
        }

        for key, label in field_map.items():
            if key in data and label in entry_vars:
                entry_vars[label].set(str(data[key]))

        # Доп. лог (опционально)
        for key in ("Ms", "Idc", "Isd", "ns", "Udc"):
            if key in data:
                ui_log(f"{key}: {data[key]}")
        
        # строка в журнал
        log_telemetry_row(data)

    
    
    def send_fake_can_from_fields():
        try:
            # Получаем значения
            Id = float(Id_var.get() or 0)
            Iq = float(Iq_var.get() or 0)
            torque = float(torque_var.get() or 0)
            speed = float(speed_var.get() or 0)

            # Формируем CAN-кадр
            can_msg = {
                "cmd": "FakeCAN",
                "direction": "tx",
                "id": 0x555,  # можно выбрать любой ID
                "len": 8,
                "flags": 0,
                "ts": 10,#time.time(),  # или фиксированное значение
                "data0": int(Id * 10) & 0xFF,
                "data1": int(Iq * 10) & 0xFF,
                "data2": int(torque) & 0xFF,
                "data3": int(speed / 10) & 0xFF,
                "data4": 0,
                "data5": 0,
                "data6": 0,
                "data7": 0
            }

            # Отправляем
            client.send_json_threadsafe(can_msg)
            ui_log("[UI] Sent a FakeCAN from the fields: Id/Iq, Torque, Speed")
        except Exception as e:
            ui_log(f"[Ошибка отправки FakeCAN] {e}")

    def on_status(msg):
        root.after(0, lambda: ui_log(f"[WS] {msg}"))

    def on_error(msg):
        root.after(0, lambda: ui_log(f"[ERR] {msg}"))

    # ---------------- UI layout ----------------
    # Вкладки
    notebook = ttk.Notebook(root)
    notebook.pack(fill="both", expand=True)
    main_frame = ttk.Frame(notebook)
    notebook.add(main_frame, text="Control")
    ind_frame = ttk.Frame(notebook)
    notebook.add(ind_frame, text="Indication")
    log_frame = ttk.Frame(notebook)
    notebook.add(log_frame, text="Logbook")
    trends_frame = ttk.Frame(notebook)
    notebook.add(trends_frame, text="Trends")

    # === Trends UI ===
    trends_container = ttk.Frame(trends_frame)
    trends_container.pack(fill="both", expand=True, padx=10, pady=10)

    # --- NEW: вкладка "Maps" ---
    maps_frame = ttk.Frame(notebook)
    notebook.add(maps_frame, text="Maps")

    maps_container = ttk.Frame(maps_frame)
    maps_container.pack(fill="both", expand=True, padx=10, pady=10)

    fig_maps = Figure(figsize=(8, 5), dpi=100)
    ax5a = fig_maps.add_subplot(221)  # Ld(Id)
    ax5b = fig_maps.add_subplot(222)  # Lq(Iq)
    ax6  = fig_maps.add_subplot(212)  # Torque & Power vs RPM (двойная ось Y)

    ax5a.set_title("Ld vs Id"); ax5a.set_xlabel("Id, A"); ax5a.set_ylabel("Ld, H"); ax5a.grid(True)
    ax5b.set_title("Lq vs Iq"); ax5b.set_xlabel("Iq, A"); ax5b.set_ylabel("Lq, H"); ax5b.grid(True)
    ax6.set_title("Torque & Power vs RPM"); ax6.set_xlabel("RPM"); ax6.grid(True)
    ax6_right = ax6.twinx()
    ax6.set_ylabel("Torque, N·m")
    ax6_right.set_ylabel("Power, kW")

    # Примитивы
    sc_ld = ax5a.plot([], [], linestyle="", marker=".", markersize=3)[0]
    sc_lq = ax5b.plot([], [], linestyle="", marker=".", markersize=3)[0]
    ln_torque, = ax6.plot([], [], label="Torque (N·m)")
    ln_pmech,  = ax6_right.plot([], [], label="P_mech (kW)")
    ln_pelec,  = ax6_right.plot([], [], label="P_elec (kW)", linestyle="--")

    # Общая легенда по обеим осям
    handles = [ln_torque, ln_pmech, ln_pelec]
    ax6.legend(handles, [h.get_label() for h in handles], loc="best")

    canvas_maps = FigureCanvasTkAgg(fig_maps, master=maps_container)
    canvas_maps.get_tk_widget().pack(fill="both", expand=True)

    # Запускаем WS-клиент
    client = WSClient(WS_URL, on_message, on_status, on_error)
    client.start()
    
    root.protocol("WM_DELETE_WINDOW", lambda: (client.stop(), root.destroy()))


    fig = Figure(figsize=(8, 5), dpi=100)
    ax1 = fig.add_subplot(221)  # Speed (ns)
    ax2 = fig.add_subplot(222)  # Torque (Ms)
    ax3 = fig.add_subplot(223)  # Currents (Idc/Isd)
    ax4 = fig.add_subplot(224)  # dq currents/voltages (Id/Iq or Ud/Uq)

    ax1.set_title("Speed (ns)")
    ax2.set_title("Torque (Ms)")
    ax3.set_title("Currents (Idc / Isd)")
    ax4.set_title("dq (Id/Iq) and Voltages (Ud/Uq)")

    # линии (пустые на старте)
    l_ns, = ax1.plot([], [])
    l_ms, = ax2.plot([], [])
    l_idc, = ax3.plot([], [])
    l_isd, = ax3.plot([], [])
    l_id, = ax4.plot([], [])
    l_iq, = ax4.plot([], [])
    l_ud, = ax4.plot([], [])
    l_uq, = ax4.plot([], [])

    for ax in (ax1, ax2, ax3, ax4):
        ax.grid(True)

    canvas = FigureCanvasTkAgg(fig, master=trends_container)
    canvas_widget = canvas.get_tk_widget()
    canvas_widget.pack(fill="both", expand=True)

    # --- NEW: вкладка "Maps" ---
    # --- NEW: вкладка "Maps" ---
    maps_frame = ttk.Frame(notebook)
    notebook.add(maps_frame, text="Maps")

    maps_container = ttk.Frame(maps_frame)
    maps_container.pack(fill="both", expand=True, padx=10, pady=10)

    fig_maps = Figure(figsize=(8,5), dpi=100)
    ax5a = fig_maps.add_subplot(221)  # Ld(Id)
    ax5b = fig_maps.add_subplot(222)  # Lq(Iq)
    ax6  = fig_maps.add_subplot(212)  # Torque & Power vs RPM (с двойной осью Y)

    ax5a.set_title("Ld vs Id")
    ax5a.set_xlabel("Id, A"); ax5a.set_ylabel("Ld, H")
    ax5a.grid(True)

    ax5b.set_title("Lq vs Iq")
    ax5b.set_xlabel("Iq, A"); ax5b.set_ylabel("Lq, H")
    ax5b.grid(True)

    ax6.set_title("Torque & Power vs RPM")
    ax6.set_xlabel("RPM"); ax6.grid(True)
    ax6_right = ax6.twinx()
    ax6.set_ylabel("Torque, N·m")     # левая ось
    ax6_right.set_ylabel("Power, kW") # правая ось

    # линии/точки (пустые на старте)
    sc_ld = ax5a.plot([], [], linestyle="", marker=".", markersize=3)[0]
    sc_lq = ax5b.plot([], [], linestyle="", marker=".", markersize=3)[0]

    ln_torque, = ax6.plot([], [], label="Torque (N·m)")
    ln_pmech,  = ax6_right.plot([], [], label="P_mech (kW)")
    ln_pelec,  = ax6_right.plot([], [], label="P_elec (kW)", linestyle="--")

    # легенда — соберём хэндлы с обеих осей
    handles = [ln_torque]
    handles_right = [ln_pmech, ln_pelec]
    ax6.legend(handles + handles_right, [h.get_label() for h in handles + handles_right], loc="best")

    canvas_maps = FigureCanvasTkAgg(fig_maps, master=maps_container)
    canvas_maps.get_tk_widget().pack(fill="both", expand=True)


    notebook.enable_traversal()

    # --- ЖУРНАЛ ---
    # панель кнопок
    log_toolbar = ttk.Frame(log_frame)
    log_toolbar.pack(fill="x", padx=10, pady=(10, 0))

    def clear_log():
        log_text.delete("1.0", "end")

    def copy_log():
        root.clipboard_clear()
        root.clipboard_append(log_text.get("1.0", "end"))
        root.update()  # чтобы буфер обмена сохранился после закрытия окна

    ttk.Button(log_toolbar, text="Очистить", command=clear_log).pack(side="left")
    ttk.Button(log_toolbar, text="Копировать", command=copy_log).pack(side="left", padx=6)

    # само поле лога со скроллом
    log_text = ScrolledText(log_frame, height=18, wrap="word", font=("Consolas", 10))
    log_text.pack(fill="both", expand=True, padx=10, pady=10)

    # функция логирования с таймштампом
    def ui_log(msg: str):
        ts = time.strftime("%H:%M:%S")
        log_text.insert("end", f"[{ts}] {msg.strip()}\n")
        log_text.see("end")
    
    # Вкладка 1
    main_inner = ttk.Frame(main_frame)
    main_inner.pack(fill="both", expand=True)

    for i in range(3):
        main_inner.grid_columnconfigure(i, weight=1)   # 2 левые колонки растягиваются
    main_inner.grid_columnconfigure(2, weight=0)       # правая колонка под слайдеры
    for r in range(10):
        main_inner.grid_rowconfigure(r, weight=0)

    controls_container = ttk.Frame(main_inner)
    controls_container.grid(row=1, column=0, columnspan=3, padx=10, pady=10, sticky="ew")

    # Сетка: 3 колонки (левая/средняя/правая-«слайдеры»)
    for col in (0, 1):
        main_inner.columnconfigure(col, weight=1)   # растягиваем контент
    main_inner.columnconfigure(2, weight=0)         # колонка со слайдерами фикс ширины
    # строки, где большие панели, пусть растягиваются
    for r in (2, 3):
        main_inner.rowconfigure(r, weight=1)


        # ---  переключатель режима ---
    mode_frame = ttk.LabelFrame(main_inner, text="Control mode")
    mode_frame.grid(row=0, column=0, columnspan=1, padx=10, pady=10, sticky="ew")


    # ——— Рамка выбора передачи ———
    gear_frame = ttk.LabelFrame(main_inner, text="Gear (D/R/N)")
    gear_frame.grid(row=0, column=1, padx=10, pady=10, sticky="ew")

    gear_var = tk.StringVar(value="N")  # стартовое значение — N

    def set_gear_from_ui():
        sel = gear_var.get()
        code = GEAR_MAP[sel]
        client.send_json_threadsafe({
            "cmd": "SendControl",
            "GearCtrl": int(code)
        })
        ui_log(f"[UI] Gear set to {sel} (code {code})", "UI")

    ttk.Radiobutton(gear_frame, text="D", value="D", variable=gear_var,
                    command=set_gear_from_ui).grid(row=0, column=0, padx=8, pady=8, sticky="w")
    ttk.Radiobutton(gear_frame, text="R", value="R", variable=gear_var,
                    command=set_gear_from_ui).grid(row=0, column=1, padx=8, pady=8, sticky="w")
    ttk.Radiobutton(gear_frame, text="N", value="N", variable=gear_var,
                    command=set_gear_from_ui).grid(row=0, column=2, padx=8, pady=8, sticky="w")

    # текущее значение режима: "currents" (токи) или "speed" (частота)
    mode_var = tk.StringVar(value="currents")

    def update_mode_controls():
        # включаем/выключаем ползунки в зависимости от режима
        if mode_var.get() == "speed":
            speed_slider.state(["!disabled"])
            speed_entry.config(state="normal")
            torque_slider.state(["disabled"])
            torque_entry.config(state="disabled")
        else:
            speed_slider.state(["disabled"])
            speed_entry.config(state="disabled")
            torque_slider.state(["!disabled"])
            torque_entry.config(state="normal")

    def set_mode(val: str):
        mode_var.set(val)
        update_mode_controls()
        ui_log("[UI] Режим выбран: "
            + ("Токи (Id/Iq) — будет отправлено En_Is=1, Kl_15=0" if val == "currents"
                else "Частота (ns) — будет отправлено En_Is=0, Kl_15=1")
            + " → нажмите «Отправить»")

    # сами «сегменты» — две радиокнопки
    rb1 = ttk.Radiobutton(mode_frame, text="Currents (Id/Iq)",
                        value="currents", variable=mode_var,
                        command=lambda: set_mode("currents"))
    rb2 = ttk.Radiobutton(mode_frame, text="Frequency (ns)",
                        value="speed", variable=mode_var,
                        command=lambda: set_mode("speed"))

    rb1.grid(row=0, column=0, padx=8, pady=8, sticky="w")
    rb2.grid(row=0, column=1, padx=8, pady=8, sticky="w")


    # Кнопки управления
    control_frame = ttk.Frame(controls_container)
    control_frame.pack(padx=10, pady=10, fill="x")

    # Создадим переменные токов/флага заранее (нужны для варианта A)
    currents_frame = ttk.LabelFrame(main_inner, text="Токи (Id/Iq)")
    currents_frame.place(x=10, y=120, width=340, height=120)

    En_Is_var = tk.IntVar(value=1)
    Id_var = tk.StringVar(value="-0.5")
    Iq_var = tk.StringVar(value="0.0")

    ttk.Label(currents_frame, text="Id [A]").grid(row=1, column=0, sticky="e", padx=6, pady=6)
    id_entry = ttk.Entry(currents_frame, width=10, textvariable=Id_var)
    id_entry.grid(row=1, column=1, sticky="w")

    ttk.Label(currents_frame, text="Iq [A]").grid(row=1, column=2, sticky="e", padx=6, pady=6)
    iq_entry = ttk.Entry(currents_frame, width=10, textvariable=Iq_var)
    iq_entry.grid(row=1, column=3, sticky="w")

    # >>> ВАРИАНТ А: функция, которая применяет значения агента к GUI-полям
    def apply_agent_currents(Isd: float, Isq: float, En_rem: bool):
        try:
            En_rem_var.set(1 if En_rem else 0)
            Id_var.set(f"{Isd:.3f}")
            Iq_var.set(f"{Isq:.3f}")
        except Exception:
            pass

    ttk.Button(control_frame, text="▶ Старт", width=15,
               command=lambda: client.send_cmd_threadsafe("Init")).pack(side="left", padx=5)
    ttk.Button(control_frame, text="■ Стоп", width=15,
               command=lambda: client.send_cmd_threadsafe("Stop")).pack(side="left", padx=5)
    ttk.Button(control_frame, text="↺ Сброс", width=15,
               command=lambda: client.send_cmd_threadsafe("Read2")).pack(side="left", padx=5)
    ttk.Button(control_frame, text="💾 Сохранить", width=15,
               command=lambda: client.send_cmd_threadsafe("SaveCfg")).pack(side="left", padx=5)

    # ====== Блок "Лимиты" ======
    limits_frame = ttk.LabelFrame(main_inner, text="Limits")
    limits_frame.grid(  row=1, column=1, padx=10, pady=10, sticky="nsew")

    M_min_var      = tk.StringVar(value="-50.0")
    M_max_var      = tk.StringVar(value="400.0")
    M_grad_max_var = tk.StringVar(value="50")
    n_max_var      = tk.StringVar(value="1000")

    ttk.Label(limits_frame, text="M_min [Н·м]").grid(row=0, column=0, sticky="e", padx=6, pady=6)
    ttk.Entry(limits_frame, width=10, textvariable=M_min_var).grid(row=0, column=1, sticky="w")

    ttk.Label(limits_frame, text="M_max [Н·м]").grid(row=0, column=2, sticky="e", padx=6, pady=6)
    ttk.Entry(limits_frame, width=10, textvariable=M_max_var).grid(row=0, column=3, sticky="w")

    ttk.Label(limits_frame, text="M_grad_max").grid(row=1, column=0, sticky="e", padx=6, pady=6)
    ttk.Entry(limits_frame, width=10, textvariable=M_grad_max_var).grid(row=1, column=1, sticky="w")

    ttk.Label(limits_frame, text="n_max [rpm]").grid(row=1, column=2, sticky="e", padx=6, pady=6)
    ttk.Entry(limits_frame, width=10, textvariable=n_max_var).grid(row=1, column=3, sticky="w")

    # Доп. команды
    extra_frame = ttk.Frame(controls_container)
    extra_frame.pack(padx=0, pady=(6, 0), fill="x")

    ttk.Button(
        extra_frame, text="Применить режим", width=18,
        command=set_mode_from_ui
    ).pack(side="left", padx=5)

    # Лимиты
    ttk.Button(
        extra_frame, text="SendLimits", width=15,
        command=send_limits_now
    ).pack(side="left", padx=5)

    # Токи Id/Iq
    ttk.Button(
        extra_frame, text="SendTorque (Id/Iq)", width=20,
        command=send_torque_now
    ).pack(side="left", padx=5)

    
    # Параметры стенда
    params_frame = ttk.LabelFrame(main_inner, text="MCU_VCU_parameters")
    params_frame.grid(  row=2, column=0, columnspan=2, padx=10, pady=0, sticky="nsew")
    params = [
        "Speed rotation",
        "Torque (Ms)",        # было: "Момент (Ms)"
        "direct current (Idc)",   # было: "Ток постоянного (Idc)"
        "Stator current d (Isd)",  # было: "Ток статора d (Isd)"
        "IGBT temperature", # было: "Температура статора"
        "Stator temperature",  # было: "Температура ротора"
    ]
    

    entry_vars = {}
    for i, param in enumerate(params):
        ttk.Label(params_frame, text=param + ":").grid(row=i, column=0, sticky="e", padx=5, pady=5)
        var = tk.StringVar()
        entry = ttk.Entry(params_frame, textvariable=var, width=20)
        entry.grid(row=i, column=1, padx=5, pady=5)
        entry_vars[param] = var

    # CAN
    can_frame = ttk.LabelFrame(main_inner, text="Tx / Rx CAN")
    can_frame.place(x=10, y=390, width=710, height=140)
    headers = ["id"] + [f"data{i}" for i in range(8)] + ["len", "flags", "ts"]
    for col, header in enumerate(headers):
        ttk.Label(can_frame, text=header, anchor="center", width=8).grid(row=0, column=col+1, padx=2, pady=(0, 5))
    ttk.Label(can_frame, text="Tx:").grid(row=1, column=0, sticky="e", padx=3)
    ttk.Label(can_frame, text="Rx:").grid(row=2, column=0, sticky="e", padx=3)
    for col in range(12):
        Entry(can_frame, textvariable=can_tx_data[col], width=8, justify="center", state="readonly")\
            .grid(row=1, column=col+1, padx=2, pady=2)
    for col in range(12):
        entry = Entry(can_frame, textvariable=can_rx_data[col], width=8, justify="center", state="readonly")
        entry.grid(row=2, column=col+1, padx=2, pady=2)
    
    # Блок MCU_CurrentVoltage
    voltage_frame = ttk.LabelFrame(main_inner, text="MCU Current & Voltage")
    voltage_frame.grid( row=4, column=0, padx=10, pady=10, sticky="nsew")

    voltage_params = ["Ud", "Uq", "Id", "Iq"]
    for i, param in enumerate(voltage_params):
        ttk.Label(voltage_frame, text=param + ":").grid(row=i, column=0, sticky="e", padx=5, pady=3)
        var = tk.StringVar()
        entry = ttk.Entry(voltage_frame, textvariable=var, width=15)
        entry.grid(row=i, column=1, padx=5, pady=3)
        entry_vars[param] = var

    # Блок MCU_FluxParams
    flux_frame = ttk.LabelFrame(main_inner, text="MCU Flux Parameters")
    flux_frame.grid(    row=4, column=1, padx=10, pady=10, sticky="nsew")

    flux_params = ["Emf", "Welectrical", "motorRs", "Wmechanical"]
    for i, param in enumerate(flux_params):
        ttk.Label(flux_frame, text=param + ":").grid(row=i, column=0, sticky="e", padx=5, pady=3)
        var = tk.StringVar()
        entry = ttk.Entry(flux_frame, textvariable=var, width=15)
        entry.grid(row=i, column=1, padx=5, pady=3)
        entry_vars[param] = var
    
    # Правая колонка: слайдеры
    slider_frame = ttk.Frame(main_inner, width=180, height=450)
    slider_frame.grid(  row=0, column=2, rowspan=6, padx=10, pady=10, sticky="ns")
    slider_frame.pack_propagate(False)

    speed_var = tk.DoubleVar()
    torque_var = tk.DoubleVar()

    # layout на grid (без .place)
    slider_frame.grid_columnconfigure(0, weight=1, minsize=80)
    slider_frame.grid_columnconfigure(1, weight=1, minsize=80)
    
    ttk.Label(slider_frame, text="Speed\nrpm").grid(row=0, column=0, pady=(0,4))
    ttk.Label(slider_frame,  text="Torque\nN*m").grid(row=0, column=1, pady=(0,4))

    speed_slider = ttk.Scale(slider_frame, from_=10000, to=0, variable=speed_var, orient="vertical", length=300)
    speed_slider.grid(row=1, column=0, sticky="ns", padx=6, pady=6)
    speed_slider.state(["disabled"])
    speed_slider.bind("<Button-1>", lambda e: speed_slider.focus_set())
    make_focusable_scale(speed_slider, speed_var, step=100)

    torque_slider = ttk.Scale(slider_frame, from_=500, to=0, variable=torque_var, orient="vertical", length=300)
    torque_slider.grid(row=1, column=1, sticky="ns", padx=6, pady=6)
    torque_slider.state(["disabled"])
    torque_slider.bind("<Button-1>", lambda e: torque_slider.focus_set())
    make_focusable_scale(torque_slider, torque_var, step=1.0)

    speed_entry = ttk.Entry(slider_frame, textvariable=speed_var, width=6, state="disabled")
    speed_entry.grid(row=2, column=0, pady=(4,0))
    torque_entry = ttk.Entry(slider_frame, textvariable=torque_var, width=6, state="disabled")
    torque_entry.grid(row=2, column=1, pady=(4,0))


    def update_mode_controls():
        if mode_var.get() == "speed":
            speed_slider.state(["!disabled"])
            speed_entry.config(state="normal")
            torque_slider.state(["disabled"])
            torque_entry.config(state="disabled")
        else:
            speed_slider.state(["disabled"])
            speed_entry.config(state="disabled")
            torque_slider.state(["!disabled"])
            torque_entry.config(state="normal")

    def _on_speed_released(_=None):
        # больше НИЧЕГО не отправляем автоматически
        if mode_var.get() == "speed":
            ui_log(f"[UI] ns изменён локально → нажмите «Отправить»")

    def _on_torque_released(_=None):
        if mode_var.get() == "currents":
            ui_log("[UI] Id/Iq изменены локально → нажмите «Отправить»")

    speed_slider.bind("<ButtonRelease-1>", _on_speed_released)
    torque_slider.bind("<ButtonRelease-1>", _on_torque_released)
    
    # режим уже создан выше
    update_mode_controls()

    ttk.Radiobutton(mode_frame, text="Currents (Id/Iq)", value="currents",
                    variable=mode_var, command=lambda: set_mode("currents"))\
    .grid(row=0, column=0, padx=8, pady=8, sticky="w")

    ttk.Radiobutton(mode_frame, text="Frequency (ns)", value="speed",
                    variable=mode_var, command=lambda: set_mode("speed"))\
    .grid(row=0, column=1, padx=8, pady=8, sticky="w")


     # === Logbook UI ===
    logbook_top = ttk.Frame(log_frame)
    logbook_top.pack(fill="both", expand=True, padx=10, pady=(10,5))

    # toolbar
    lb_toolbar = ttk.Frame(logbook_top)
    lb_toolbar.pack(fill="x", pady=(0,6))

    def toggle_logging():
        ui_log("📒 logging:", "ON" if log_enabled.get() else "OFF")

    ttk.Checkbutton(lb_toolbar, text="Log telemetry", variable=log_enabled,
                    command=toggle_logging).pack(side="left")

    def clear_log():
        log_rows.clear()
        for i in telem_tree.get_children():
            telem_tree.delete(i)
        ui_log("🧹 journal cleared")

    def export_csv():
        # простой экспорт в файл рядом с клиентом
        fname = f"logbook_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        with open(fname, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f, delimiter=";")
            w.writerow(TELEM_COLUMNS)
            for row in log_rows:
                w.writerow([row.get(k, "") for k in TELEM_COLUMNS])
        ui_log(f"💾 exported: {fname}")

    ttk.Button(lb_toolbar, text="Clear", command=clear_log).pack(side="right", padx=4)
    ttk.Button(lb_toolbar, text="Export CSV", command=export_csv).pack(side="right", padx=4)

    # таблица
    columns = TELEM_COLUMNS
    telem_tree = ttk.Treeview(logbook_top, columns=columns, show="headings", height=12)
    for col in columns:
        telem_tree.heading(col, text=col)
        telem_tree.column(col, width=100, anchor="center")

    ys = ttk.Scrollbar(logbook_top, orient="vertical", command=telem_tree.yview)
    telem_tree.configure(yscroll=ys.set)

    telem_tree.pack(side="left", fill="both", expand=True)
    ys.pack(side="right", fill="y")

    # нижняя панель с текстовым логом событий
    log_events = ttk.LabelFrame(log_frame, text="Events")
    log_events.pack(fill="both", expand=True, padx=10, pady=(0,10))
    log_box.pack(in_=log_events, fill="both", padx=6, pady=6, expand=True)

    # Горячие клавиши для журнала
    root.bind_all("<Control-l>", lambda e: log_enabled.set(not log_enabled.get()))
    root.bind_all("<Control-e>", lambda e: export_csv())
    root.bind_all("<Control-Shift-C>", lambda e: clear_log())

    def _init_mode():
        set_mode("currents")   # или "speed"

    # --- NEW: обновление вкладки Maps ---

    # Ld(Id)
    if map_Id and map_Ld:
        # длина по минимальному
        n = min(len(map_Id), len(map_Ld))
        sc_ld.set_data(list(map_Id)[-n:], list(map_Ld)[-n:])
        ax5a.relim(); ax5a.autoscale_view()

    # Lq(Iq)
    if map_Iq and map_Lq:
        n = min(len(map_Iq), len(map_Lq))
        sc_lq.set_data(list(map_Iq)[-n:], list(map_Lq)[-n:])
        ax5b.relim(); ax5b.autoscale_view()

    # Torque & Power vs RPM
    if map_ns:
        xs = list(map_ns)
        if map_Ms:
            ln_torque.set_data(xs[-len(map_Ms):], list(map_Ms))
        if map_Pmech:
            ln_pmech.set_data(xs[-len(map_Pmech):], list(map_Pmech))
        if map_Pelec:
            ln_pelec.set_data(xs[-len(map_Pelec):], list(map_Pelec))

        ax6.relim(); ax6.autoscale_view()
        ax6_right.relim(); ax6_right.autoscale_view()

    canvas_maps.draw_idle()

    root.after(0, _init_mode)

    root.after(500, _update_trends)

        # --- UPDATE Maps ---

        # Ld(Id)
    if map_Id and map_Ld:
        n = min(len(map_Id), len(map_Ld))
        sc_ld.set_data(list(map_Id)[-n:], list(map_Ld)[-n:])
        ax5a.relim(); ax5a.autoscale_view()

        # Lq(Iq)
    if map_Iq and map_Lq:
        n = min(len(map_Iq), len(map_Lq))
        sc_lq.set_data(list(map_Iq)[-n:], list(map_Lq)[-n:])
        ax5b.relim(); ax5b.autoscale_view()

        # Torque & Power vs RPM (общая ось X = rpm)
    if map_rpm:
        xs = list(map_rpm)
        if map_Ms:
            ln_torque.set_data(xs[-len(map_Ms):], list(map_Ms))
        if map_Pmech:
            ln_pmech.set_data(xs[-len(map_Pmech):], list(map_Pmech))
        if map_Pelec:
            ln_pelec.set_data(xs[-len(map_Pelec):], list(map_Pelec))
        ax6.relim(); ax6.autoscale_view()
        ax6_right.relim(); ax6_right.autoscale_view()
    canvas_maps.draw_idle()

    # ------- RPC-мост (агент ↔ GUI) -------
    class AgentRPCServer(socketserver.ThreadingTCPServer):
        allow_reuse_address = True
        def __init__(self, addr, handler, ws_client):
            super().__init__(addr, handler)
            self.ws_client = ws_client

    class AgentRPCHandler(socketserver.StreamRequestHandler):
        def reply(self, obj):
            self.wfile.write((json.dumps(obj, ensure_ascii=False) + "\n").encode("utf-8"))

        def handle(self):
            while True:
                line = self.rfile.readline()
                if not line:
                    break
                try:
                    req = json.loads(line.decode("utf-8"))
                except Exception as e:
                    self.reply({"ok": False, "error": f"bad json: {e}"})
                    continue

                cmd = req.get("cmd")
                try:
                    if cmd == "start":
                        fut = self.server.ws_client.send_cmd_threadsafe("Init"); fut.result(timeout=1)
                        self.reply({"ok": True})
                    elif cmd == "stop":
                        fut = self.server.ws_client.send_cmd_threadsafe("Stop"); fut.result(timeout=1)
                        self.reply({"ok": True})
                    elif cmd == "read2":
                        fut = self.server.ws_client.send_cmd_threadsafe("Read2"); fut.result(timeout=1)
                        time.sleep(0.05)
                        with telemetry_lock:
                            self.reply({"ok": True, "telemetry": dict(last_telemetry)})
                    elif cmd == "set_currents":
                        Isd = float(req.get("Isd", 0.0)); Isq = float(req.get("Isq", 0.0))
                        En_rem = bool(req.get("En_rem", True))
                        payload = {"cmd": "SendTorque", "En_rem": En_rem, "Isd": Isd, "Isq": Isq, "src": "rl-agent"}
                        fut = self.server.ws_client.send_json_threadsafe(payload); fut.result(timeout=1)
                        # >>> обновим GUI-поля и лог из главного потока:
                        root.after(0, lambda: (
                            apply_agent_currents(Isd, Isq, En_rem),
                            ui_log(f"[AGENT] SendTorque Isd={Isd:.3f} Iq={Isq:.3f} En_rem={En_rem}")
                        ))
                        self.reply({"ok": True, "echo": {"Isd": Isd, "Isq": Isq, "En_rem": En_rem}})
                    elif cmd == "snapshot":
                        with telemetry_lock:
                            self.reply({"ok": True, "telemetry": dict(last_telemetry)})
                    else:
                        self.reply({"ok": False, "error": f"unknown cmd: {cmd}"})
                except Exception as e:
                    self.reply({"ok": False, "error": str(e)})

    # Запускаем WS-клиент
    client = WSClient(WS_URL, on_message, on_status, on_error)
    client.start()
    # RPC сервер агента
    def start_agent_bridge(ws_client, host="127.0.0.1", port=8765):
        srv = AgentRPCServer((host, port), AgentRPCHandler, ws_client)
        t = threading.Thread(target=srv.serve_forever, daemon=True)
        t.start()
        ui_log(f"[AgentRPC] listening on {host}:{port}")
        return srv
    agent_bridge = start_agent_bridge(client, port=8765)

    # Лог, бинды и выход
    root.protocol("WM_DELETE_WINDOW", lambda: (client.stop(), root.destroy()))
    root.bind("<Up>", on_arrow_key)
    root.bind("<Down>", on_arrow_key)

    # Вкладка 3: лог
    #log_box.pack(in_=log_frame, fill="both", padx=10, pady=10, expand=True)

    root.mainloop()

if __name__ == "__main__":
    create_gui()
