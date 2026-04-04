# view.py
from __future__ import annotations

import tkinter as tk
from tkinter import ttk, Text
from dataclasses import dataclass
from datetime import datetime
import csv

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# ---- наши модули ----
from state import AppState as State, PAD, TELEM_COLUMNS

try:
    from state import GEAR_MAP
except Exception:
    GEAR_MAP = {"D": 4, "R": 3, "N": 2}


@dataclass
class ViewRefs:
    root: tk.Tk
    style: ttk.Style

    # верхняя панель
    toolbar: ttk.Frame
    conn_pill_wrap: tk.Frame

    # вкладки
    notebook: ttk.Notebook
    main_frame: ttk.Frame
    ind_frame: ttk.Frame
    log_frame: ttk.Frame
    trends_frame: ttk.Frame
    maps_frame: ttk.Frame

    # Control
    controls_container: ttk.Frame
    mode_frame: ttk.LabelFrame
    currents_frame: ttk.LabelFrame
    limits_frame: ttk.LabelFrame
    params_frame: ttk.LabelFrame
    can_frame: ttk.LabelFrame
    voltage_frame: ttk.LabelFrame
    flux_frame: ttk.LabelFrame

    # ЕДИНЫЙ ползунок слева
    slider_frame: ttk.Frame
    main_slider: ttk.Scale
    main_entry: ttk.Entry  # фактически Spinbox, но тип для удобства

    # Logbook
    telem_tree: ttk.Treeview
    log_box: Text

    # Trends (оси/линии)
    fig_trends: Figure
    canvas_trends: FigureCanvasTkAgg
    ax1: any; l_ns: any
    ax2: any; l_ms: any
    ax3: any; l_idc: any; l_isd: any
    ax4: any; l_id: any; l_iq: any; l_ud: any; l_uq: any

    # Maps
    fig_maps: Figure
    canvas_maps: FigureCanvasTkAgg
    ax5a: any; sc_ld: any
    ax5b: any; sc_lq: any
    ax6: any; ax6_right: any; ln_torque: any; ln_pmech: any; ln_pelec: any


# --- вспомогательные для «активного ползунка» (стрелки ↑/↓) ---
_active_scale: tuple[ttk.Scale, tk.Variable, float] | None = None


# Spinbox с удобными стрелками и шорткатами
def _make_num_spin(parent, var, from_=-1000.0, to=1000.0, step=0.1, width=10):
    try:
        sp = ttk.Spinbox(parent, textvariable=var, from_=from_, to=to, increment=step,
                         width=width, justify="right")
    except Exception:
        sp = tk.Spinbox(parent, textvariable=var, from_=from_, to=to, increment=step,
                        width=width, justify="right")

    def _nudge(delta):
        try:
            val = float(var.get() or 0.0)
        except Exception:
            val = 0.0
        newv = max(from_, min(to, val + delta))
        var.set(f"{newv:.3f}")

    sp.bind("<Shift-Up>",   lambda e: (_nudge(step*10), "break")[1])
    sp.bind("<Shift-Down>", lambda e: (_nudge(-step*10), "break")[1])
    sp.bind("<Return>",     lambda e: (_nudge(0.0), "break")[1])
    return sp


def _bind_spin_steps(spin, var, step: float):
    # Перепривязываем шорткаты под текущий шаг
    for seq in ("<Shift-Up>", "<Shift-Down>", "<Return>"):
        try:
            spin.unbind(seq)
        except Exception:
            pass

    def _nudge(delta):
        try:
            val = float(var.get() or 0.0)
        except Exception:
            val = 0.0
        var.set(f"{val + delta:.3f}")

    spin.bind("<Shift-Up>",   lambda e: (_nudge(step*10), "break")[1])
    spin.bind("<Shift-Down>", lambda e: (_nudge(-step*10), "break")[1])
    spin.bind("<Return>",     lambda e: (_nudge(0.0), "break")[1])


def _make_focusable_scale(scale: ttk.Scale, var: tk.Variable, step: float = 1.0):
    def on_click(_):
        global _active_scale
        _active_scale = (scale, var, step)
        scale.focus_set()
    scale.bind("<Button-1>", on_click)


def _on_arrow_key(event):
    global _active_scale
    if _active_scale is None:
        return
    scale, var, step = _active_scale
    try:
        val = float(var.get())
    except Exception:
        return
    if event.keysym == "Up":
        var.set(val + step)
    elif event.keysym == "Down":
        var.set(val - step)


# =========================
#     СБОРКА ИНТЕРФЕЙСА
# =========================
def build_ui(root, state: State, handlers) -> ViewRefs:
    """
    Создаёт весь UI. Все обработчики — из dict `handlers` (controllers.handlers()).
    """
    # 1) Стиль уже инициализирован в app.py; возьмём текущий
    style = ttk.Style()

    # 2) Переменные (если не были созданы в state — создаём тут и сохраняем в state)
    sv = lambda cur=None, default="": cur if isinstance(cur, tk.StringVar) else tk.StringVar(value=default, master=root)
    dv = lambda cur=None, default=0.0: cur if isinstance(cur, tk.DoubleVar) else tk.DoubleVar(value=default, master=root)
    bv = lambda cur=None, default=False: cur if isinstance(cur, tk.BooleanVar) else tk.BooleanVar(value=default, master=root)
    iv = lambda cur=None, default=0: cur if isinstance(cur, tk.IntVar) else tk.IntVar(value=default, master=root)

    state.conn_var   = sv(state.conn_var, "disabled")
    state.conn_color = sv(state.conn_color, "#d72c20")

    state.mode_var = sv(state.mode_var, "currents")
    state.gear_var = sv(state.gear_var, "N")

    state.Id_var = sv(state.Id_var, "-0.5")
    state.Iq_var = sv(state.Iq_var, "0.0")

    state.speed_var  = dv(state.speed_var, 0.0)
    state.torque_var = dv(state.torque_var, 0.0)

    state.M_min_var      = sv(state.M_min_var, "-50.0")
    state.M_max_var      = sv(state.M_max_var, "400.0")
    state.M_grad_max_var = sv(state.M_grad_max_var, "50")
    state.n_max_var      = sv(state.n_max_var, "1000")

    state.auto_delay_s_var = dv(getattr(state, "auto_delay_s_var", None), 0.5)
    state.auto_status_var  = sv(getattr(state, "auto_status_var", None), "idle")
    state.auto_points_var  = sv(getattr(state, "auto_points_var", None), "0")

    # массивы строк для CAN (12 полей: id, data0..7, len, flags, ts)
    if not getattr(state, "can_rx_data", None) or len(state.can_rx_data) != 12:
        state.can_rx_data = [sv(master=root) for _ in range(12)]
    if not getattr(state, "can_tx_data", None) or len(state.can_tx_data) != 12:
        state.can_tx_data = [sv(master=root) for _ in range(12)]

    state.log_enabled = bv(state.log_enabled, True)
    state.log_rows = getattr(state, "log_rows", []) or []
    state.max_rows = getattr(state, "max_rows", 5000) or 5000

    # 3) Toolbar
    toolbar = ttk.Frame(root, style="Toolbar.TFrame")
    toolbar.pack(fill="x")

    ttk.Button(toolbar, text="Send", style="Accent.TButton",
               command=handlers.get("send_all", lambda: None)).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="▶ Start", width=14,
               command=lambda: handlers.get("send_cmd", lambda *_: None)("Init")).pack(side="left", padx=(PAD, 4), pady=PAD)
    ttk.Button(toolbar, text="■ Stop", width=14,
               command=lambda: handlers.get("send_cmd", lambda *_: None)("Stop")).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="↺ Reset", width=14,
               command=lambda: handlers.get("send_cmd", lambda *_: None)("Read2")).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="💾 Save", width=14,
               command=lambda: handlers.get("send_cmd", lambda *_: None)("SaveCfg")).pack(side="left", padx=4, pady=PAD)

    # поле адреса WS (host:port или ws://host:port)
    state.ws_addr_var = sv(getattr(state, "ws_addr_var", None), "192.168.8.100:9000")

    pill_wrap = _make_pill(toolbar, state.conn_var, state.conn_color, style)
    pill_wrap.pack(side="right", padx=6, pady=6)

    # Connect UI справа (перед статусом)
    connect_btn = ttk.Button(
        toolbar,
        text="Connect",
        command=handlers.get("connect_ws", lambda: None),
    )
    connect_btn.pack(side="right", padx=(6, 4), pady=PAD)

    ws_entry = ttk.Entry(toolbar, textvariable=state.ws_addr_var, width=24)
    ws_entry.pack(side="right", padx=(6, 0), pady=PAD)

    # Enter в поле = Connect
    ws_entry.bind("<Return>", lambda e: (handlers.get("connect_ws", lambda: None)(), "break")[1])


    # 4) Вкладки
    notebook = ttk.Notebook(root); notebook.pack(fill="both", expand=True)
    main_frame   = ttk.Frame(notebook); notebook.add(main_frame, text="Control")
    ind_frame    = ttk.Frame(notebook); notebook.add(ind_frame,  text="Indication")
    log_frame    = ttk.Frame(notebook); notebook.add(log_frame,  text="Logbook")
    trends_frame = ttk.Frame(notebook); notebook.add(trends_frame, text="Trends")
    maps_frame   = ttk.Frame(notebook); notebook.add(maps_frame,   text="Maps")
    auto_frame = ttk.Frame(notebook); notebook.add(auto_frame, text="AutoCal")

    # === Control ===
    main_inner = ttk.Frame(main_frame)
    main_inner.pack(fill="both", expand=True, padx=10, pady=10)
    # 0-я колонка — для вертикального ползунка (узкая)
    main_inner.grid_columnconfigure(0, weight=0, minsize=120)
    # 1–2 колонки — остальной контент
    main_inner.grid_columnconfigure(1, weight=1)
    main_inner.grid_columnconfigure(2, weight=1)

    # верхняя «карточка» управления
    controls_container = ttk.Frame(main_inner, style="Card.TFrame")
    controls_container.grid(row=0, column=1, columnspan=2, sticky="ew", padx=(0,10), pady=(0,10))

    # Gear
    gear_frame = ttk.LabelFrame(controls_container, text="Gear")
    gear_frame.pack(side="left", padx=10, pady=10)
    for i, g in enumerate(("D", "R", "N")):
        ttk.Radiobutton(gear_frame, text=g, value=g, variable=state.gear_var).grid(row=0, column=i, padx=6, pady=6)

    # Mode
    mode_frame = ttk.LabelFrame(controls_container, text="Mode")
    mode_frame.pack(side="left", padx=10, pady=10)

    # --- ЛЕВАЯ КОЛОНКА: единый ползунок + SPINBOX cо стрелками ---
    slider_frame = ttk.Frame(main_inner, width=180, height=450)
    slider_frame.grid(row=0, column=0, rowspan=6, padx=(10, 10), pady=10, sticky="ns")
    slider_frame.pack_propagate(False)

    slider_title = ttk.Label(slider_frame, text="", justify="center")
    slider_title.grid(row=0, column=0, pady=(0, 4))

    main_slider = ttk.Scale(slider_frame, orient="vertical", length=300)
    main_slider.grid(row=1, column=0, sticky="ns", padx=6, pady=6)
    main_slider.bind("<Button-1>", lambda e: main_slider.focus_set())

    # Spinbox под ползунком (стрелочки ↑/↓)
    try:
        main_entry = ttk.Spinbox(slider_frame, width=8, justify="right")
    except Exception:
        main_entry = tk.Spinbox(slider_frame, width=8, justify="right")
    main_entry.grid(row=2, column=0, pady=(4, 0))

    # Горячие клавиши ↑/↓ для активного слайдера
    root.bind("<Up>", _on_arrow_key)
    root.bind("<Down>", _on_arrow_key)

    # Функция, которая перенастраивает ползунок и SPINBOX под режим
    def _configure_main_slider(mode: str):
        if mode == "speed":
            slider_title.configure(text="Speed\nrpm")
            main_slider.configure(from_=20000, to=0, variable=state.speed_var)
            _make_focusable_scale(main_slider, state.speed_var, step=1.0)

            # настроим spinbox
            try:
                main_entry.configure(textvariable=state.speed_var, from_=0, to=5000, increment=1.0)
            except Exception:
                main_entry.config(textvariable=state.speed_var, from_=0, to=5000, increment=1.0)
            _bind_spin_steps(main_entry, state.speed_var, step=1.0)

            def _on_release(_=None):
                _ui_log(state, "[UI] ns изменён локально → нажмите «Отправить»")
            main_slider.unbind("<ButtonRelease-1>")
            main_slider.bind("<ButtonRelease-1>", _on_release)

        else:  # torque или currents
            slider_title.configure(text="Torque\nN·m")
            main_slider.configure(from_=500, to=0, variable=state.torque_var)
            _make_focusable_scale(main_slider, state.torque_var, step=1.0)

            try:
                main_entry.configure(textvariable=state.torque_var, from_=0, to=500, increment=1.0)
            except Exception:
                main_entry.config(textvariable=state.torque_var, from_=0, to=500, increment=1.0)
            _bind_spin_steps(main_entry, state.torque_var, step=1.0)

            def _on_release(_=None):
                _ui_log(state, "[UI] Ms изменён локально → нажмите «Отправить»")
            main_slider.unbind("<ButtonRelease-1>")
            main_slider.bind("<ButtonRelease-1>", _on_release)

    # Радиокнопки режимов (сообщаем контроллеру и сразу переконфигурируем слайдер/спинбокс)
    def _on_mode_pick(val):
        handlers.get("set_mode", lambda *_: None)(val)
        _configure_main_slider(val)

    ttk.Radiobutton(mode_frame, text="Torque (Ms)", value="torque",
                    variable=state.mode_var, command=lambda: _on_mode_pick("torque")).grid(row=0, column=0, padx=8, pady=8, sticky="w")
    ttk.Radiobutton(mode_frame, text="Currents (Id/Iq)", value="currents",
                    variable=state.mode_var, command=lambda: _on_mode_pick("currents")).grid(row=0, column=1, padx=8, pady=8, sticky="w")
    ttk.Radiobutton(mode_frame, text="Frequency (ns)", value="speed",
                    variable=state.mode_var, command=lambda: _on_mode_pick("speed")).grid(row=0, column=2, padx=8, pady=8, sticky="w")
    # Currents
    currents_frame = ttk.LabelFrame(main_inner, text="Currents")
    currents_frame.grid(row=1, column=1, padx=(0,10), pady=10, sticky="nsew")
    ttk.Label(currents_frame, text="Id [A]").grid(row=0, column=0, sticky="e", padx=6, pady=6)
    _make_num_spin(currents_frame, state.Id_var, from_=-1000.0, to=1000.0, step=0.1, width=10)\
        .grid(row=0, column=1, sticky="w")
    ttk.Label(currents_frame, text="Iq [A]").grid(row=0, column=2, sticky="e", padx=6, pady=6)
    _make_num_spin(currents_frame, state.Iq_var, from_=-1000.0, to=1000.0, step=0.1, width=10)\
        .grid(row=0, column=3, sticky="w")

    # Limits
    limits_frame = ttk.LabelFrame(main_inner, text="Limits")
    limits_frame.grid(row=1, column=2, padx=(0,10), pady=10, sticky="nsew")
    _labent(limits_frame, 0, 0, "M_min [Н·м]", state.M_min_var)
    _labent(limits_frame, 0, 2, "M_max [Н·м]", state.M_max_var)
    _labent(limits_frame, 1, 0, "M_grad_max",  state.M_grad_max_var)
    _labent(limits_frame, 1, 2, "n_max [rpm]", state.n_max_var)

    # Параметры стенда (поля-отображение)
    params_frame = ttk.LabelFrame(main_inner, text="MCU_VCU_parameters")
    params_frame.grid(row=2, column=1, columnspan=2, padx=(0,10), pady=0, sticky="nsew")
    state.entry_vars = getattr(state, "entry_vars", {}) or {}
    for i, param in enumerate([
        "Speed rotation",
        "Torque (Ms)",
        "direct current (Idc)",
        "Stator current d (Isd)",
        "IGBT temperature",
        "Stator temperature",
    ]):
        ttk.Label(params_frame, text=param + ":").grid(row=i, column=0, sticky="e", padx=5, pady=5)
        var = state.entry_vars.get(param) or tk.StringVar(master=root)
        state.entry_vars[param] = var
        ttk.Entry(params_frame, textvariable=var, width=20).grid(row=i, column=1, padx=5, pady=5)

    # CAN Tx/Rx (12 полей: id, data0..7, len, flags, ts)
    can_frame = ttk.LabelFrame(main_inner, text="Tx / Rx CAN")
    can_frame.grid(row=3, column=1, columnspan=2, padx=(0,10), pady=10, sticky="nsew")
    headers = ["id"] + [f"data{i}" for i in range(8)] + ["len", "flags", "ts"]
    for col, header in enumerate(headers):
        ttk.Label(can_frame, text=header, anchor="center", width=8).grid(row=0, column=col+1, padx=2, pady=(0,5))
    ttk.Label(can_frame, text="Tx:").grid(row=1, column=0, sticky="e", padx=3)
    ttk.Label(can_frame, text="Rx:").grid(row=2, column=0, sticky="e", padx=3)
    for col in range(12):
        Entry(can_frame, textvariable=state.can_tx_data[col], width=8, justify="center", state="readonly")\
            .grid(row=1, column=col+1, padx=2, pady=2)
    for col in range(12):
        Entry(can_frame, textvariable=state.can_rx_data[col], width=8, justify="center", state="readonly")\
            .grid(row=2, column=col+1, padx=2, pady=2)

    # MCU Current & Voltage
    voltage_frame = ttk.LabelFrame(main_inner, text="MCU Current & Voltage")
    voltage_frame.grid(row=4, column=1, padx=(0,10), pady=10, sticky="nsew")
    for i, param in enumerate(["Ud", "Uq", "Id", "Iq"]):
        ttk.Label(voltage_frame, text=param + ":").grid(row=i, column=0, sticky="e", padx=5, pady=3)
        var = state.entry_vars.get(param) or tk.StringVar(master=root)
        state.entry_vars[param] = var
        ttk.Entry(voltage_frame, textvariable=var, width=15).grid(row=i, column=1, padx=5, pady=3)

    # MCU Flux Parameters
    flux_frame = ttk.LabelFrame(main_inner, text="MCU Flux Parameters")
    flux_frame.grid(row=4, column=2, padx=(0,10), pady=10, sticky="nsew")
    for i, param in enumerate(["Emf", "Welectrical", "motorRs", "Wmechanical"]):
        ttk.Label(flux_frame, text=param + ":").grid(row=i, column=0, sticky="e", padx=5, pady=3)
        var = state.entry_vars.get(param) or tk.StringVar(master=root)
        state.entry_vars[param] = var
        ttk.Entry(flux_frame, textvariable=var, width=15).grid(row=i, column=1, padx=5, pady=3)

    # === AutoCal ===
    auto_inner = ttk.Frame(auto_frame)
    auto_inner.pack(fill="both", expand=True, padx=10, pady=10)

    auto_left = ttk.Frame(auto_inner, style="Card.TFrame")
    auto_left.pack(side="left", fill="y", padx=(0, 10))

    auto_limits = ttk.LabelFrame(auto_left, text="Limits")
    auto_limits.pack(fill="x", padx=10, pady=(10, 8))
    _labent(auto_limits, 0, 0, "M_min [Н·м]", state.M_min_var)
    _labent(auto_limits, 0, 2, "M_max [Н·м]", state.M_max_var)
    _labent(auto_limits, 1, 0, "M_grad_max",  state.M_grad_max_var)
    _labent(auto_limits, 1, 2, "n_max [rpm]", state.n_max_var)

    auto_ctl = ttk.LabelFrame(auto_left, text="Auto calibration")
    auto_ctl.pack(fill="x", padx=10, pady=(0, 10))

    ttk.Button(auto_ctl, text="Load XLSX",
            command=handlers.get("auto_load_table", lambda: None)).grid(row=0, column=0, padx=6, pady=6, sticky="ew")
    ttk.Button(auto_ctl, text="Delay…",
            command=handlers.get("auto_delay_dialog", lambda: None)).grid(row=0, column=1, padx=6, pady=6, sticky="ew")

    ttk.Button(auto_ctl, text="▶ Start",
            command=handlers.get("auto_start", lambda: None)).grid(row=1, column=0, padx=6, pady=6, sticky="ew")
    ttk.Button(auto_ctl, text="■ Stop",
            command=handlers.get("auto_stop", lambda: None)).grid(row=1, column=1, padx=6, pady=6, sticky="ew")

    ttk.Label(auto_ctl, text="Delay (s):").grid(row=2, column=0, sticky="e", padx=6, pady=4)
    ttk.Label(auto_ctl, textvariable=state.auto_delay_s_var).grid(row=2, column=1, sticky="w", padx=6, pady=4)

    ttk.Label(auto_ctl, text="Points:").grid(row=3, column=0, sticky="e", padx=6, pady=4)
    ttk.Label(auto_ctl, textvariable=state.auto_points_var).grid(row=3, column=1, sticky="w", padx=6, pady=4)

    ttk.Label(auto_ctl, text="Status:").grid(row=4, column=0, sticky="e", padx=6, pady=4)
    ttk.Label(auto_ctl, textvariable=state.auto_status_var).grid(row=4, column=1, sticky="w", padx=6, pady=4)

    auto_ctl.grid_columnconfigure(0, weight=1)
    auto_ctl.grid_columnconfigure(1, weight=1)

    auto_right = ttk.Frame(auto_inner, style="Card.TFrame")
    auto_right.pack(side="left", fill="both", expand=True)

    ttk.Label(auto_right, text="LookupTable preview (Id/Iq)").pack(anchor="w", padx=10, pady=(10, 0))

    auto_tree = ttk.Treeview(auto_right, columns=("idx", "Id", "Iq"), show="headings", height=20)
    auto_tree.heading("idx", text="#")
    auto_tree.heading("Id", text="Id_A")
    auto_tree.heading("Iq", text="Iq_A")
    auto_tree.column("idx", width=60, anchor="center")
    auto_tree.column("Id",  width=120, anchor="center")
    auto_tree.column("Iq",  width=120, anchor="center")

    auto_ys = ttk.Scrollbar(auto_right, orient="vertical", command=auto_tree.yview)
    auto_tree.configure(yscroll=auto_ys.set)

    auto_tree.pack(side="left", fill="both", expand=True, padx=10, pady=10)
    auto_ys.pack(side="right", fill="y", pady=10)


    # === Logbook ===
    logbook_top = ttk.Frame(log_frame); logbook_top.pack(fill="both", expand=True, padx=10, pady=(10,5))

    lb_toolbar = ttk.Frame(logbook_top); lb_toolbar.pack(fill="x", pady=(0,6))
    ttk.Checkbutton(lb_toolbar, text="Log telemetry", variable=state.log_enabled,
                    command=lambda: _ui_log(state, f"📒 logging: {'ON' if state.log_enabled.get() else 'OFF'}")
                    ).pack(side="left")
    ttk.Button(lb_toolbar, text="Clear",
               command=handlers.get("clear_log", lambda: _clear_log_default(state))).pack(side="right", padx=4)
    ttk.Button(lb_toolbar, text="Export CSV",
               command=handlers.get("export_csv", lambda: _export_csv_default(state))).pack(side="right", padx=4)

    telem_tree = ttk.Treeview(logbook_top, columns=TELEM_COLUMNS, show="headings", height=12)
    for col in TELEM_COLUMNS:
        telem_tree.heading(col, text=col)
        telem_tree.column(col, width=100, anchor="center")
    ys = ttk.Scrollbar(logbook_top, orient="vertical", command=telem_tree.yview)
    telem_tree.configure(yscroll=ys.set)
    telem_tree.pack(side="left", fill="both", expand=True)
    ys.pack(side="right", fill="y")
    state.telem_tree = telem_tree

    log_events = ttk.LabelFrame(log_frame, text="Events")
    log_events.pack(fill="both", expand=True, padx=10, pady=(0,10))
    log_box = Text(log_events, height=8, wrap="word")
    log_box.pack(fill="both", padx=6, pady=6, expand=True)
    state.log_box = log_box

    root.bind_all("<Control-l>", lambda e: state.log_enabled.set(not state.log_enabled.get()))
    root.bind_all("<Control-e>", lambda e: handlers.get("export_csv", lambda: _export_csv_default(state))())
    root.bind_all("<Control-Shift-C>", lambda e: handlers.get("clear_log", lambda: _clear_log_default(state))())

    # === Trends ===
    trends_container = ttk.Frame(trends_frame); trends_container.pack(fill="both", expand=True, padx=10, pady=10)

    fig_trends = Figure(figsize=(8, 5), dpi=100)
    ax1 = fig_trends.add_subplot(221); ax1.set_title("ns (rpm)"); ax1.grid(True)
    ax2 = fig_trends.add_subplot(222); ax2.set_title("Ms (N·m)"); ax2.grid(True)
    ax3 = fig_trends.add_subplot(223); ax3.set_title("Idc/Isd (A)"); ax3.grid(True)
    ax4 = fig_trends.add_subplot(224); ax4.set_title("Id/Iq & Ud/Uq"); ax4.grid(True)

    l_ns,   = ax1.plot([], [])
    l_ms,   = ax2.plot([], [])
    l_idc,  = ax3.plot([], [], label="Idc")
    l_isd,  = ax3.plot([], [], label="Isd"); ax3.legend()

    l_id,   = ax4.plot([], [], label="Id")
    l_iq,   = ax4.plot([], [], label="Iq")
    l_ud,   = ax4.plot([], [], label="Ud", linestyle="--")
    l_uq,   = ax4.plot([], [], label="Uq", linestyle="--"); ax4.legend()

    canvas_trends = FigureCanvasTkAgg(fig_trends, master=trends_container)
    canvas_trends.get_tk_widget().pack(fill="both", expand=True)

    # === Maps ===
    maps_container = ttk.Frame(maps_frame); maps_container.pack(fill="both", expand=True, padx=10, pady=10)

    fig_maps = Figure(figsize=(8, 5), dpi=100)
    ax5a = fig_maps.add_subplot(221)  # Ld(Id)
    ax5b = fig_maps.add_subplot(222)  # Lq(Iq)
    ax6  = fig_maps.add_subplot(212)  # Torque & Power vs RPM

    ax5a.set_title("Ld vs Id"); ax5a.set_xlabel("Id, A"); ax5a.set_ylabel("Ld, H"); ax5a.grid(True)
    ax5b.set_title("Lq vs Iq"); ax5b.set_xlabel("Iq, A"); ax5b.set_ylabel("Lq, H"); ax5b.grid(True)
    ax6.set_title("Torque & Power vs RPM"); ax6.set_xlabel("RPM"); ax6.grid(True)
    ax6_right = ax6.twinx(); ax6.set_ylabel("Torque, N·m"); ax6_right.set_ylabel("Power, kW")

    sc_ld = ax5a.plot([], [], linestyle="", marker=".", markersize=3)[0]
    sc_lq = ax5b.plot([], [], linestyle="", marker=".", markersize=3)[0]
    ln_torque, = ax6.plot([], [], label="Torque (N·m)")
    ln_pmech,  = ax6_right.plot([], [], label="P_mech (kW)")
    ln_pelec,  = ax6_right.plot([], [], label="P_elec (kW)", linestyle="--")
    ax6.legend([ln_torque, ln_pmech, ln_pelec], ["Torque (N·m)", "P_mech (kW)", "P_elec (kW)"])

    canvas_maps = FigureCanvasTkAgg(fig_maps, master=maps_container)
    canvas_maps.get_tk_widget().pack(fill="both", expand=True)

    # стартовая конфигурация единого ползунка + спинбокса
    _configure_main_slider(state.mode_var.get())

    # Выдаём ссылки на графики/оси в state — чтобы контроллер мог обновлять
    state.trends = {
        "figure": fig_trends,
        "canvas": canvas_trends,

        # список осей
        "axes": [ax1, ax2, ax3, ax4],

        # список линий в ожидаемом порядке:
        # 0: ns, 1: Ms, 2: Idc, 3: Isd, 4: Id, 5: Iq, 6: Ud, 7: Uq
        "lines": [l_ns, l_ms, l_idc, l_isd, l_id, l_iq, l_ud, l_uq],

        # именные ссылки — просто для удобства (можешь оставить или убрать)
        "ax1": ax1, "ax2": ax2, "ax3": ax3, "ax4": ax4,
        "l_ns": l_ns, "l_ms": l_ms,
        "l_idc": l_idc, "l_isd": l_isd,
        "l_id": l_id, "l_iq": l_iq, "l_ud": l_ud, "l_uq": l_uq,
    }
    state.maps = {
        "figure": fig_maps, "canvas": canvas_maps,
        "ax5a": ax5a, "sc_ld": sc_ld,
        "ax5b": ax5b, "sc_lq": sc_lq,
        "ax6": ax6, "ax6_right": ax6_right,
        "ln_torque": ln_torque, "ln_pmech": ln_pmech, "ln_pelec": ln_pelec,
    }

    view = ViewRefs(
        root=root, style=style,
        toolbar=toolbar, conn_pill_wrap=pill_wrap,
        notebook=notebook, main_frame=main_frame, ind_frame=ind_frame, log_frame=log_frame,
        trends_frame=trends_frame, maps_frame=maps_frame,
        controls_container=controls_container, mode_frame=mode_frame, currents_frame=currents_frame,
        limits_frame=limits_frame, params_frame=params_frame, can_frame=can_frame,
        voltage_frame=voltage_frame, flux_frame=flux_frame,
        slider_frame=slider_frame, main_slider=main_slider, main_entry=main_entry,
        telem_tree=telem_tree, log_box=log_box,
        fig_trends=fig_trends, canvas_trends=canvas_trends,
        ax1=ax1, l_ns=l_ns, ax2=ax2, l_ms=l_ms, ax3=ax3, l_idc=l_idc, l_isd=l_isd,
        ax4=ax4, l_id=l_id, l_iq=l_iq, l_ud=l_ud, l_uq=l_uq,
        fig_maps=fig_maps, canvas_maps=canvas_maps,
        ax5a=ax5a, sc_ld=sc_ld, ax5b=ax5b, sc_lq=sc_lq,
        ax6=ax6, ax6_right=ax6_right, ln_torque=ln_torque, ln_pmech=ln_pmech, ln_pelec=ln_pelec
    )

    # Для совместимости с контроллером — оба «виртуальных» ключа указывают на один виджет
    view.widgets = {
        "speed_slider":  main_slider,
        "speed_entry":   main_entry,
        "torque_slider": main_slider,
        "torque_entry":  main_entry,
    }
    # опционально: дать контроллеру прямой вызов
    view.configure_main_slider = _configure_main_slider

    # если контроллер хочет что-то сделать после сборки view
    after_hook = handlers.get("after_view_built")
    if callable(after_hook):
        try:
            after_hook(view, state)
        except Exception:
            pass

    return view


# =============== вспомогательные функции UI ===============
def _labent(parent: ttk.Frame, r: int, c: int, text: str, var: tk.Variable):
    ttk.Label(parent, text=text).grid(row=r, column=c, sticky="e", padx=6, pady=6)
    ttk.Entry(parent, width=10, textvariable=var).grid(row=r, column=c+1, sticky="w")


def _make_pill(parent, textvar: tk.StringVar, colorvar: tk.StringVar, style: ttk.Style) -> tk.Frame:
    wrap = tk.Frame(parent, bg=style.lookup("Toolbar.TFrame", "background"))
    dot = tk.Canvas(wrap, width=10, height=10, highlightthickness=0,
                    bg=style.lookup("Toolbar.TFrame", "background"))
    oval = dot.create_oval(2, 2, 8, 8, fill=colorvar.get(), outline="")
    lbl = ttk.Label(wrap, textvariable=textvar)
    dot.grid(row=0, column=0, padx=(0, 6), pady=6)
    lbl.grid(row=0, column=1, pady=6)

    def _sync_color(*_):
        try:
            dot.itemconfig(oval, fill=colorvar.get())
        except Exception:
            pass

    colorvar.trace_add("write", lambda *_: _sync_color())
    return wrap


def _ui_log(state: State, msg: str):
    if not getattr(state, "log_box", None):
        return
    state.log_box.insert("end", f"{datetime.now().strftime('%H:%M:%S')} {msg}\n")
    state.log_box.see("end")


def _clear_log_default(state: State):
    state.log_rows.clear()
    if getattr(state, "telem_tree", None):
        for i in state.telem_tree.get_children():
            state.telem_tree.delete(i)
    _ui_log(state, "🧹 journal cleared")


def _export_csv_default(state: State):
    fname = f"logbook_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    with open(fname, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(TELEM_COLUMNS)
        for row in state.log_rows:
            w.writerow([row.get(k, "") for k in TELEM_COLUMNS])
    _ui_log(state, f"💾 exported: {fname}")


# удобный Getter для Entry (простой алиас, чтобы не импортировать напрямую вверху)
def Entry(parent, **kwargs):
    return ttk.Entry(parent, **kwargs)
