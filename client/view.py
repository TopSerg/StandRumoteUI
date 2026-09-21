# view.py
from __future__ import annotations

import tkinter as tk
from tkinter import ttk, Text
from dataclasses import dataclass
from datetime import datetime
import csv

# ---- наши модули ----
from state import (
    AppState as State,
    PAD,
    TELEM_COLUMNS,
    FIELD_SPECS,
    CONTROL_MONITOR_FIELDS,
    CURRENT_VOLTAGE_FIELDS,
    FLUX_FIELDS,
    INDICATION_GROUPS,
)

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
    signals_frame: ttk.Frame
    resolver_frame: ttk.Frame

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
    rx_signal_tree: ttk.Treeview
    tx_signal_tree: ttk.Treeview

    # Trends (оси/линии)
    fig_trends: any
    canvas_trends: any
    ax1: any; l_ns: any
    ax2: any; l_ms: any
    ax3: any; l_idc: any; l_isd: any
    ax4: any; l_id: any; l_iq: any; l_ud: any; l_uq: any

    # Maps
    fig_maps: any
    canvas_maps: any
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


def _field_label(key: str) -> str:
    title, unit, _digits = FIELD_SPECS.get(key, (key, "", 3))
    return f"{title} [{unit}]" if unit else title


def _build_telemetry_fields(parent, state: State, keys, columns: int = 1):
    """Create read-only fields bound by canonical server JSON key."""
    keys = tuple(keys)
    rows_per_column = max(1, (len(keys) + columns - 1) // columns)
    for index, key in enumerate(keys):
        block = index // rows_per_column
        row = index % rows_per_column
        label_col = block * 2
        value_col = label_col + 1
        ttk.Label(parent, text=_field_label(key) + ":").grid(
            row=row, column=label_col, sticky="e", padx=(8, 4), pady=4
        )
        var = state.entry_vars.get(key)
        if var is None:
            var = tk.StringVar(master=parent, value="—")
            state.entry_vars[key] = var
        ttk.Entry(parent, textvariable=var, width=18, state="readonly").grid(
            row=row, column=value_col, sticky="ew", padx=(0, 8), pady=4
        )
        parent.grid_columnconfigure(value_col, weight=1)


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

    state.Id_var = sv(state.Id_var, "0.0")
    state.Iq_var = sv(state.Iq_var, "0.0")
    state.torque_meter_var = sv(getattr(state, "torque_meter_var", None), "")
    state.control_arm_status_var = sv(getattr(state, "control_arm_status_var", None), "DISARMED")

    state.speed_var  = dv(state.speed_var, 0.0)
    state.torque_var = dv(state.torque_var, 0.0)

    state.M_min_var      = sv(state.M_min_var, "-50.0")
    state.M_max_var      = sv(state.M_max_var, "400.0")
    state.M_grad_max_var = sv(state.M_grad_max_var, "50")
    state.n_max_var      = sv(state.n_max_var, "500")

    state.auto_delay_s_var = dv(getattr(state, "auto_delay_s_var", None), 0.5)
    state.auto_status_var  = sv(getattr(state, "auto_status_var", None), "idle")
    state.auto_points_var  = sv(getattr(state, "auto_points_var", None), "0")
    state.json_period_ms_var = sv(getattr(state, "json_period_ms_var", None), "500")
    state.resolver_mode_var = sv(getattr(state, "resolver_mode_var", None), "STOPPED")
    state.resolver_sine_var = sv(getattr(state, "resolver_sine_var", None), "—")
    state.resolver_cosine_var = sv(getattr(state, "resolver_cosine_var", None), "—")
    state.resolver_amplitude_var = sv(getattr(state, "resolver_amplitude_var", None), "—")
    state.resolver_theta_var = sv(getattr(state, "resolver_theta_var", None), "—")
    state.resolver_theta_corr_var = sv(getattr(state, "resolver_theta_corr_var", None), "—")
    state.resolver_capture_count_var = sv(getattr(state, "resolver_capture_count_var", None), "0")
    state.resolver_sine_offset_var = sv(getattr(state, "resolver_sine_offset_var", None), "—")
    state.resolver_cosine_offset_var = sv(getattr(state, "resolver_cosine_offset_var", None), "—")
    state.resolver_sine_amplitude_var = sv(getattr(state, "resolver_sine_amplitude_var", None), "—")
    state.resolver_cosine_amplitude_var = sv(getattr(state, "resolver_cosine_amplitude_var", None), "—")
    state.resolver_gain_ratio_var = sv(getattr(state, "resolver_gain_ratio_var", None), "—")
    state.resolver_flux_error_var = sv(getattr(state, "resolver_flux_error_var", None), "—")
    state.resolver_theta_correction_var = sv(getattr(state, "resolver_theta_correction_var", None), "—")
    state.resolver_electrical_speed_var = sv(getattr(state, "resolver_electrical_speed_var", None), "—")
    state.resolver_auto_state_var = sv(getattr(state, "resolver_auto_state_var", None), "idle")
    state.resolver_flux_valid_var = sv(getattr(state, "resolver_flux_valid_var", None), "no")
    state.resolver_command_active_var = sv(getattr(state, "resolver_command_active_var", None), "no")
    state.resolver_ack_sequence_var = sv(getattr(state, "resolver_ack_sequence_var", None), "—")
    state.resolver_command_var = sv(getattr(state, "resolver_command_var", None), "—")
    state.resolver_loop_error_var = sv(getattr(state, "resolver_loop_error_var", None), "—")
    state.resolver_status_count_var = sv(getattr(state, "resolver_status_count_var", None), "0")
    state.resolver_command_sequence_var = sv(getattr(state, "resolver_command_sequence_var", None), "—")
    state.resolver_enable_command_var = sv(getattr(state, "resolver_enable_command_var", None), "no")
    state.resolver_active_var = sv(getattr(state, "resolver_active_var", None), "no")
    state.resolver_converged_var = sv(getattr(state, "resolver_converged_var", None), "no")
    state.resolver_auto_gain_var = sv(getattr(state, "resolver_auto_gain_var", None), "0.20")
    state.resolver_auto_tolerance_var = sv(getattr(state, "resolver_auto_tolerance_var", None), "0.010")
    state.resolver_auto_max_step_var = sv(getattr(state, "resolver_auto_max_step_var", None), "0.020")

    # массивы строк для CAN (12 полей: id, data0..7, len, flags, ts)
    if not getattr(state, "can_rx_data", None) or len(state.can_rx_data) != 12:
        state.can_rx_data = [sv(master=root) for _ in range(12)]
    if not getattr(state, "can_tx_data", None) or len(state.can_tx_data) != 12:
        state.can_tx_data = [sv(master=root) for _ in range(12)]

    state.log_enabled = bv(state.log_enabled, True)
    state.log_rows = getattr(state, "log_rows", []) or []
    state.max_rows = getattr(state, "max_rows", 5000) or 5000
    state.dynamic_log_columns = getattr(state, "dynamic_log_columns", []) or []

    # 3) Toolbar
    toolbar = ttk.Frame(root, style="Toolbar.TFrame")
    toolbar.pack(fill="x")

    ttk.Button(toolbar, text="Send", style="Accent.TButton",
               command=handlers.get("send_all", lambda: None)).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="▶ Start CAN", width=14,
               command=lambda: handlers.get("send_cmd", lambda *_: None)("Init")).pack(side="left", padx=(PAD, 4), pady=PAD)
    ttk.Button(toolbar, text="ARM", width=10,
               command=handlers.get("arm_control", lambda: None)).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="■ STOP", width=12,
               command=handlers.get("safe_stop", lambda: None)).pack(side="left", padx=4, pady=PAD)
    ttk.Label(toolbar, textvariable=state.control_arm_status_var, foreground="#a33").pack(side="left", padx=8, pady=PAD)
    ttk.Button(toolbar, text="Resolver RX", width=14,
               command=handlers.get("start_resolver_calibration", lambda: None)).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="↺ Reset", width=14,
               command=lambda: handlers.get("send_cmd", lambda *_: None)("Read2")).pack(side="left", padx=4, pady=PAD)
    ttk.Button(toolbar, text="💾 Save", width=14,
               command=lambda: handlers.get("send_cmd", lambda *_: None)("SaveCfg")).pack(side="left", padx=4, pady=PAD)

    ttk.Label(toolbar, text="JSON ms:").pack(side="left", padx=(12, 4), pady=PAD)
    json_period_entry = ttk.Entry(toolbar, textvariable=state.json_period_ms_var, width=8, justify="right")
    json_period_entry.pack(side="left", padx=(0, 4), pady=PAD)
    ttk.Button(toolbar, text="Apply", width=8,
               command=handlers.get("apply_json_period", lambda: None)).pack(side="left", padx=(0, 4), pady=PAD)
    json_period_entry.bind(
        "<Return>",
        lambda e: (handlers.get("apply_json_period", lambda: None)(), "break")[1],
    )

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
    maps_frame   = ttk.Frame(notebook); notebook.add(maps_frame, text="Maps")
    signals_frame = ttk.Frame(notebook); notebook.add(signals_frame, text="Signals")
    resolver_frame = ttk.Frame(notebook); notebook.add(resolver_frame, text="Resolver RX")
    # Legacy lookup-table AutoCal has no controller/server handlers. Keep its
    # frame internal instead of exposing dead buttons in the production UI.
    auto_frame = ttk.Frame(notebook)

    # === Complete server telemetry ===
    indication_canvas = tk.Canvas(ind_frame, highlightthickness=0)
    indication_scroll = ttk.Scrollbar(ind_frame, orient="vertical", command=indication_canvas.yview)
    indication_inner = ttk.Frame(indication_canvas)
    indication_inner.bind(
        "<Configure>",
        lambda _e: indication_canvas.configure(scrollregion=indication_canvas.bbox("all")),
    )
    indication_window = indication_canvas.create_window((0, 0), window=indication_inner, anchor="nw")
    indication_canvas.bind(
        "<Configure>",
        lambda e: indication_canvas.itemconfigure(indication_window, width=e.width),
    )
    indication_canvas.configure(yscrollcommand=indication_scroll.set)
    indication_canvas.pack(side="left", fill="both", expand=True)
    indication_scroll.pack(side="right", fill="y")

    for group_index, (group_title, group_keys) in enumerate(INDICATION_GROUPS):
        group = ttk.LabelFrame(indication_inner, text=group_title)
        group.grid(
            row=group_index // 2,
            column=group_index % 2,
            sticky="nsew",
            padx=10,
            pady=8,
        )
        _build_telemetry_fields(group, state, group_keys, columns=2)
    indication_inner.grid_columnconfigure(0, weight=1)
    indication_inner.grid_columnconfigure(1, weight=1)

    # === Resolver calibration: receive-only, no application CAN frames ===
    resolver_inner = ttk.Frame(resolver_frame)
    resolver_inner.pack(fill="both", expand=True, padx=18, pady=18)

    resolver_status = ttk.LabelFrame(resolver_inner, text="Safe calibration mode")
    resolver_status.pack(fill="x", pady=(0, 12))
    ttk.Label(resolver_status, textvariable=state.resolver_mode_var,
              font=("Segoe UI", 13, "bold")).pack(anchor="w", padx=12, pady=(10, 4))
    ttk.Label(
        resolver_status,
        text=("В этом режиме сервер принимает CAN, но блокирует все прикладные кадры TX. "
              "Перед запуском аппаратно отключите PWM/силовую часть или перезапустите инвертор: "
              "режим RX не может отменить ранее сохранённую команду."),
        wraplength=900,
        justify="left",
    ).pack(anchor="w", padx=12, pady=(0, 10))

    resolver_buttons = ttk.Frame(resolver_inner)
    resolver_buttons.pack(fill="x", pady=(0, 12))
    ttk.Button(resolver_buttons, text="▶ Start RX-only (50 Hz)",
               command=handlers.get("start_resolver_calibration", lambda: None)).pack(side="left", padx=(0, 8))
    ttk.Button(resolver_buttons, text="■ Stop CAN",
               command=lambda: handlers.get("send_cmd", lambda *_: None)("Stop")).pack(side="left")
    ttk.Button(resolver_buttons, text="Reset capture",
               command=handlers.get("reset_resolver_capture", lambda: None)).pack(side="left", padx=8)

    resolver_values = ttk.LabelFrame(resolver_inner, text="Live resolver values")
    resolver_values.pack(fill="x")
    resolver_fields = [
        ("Sine (zero-centered ADC counts)", state.resolver_sine_var),
        ("Cosine (zero-centered ADC counts)", state.resolver_cosine_var),
        ("Amplitude sqrt(sin²+cos²)", state.resolver_amplitude_var),
        ("Theta raw [rad]", state.resolver_theta_var),
        ("Theta corrected [rad]", state.resolver_theta_corr_var),
    ]
    for row, (label, var) in enumerate(resolver_fields):
        ttk.Label(resolver_values, text=label + ":").grid(row=row, column=0, sticky="e", padx=10, pady=7)
        ttk.Entry(resolver_values, textvariable=var, width=22, state="readonly").grid(
            row=row, column=1, sticky="w", padx=10, pady=7
        )
    ttk.Label(
        resolver_values,
        text=("Проворачивайте вал вручную. Sin и cos должны плавно меняться, иметь близкие амплитуды "
              "и сдвиг около 90°. Theta должна пройти полный оборот без скачков; для медленной ручной "
              "калибровки 50 Гц достаточно."),
        wraplength=900,
        justify="left",
    ).grid(row=len(resolver_fields), column=0, columnspan=2, sticky="w", padx=10, pady=(8, 12))

    resolver_auto = ttk.LabelFrame(resolver_inner, text="Automatic thetaCorr calibration")
    resolver_auto.pack(fill="x", pady=(12, 0))
    ttk.Label(
        resolver_auto,
        text=(
            "Сначала включите инвертор с Id=Iq=0 в обычном режиме, затем Start RX-only. "
            "В автокалибровке обычные команды 0x046/0x047/0x300 заблокированы; "
            "разрешён только защищённый кадр thetaCorr 0x301."
        ),
        foreground="#8a5a00",
        wraplength=940,
        justify="left",
    ).grid(row=0, column=0, columnspan=8, sticky="w", padx=10, pady=(8, 6))

    auto_live_fields = [
        ("fluxError [rad]", state.resolver_flux_error_var),
        ("thetaCorr parameter [rad]", state.resolver_theta_correction_var),
        ("electrical speed [rad/s]", state.resolver_electrical_speed_var),
        ("state", state.resolver_auto_state_var),
        ("flux valid", state.resolver_flux_valid_var),
        ("MCU command active", state.resolver_command_active_var),
        ("ACK sequence", state.resolver_ack_sequence_var),
        ("commanded thetaCorr [rad]", state.resolver_command_var),
        ("controller error [rad]", state.resolver_loop_error_var),
        ("status frame count", state.resolver_status_count_var),
        ("command sequence", state.resolver_command_sequence_var),
        ("command enabled", state.resolver_enable_command_var),
        ("stand loop active", state.resolver_active_var),
        ("stand loop converged", state.resolver_converged_var),
    ]
    for index, (label, var) in enumerate(auto_live_fields):
        row = 1 + index // 3
        col = index % 3
        ttk.Label(resolver_auto, text=label + ":").grid(row=row, column=col * 2, sticky="e", padx=(10, 4), pady=6)
        ttk.Entry(resolver_auto, textvariable=var, width=19, state="readonly").grid(
            row=row, column=col * 2 + 1, sticky="w", padx=(0, 8), pady=6
        )

    auto_settings = [
        ("gain", state.resolver_auto_gain_var),
        ("tolerance [rad]", state.resolver_auto_tolerance_var),
        ("max step [rad]", state.resolver_auto_max_step_var),
    ]
    for col, (label, var) in enumerate(auto_settings):
        ttk.Label(resolver_auto, text=label + ":").grid(row=6, column=col * 2, sticky="e", padx=(10, 4), pady=(6, 10))
        ttk.Entry(resolver_auto, textvariable=var, width=12).grid(
            row=6, column=col * 2 + 1, sticky="w", padx=(0, 8), pady=(6, 10)
        )
    ttk.Button(
        resolver_auto,
        text="▶ Start auto thetaCorr",
        command=handlers.get("start_resolver_auto_calibration", lambda: None),
    ).grid(row=7, column=0, columnspan=2, padx=10, pady=(0, 10), sticky="ew")
    ttk.Button(
        resolver_auto,
        text="■ Stop auto",
        command=handlers.get("stop_resolver_auto_calibration", lambda: None),
    ).grid(row=7, column=2, columnspan=2, padx=(0, 10), pady=(0, 10), sticky="ew")

    resolver_stats = ttk.LabelFrame(resolver_inner, text="Full-turn capture")
    resolver_stats.pack(fill="x", pady=(12, 0))
    resolver_stat_fields = [
        ("Samples", state.resolver_capture_count_var),
        ("Sine offset [ADC counts]", state.resolver_sine_offset_var),
        ("Cosine offset [ADC counts]", state.resolver_cosine_offset_var),
        ("Sine amplitude [ADC counts]", state.resolver_sine_amplitude_var),
        ("Cosine amplitude [ADC counts]", state.resolver_cosine_amplitude_var),
        ("Gain ratio sine/cosine", state.resolver_gain_ratio_var),
        ("Unwrapped electrical angle [rad]", state.resolver_unwrapped_theta_var),
        ("Cycles between markers", state.resolver_cycle_count_var),
        ("Marker status", state.resolver_marker_status_var),
        ("Ellipse center", state.resolver_ellipse_center_var),
        ("Ellipse axes", state.resolver_ellipse_axes_var),
        ("Ellipse rotation", state.resolver_ellipse_rotation_var),
        ("Channel non-orthogonality", state.resolver_nonorthogonality_var),
        ("Ellipse fit", state.resolver_fit_status_var),
    ]
    for row, (label, var) in enumerate(resolver_stat_fields):
        col = 0 if row % 2 == 0 else 2
        grid_row = row // 2
        ttk.Label(resolver_stats, text=label + ":").grid(row=grid_row, column=col, sticky="e", padx=10, pady=7)
        ttk.Entry(resolver_stats, textvariable=var, width=18, state="readonly").grid(
            row=grid_row, column=col + 1, sticky="w", padx=10, pady=7
        )

    resolver_actions = ttk.Frame(resolver_inner)
    resolver_actions.pack(fill="x", pady=(8, 0))
    ttk.Button(resolver_actions, text="Mark mechanical revolution start",
               command=handlers.get("resolver_mark_start", lambda: None)).pack(side="left", padx=4)
    ttk.Button(resolver_actions, text="Mark mechanical revolution end",
               command=handlers.get("resolver_mark_end", lambda: None)).pack(side="left", padx=4)
    ttk.Button(resolver_actions, text="Fit ellipse",
               command=handlers.get("resolver_fit_ellipse", lambda: None)).pack(side="left", padx=4)
    ttk.Button(resolver_actions, text="Export raw points CSV",
               command=handlers.get("export_resolver_csv", lambda: None)).pack(side="right", padx=4)
    resolver_plot_frame = ttk.LabelFrame(resolver_inner, text="SIN against COS (raw CAN points)")
    resolver_plot_frame.pack(fill="both", expand=True, pady=(10, 0))

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
            try:
                main_slider.state(["!disabled"])
                main_entry.state(["!disabled"])
            except Exception:
                main_entry.configure(state="normal")
            slider_title.configure(text="Speed\nrpm")
            main_slider.configure(from_=1000, to=-1000, variable=state.speed_var)
            _make_focusable_scale(main_slider, state.speed_var, step=1.0)

            # настроим spinbox
            try:
                main_entry.configure(textvariable=state.speed_var, from_=-1000, to=1000, increment=1.0)
            except Exception:
                main_entry.config(textvariable=state.speed_var, from_=-1000, to=1000, increment=1.0)
            _bind_spin_steps(main_entry, state.speed_var, step=1.0)

            def _on_release(_=None):
                _ui_log(state, "[UI] ns изменён локально → нажмите «Отправить»")
            main_slider.unbind("<ButtonRelease-1>")
            main_slider.bind("<ButtonRelease-1>", _on_release)

        elif mode == "torque":
            try:
                main_slider.state(["!disabled"])
                main_entry.state(["!disabled"])
            except Exception:
                main_entry.configure(state="normal")
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
        else:  # currents are edited in the Id/Iq fields
            slider_title.configure(text="Currents\nuse Id / Iq")
            main_slider.unbind("<ButtonRelease-1>")
            try:
                main_slider.state(["disabled"])
                main_entry.state(["disabled"])
            except Exception:
                main_entry.configure(state="disabled")

    # Радиокнопки режимов (сообщаем контроллеру и сразу переконфигурируем слайдер/спинбокс)
    def _on_mode_pick(val):
        handlers.get("set_mode", lambda *_: None)(val)
        _configure_main_slider(val)

    ttk.Radiobutton(mode_frame, text="Torque (Ms)", value="torque",
                    variable=state.mode_var, command=lambda: _on_mode_pick("torque")).grid(row=0, column=0, padx=8, pady=8, sticky="w")
    ttk.Radiobutton(mode_frame, text="Currents (Id/Iq)", value="currents",
                    variable=state.mode_var, command=lambda: _on_mode_pick("currents")).grid(row=0, column=1, padx=8, pady=8, sticky="w")
    ttk.Radiobutton(mode_frame, text="Speed (ns)", value="speed",
                    variable=state.mode_var, command=lambda: _on_mode_pick("speed")).grid(row=0, column=2, padx=8, pady=8, sticky="w")
    # Currents
    currents_frame = ttk.LabelFrame(main_inner, text="Currents")
    currents_frame.grid(row=1, column=1, padx=(0,10), pady=10, sticky="nsew")
    ttk.Label(currents_frame, text="Id [A]").grid(row=0, column=0, sticky="e", padx=6, pady=6)
    _make_num_spin(currents_frame, state.Id_var, from_=-10.0, to=10.0, step=0.1, width=10)\
        .grid(row=0, column=1, sticky="w")
    ttk.Label(currents_frame, text="Iq [A]").grid(row=0, column=2, sticky="e", padx=6, pady=6)
    _make_num_spin(currents_frame, state.Iq_var, from_=-10.0, to=10.0, step=0.1, width=10)\
        .grid(row=0, column=3, sticky="w")

    # Limits
    limits_frame = ttk.LabelFrame(main_inner, text="Limits")
    limits_frame.grid(row=1, column=2, padx=(0,10), pady=10, sticky="nsew")
    _labent(limits_frame, 0, 0, "M_min [Н·м]", state.M_min_var)
    _labent(limits_frame, 0, 2, "M_max [Н·м]", state.M_max_var)
    _labent(limits_frame, 1, 0, "M_grad_max",  state.M_grad_max_var)
    _labent(limits_frame, 1, 2, "n_max [rpm] ≤ 1000", state.n_max_var)
    ttk.Label(limits_frame, text="ARM required for non-zero commands; commissioning limit ±10 A").grid(
        row=2, column=0, columnspan=4, sticky="w", padx=6, pady=4
    )

    torque_meter_frame = ttk.LabelFrame(main_inner, text="Torque meter (operator input)")
    torque_meter_frame.grid(row=2, column=1, padx=(0, 10), pady=10, sticky="nsew")
    ttk.Label(torque_meter_frame, text="Torque_meter [N·m]").grid(row=0, column=0, sticky="e", padx=6, pady=6)
    ttk.Entry(torque_meter_frame, textvariable=state.torque_meter_var, width=12).grid(row=0, column=1, sticky="w", padx=6, pady=6)
    ttk.Label(torque_meter_frame, text="Заполняется оператором; сохраняется в журнале и CSV.").grid(
        row=1, column=0, columnspan=2, sticky="w", padx=6, pady=4
    )

    # Параметры стенда (поля-отображение)
    params_frame = ttk.LabelFrame(main_inner, text="MCU_VCU_parameters")
    params_frame.grid(row=3, column=1, columnspan=2, padx=(0,10), pady=0, sticky="nsew")
    state.entry_vars = getattr(state, "entry_vars", {}) or {}
    _build_telemetry_fields(params_frame, state, CONTROL_MONITOR_FIELDS, columns=2)

    # CAN Tx/Rx (12 полей: id, data0..7, len, flags, ts)
    can_frame = ttk.LabelFrame(main_inner, text="Tx / Rx CAN")

    # MCU Current & Voltage
    voltage_frame = ttk.LabelFrame(main_inner, text="MCU Current & Voltage")
    voltage_frame.grid(row=4, column=1, padx=(0,10), pady=10, sticky="nsew")
    _build_telemetry_fields(voltage_frame, state, CURRENT_VOLTAGE_FIELDS)

    # MCU Flux Parameters
    flux_frame = ttk.LabelFrame(main_inner, text="MCU Flux Parameters")
    flux_frame.grid(row=4, column=2, padx=(0,10), pady=10, sticky="nsew")
    _build_telemetry_fields(flux_frame, state, FLUX_FIELDS)

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
    _labent(auto_limits, 1, 2, "n_max [rpm] ≤ 1000", state.n_max_var)

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
    xs = ttk.Scrollbar(logbook_top, orient="horizontal", command=telem_tree.xview)
    telem_tree.configure(yscroll=ys.set, xscroll=xs.set)
    ys.pack(side="right", fill="y")
    xs.pack(side="bottom", fill="x")
    telem_tree.pack(side="left", fill="both", expand=True)
    state.telem_tree = telem_tree

    log_events = ttk.LabelFrame(log_frame, text="Events")
    log_events.pack(fill="both", expand=True, padx=10, pady=(0,10))
    log_box = Text(log_events, height=8, wrap="word")
    log_box.pack(fill="both", padx=6, pady=6, expand=True)
    state.log_box = log_box

    # === Signals ===
    signals_inner = ttk.Frame(signals_frame)
    signals_inner.pack(fill="both", expand=True, padx=10, pady=10)

    signals_toolbar = ttk.Frame(signals_inner)
    signals_toolbar.pack(fill="x", pady=(0, 8))
    ttk.Button(
        signals_toolbar,
        text="Apply selection",
        command=handlers.get("apply_signal_selection", lambda: None),
    ).pack(side="right")

    panes = ttk.Panedwindow(signals_inner, orient="horizontal")
    panes.pack(fill="both", expand=True)

    rx_wrap = ttk.LabelFrame(panes, text="RX")
    tx_wrap = ttk.LabelFrame(panes, text="TX")
    panes.add(rx_wrap, weight=1)
    panes.add(tx_wrap, weight=1)

    signal_cols = ("enabled", "id", "message", "count", "signals")
    rx_signal_tree = ttk.Treeview(rx_wrap, columns=signal_cols, show="headings", height=22)
    tx_signal_tree = ttk.Treeview(tx_wrap, columns=signal_cols, show="headings", height=22)
    for tree in (rx_signal_tree, tx_signal_tree):
        tree.heading("enabled", text="On")
        tree.heading("id", text="ID")
        tree.heading("message", text="Message")
        tree.heading("count", text="Signals")
        tree.heading("signals", text="Names")
        tree.column("enabled", width=48, anchor="center")
        tree.column("id", width=72, anchor="center")
        tree.column("message", width=150, anchor="w")
        tree.column("count", width=70, anchor="center")
        tree.column("signals", width=330, anchor="w")
        tree.pack(side="left", fill="both", expand=True)
        ys_sig = ttk.Scrollbar(tree.master, orient="vertical", command=tree.yview)
        tree.configure(yscroll=ys_sig.set)
        ys_sig.pack(side="right", fill="y")

    def _refresh_signal_trees():
        for tree in (rx_signal_tree, tx_signal_tree):
            for iid in tree.get_children():
                tree.delete(iid)
        groups = {}
        for item in getattr(state, "signal_catalog", []):
            direction = item.get("direction")
            message_id = item.get("message_id")
            key = (direction, message_id)
            groups.setdefault(key, {
                "direction": direction,
                "message_id": message_id,
                "message_name": item.get("message_name", ""),
                "signals": [],
            })
            groups[key]["signals"].append(str(item.get("signal_name", "")))

        for group in sorted(groups.values(), key=lambda g: (g["direction"] or "", int(g["message_id"] or 0))):
            direction = group["direction"]
            tree = tx_signal_tree if direction == "tx" else rx_signal_tree
            signal_names = group["signals"]
            selected = (
                all(name in state.selected_tx_signals for name in signal_names)
                if direction == "tx"
                else all(name in state.selected_rx_signals for name in signal_names)
            )
            tree.insert(
                "",
                "end",
                iid=f"{direction}:{group['message_id']}",
                values=(
                    "yes" if selected else "no",
                    f"0x{int(group['message_id'] or 0):03X}",
                    group["message_name"],
                    str(len(signal_names)),
                    ", ".join(signal_names),
                ),
            )

    def _toggle_signal_from_tree(tree, direction: str, event):
        iid = tree.identify_row(event.y)
        if not iid:
            return
        message_id = iid.split(":", 1)[1]
        handlers.get("toggle_signal_selection", lambda *_: None)(direction, message_id)

    rx_signal_tree.bind("<Double-1>", lambda e: _toggle_signal_from_tree(rx_signal_tree, "rx", e))
    tx_signal_tree.bind("<Double-1>", lambda e: _toggle_signal_from_tree(tx_signal_tree, "tx", e))

    root.bind_all("<Control-l>", lambda e: state.log_enabled.set(not state.log_enabled.get()))
    root.bind_all("<Control-e>", lambda e: handlers.get("export_csv", lambda: _export_csv_default(state))())
    root.bind_all("<Control-Shift-C>", lambda e: handlers.get("clear_log", lambda: _clear_log_default(state))())

    # === Trends / Maps ===
    # Matplotlib грузится только при первом открытии соответствующей вкладки.
    trends_container = ttk.Frame(trends_frame)
    trends_container.pack(fill="both", expand=True, padx=10, pady=10)
    maps_container = ttk.Frame(maps_frame)
    maps_container.pack(fill="both", expand=True, padx=10, pady=10)
    fig_trends = canvas_trends = None
    ax1 = ax2 = ax3 = ax4 = ax5 = ax6t = None
    l_ns = l_ms = l_idc = l_isd = l_isq = l_id = l_iq = l_ud = l_uq = None
    l_theta = l_theta_corr = l_flux_error = None

    fig_maps = canvas_maps = None
    ax5a = ax5b = ax6 = ax6_right = None
    sc_ld = sc_lq = ln_torque = ln_pmech = ln_pelec = None
    fig_resolver = canvas_resolver = None
    ax_resolver = line_resolver = None

    def _ensure_trends():
        nonlocal fig_trends, canvas_trends
        nonlocal ax1, ax2, ax3, ax4, ax5, ax6t
        nonlocal l_ns, l_ms, l_idc, l_isd, l_isq, l_id, l_iq, l_ud, l_uq
        nonlocal l_theta, l_theta_corr, l_flux_error
        if fig_trends is not None:
            return
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
        from matplotlib.figure import Figure

        fig_trends = Figure(figsize=(11, 7), dpi=100, constrained_layout=True)
        ax1 = fig_trends.add_subplot(321)
        ax2 = fig_trends.add_subplot(322)
        ax3 = fig_trends.add_subplot(323)
        ax4 = fig_trends.add_subplot(324)
        ax5 = fig_trends.add_subplot(325)
        ax6t = fig_trends.add_subplot(326)
        for ax, title, ylabel in (
            (ax1, "Motor speed", "rpm"),
            (ax2, "Actual torque", "N·m"),
            (ax3, "Measured MCU currents", "A"),
            (ax4, "FOC currents", "A"),
            (ax5, "FOC voltages", "V"),
            (ax6t, "Electrical angles / flux error", "rad"),
        ):
            ax.set_title(title)
            ax.set_xlabel("seconds from now")
            ax.set_ylabel(ylabel)
            ax.grid(True)

        l_ns, = ax1.plot([], [], label="speed")
        l_ms, = ax2.plot([], [], label="torque")
        l_idc, = ax3.plot([], [], label="MCU_IsCurr")
        l_isd, = ax3.plot([], [], label="MCU_Isd")
        l_isq, = ax3.plot([], [], label="MCU_Isq")
        l_id, = ax4.plot([], [], label="Id")
        l_iq, = ax4.plot([], [], label="Iq")
        l_ud, = ax5.plot([], [], label="Ud")
        l_uq, = ax5.plot([], [], label="Uq")
        l_theta, = ax6t.plot([], [], label="Theta")
        l_theta_corr, = ax6t.plot([], [], label="ThetaCorr")
        l_flux_error, = ax6t.plot([], [], label="fluxError")
        for ax in (ax3, ax4, ax5, ax6t):
            ax.legend(loc="best")

        canvas_trends = FigureCanvasTkAgg(fig_trends, master=trends_container)
        canvas_trends.get_tk_widget().pack(fill="both", expand=True)
        state.trends = {
            "figure": fig_trends,
            "canvas": canvas_trends,
            "axes": [ax1, ax2, ax3, ax4, ax5, ax6t],
            "series": [
                (l_ns, state.trend_ns),
                (l_ms, state.trend_Ms),
                (l_idc, state.trend_Idc),
                (l_isd, state.trend_Isd),
                (l_isq, state.trend_Isq),
                (l_id, state.trend_Id),
                (l_iq, state.trend_Iq),
                (l_ud, state.trend_Ud),
                (l_uq, state.trend_Uq),
                (l_theta, state.trend_theta),
                (l_theta_corr, state.trend_theta_corr),
                (l_flux_error, state.trend_flux_error),
            ],
            "ax1": ax1, "l_theta": l_theta,
        }

    def _ensure_maps():
        nonlocal fig_maps, canvas_maps
        nonlocal ax5a, ax5b, ax6, ax6_right
        nonlocal sc_ld, sc_lq, ln_torque, ln_pmech, ln_pelec
        if fig_maps is not None:
            return
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
        from matplotlib.figure import Figure

        fig_maps = Figure(figsize=(8, 5), dpi=100)
        ax5a = fig_maps.add_subplot(221)
        ax5b = fig_maps.add_subplot(222)
        ax6 = fig_maps.add_subplot(212)
        ax5a.set_title("Ld vs Id"); ax5a.set_xlabel("Id, A"); ax5a.set_ylabel("Ld, H"); ax5a.grid(True)
        ax5b.set_title("Lq vs Iq"); ax5b.set_xlabel("Iq, A"); ax5b.set_ylabel("Lq, H"); ax5b.grid(True)
        ax6.set_title("Torque & Power vs RPM"); ax6.set_xlabel("RPM"); ax6.grid(True)
        ax6_right = ax6.twinx(); ax6.set_ylabel("Torque, N·m"); ax6_right.set_ylabel("Power, kW")

        sc_ld = ax5a.plot([], [], linestyle="", marker=".", markersize=3)[0]
        sc_lq = ax5b.plot([], [], linestyle="", marker=".", markersize=3)[0]
        ln_torque, = ax6.plot([], [], label="Torque (N·m)")
        ln_pmech, = ax6_right.plot([], [], label="P_mech (kW)")
        ln_pelec, = ax6_right.plot([], [], label="P_elec (kW)", linestyle="--")
        ax6.legend([ln_torque, ln_pmech, ln_pelec], ["Torque (N·m)", "P_mech (kW)", "P_elec (kW)"])

        canvas_maps = FigureCanvasTkAgg(fig_maps, master=maps_container)
        canvas_maps.get_tk_widget().pack(fill="both", expand=True)
        state.maps = {
            "figure": fig_maps, "canvas": canvas_maps,
            "ax5a": ax5a, "sc_ld": sc_ld,
            "ax5b": ax5b, "sc_lq": sc_lq,
            "ax6": ax6, "ax6_right": ax6_right,
            "ln_torque": ln_torque, "ln_pmech": ln_pmech, "ln_pelec": ln_pelec,
        }

    def _ensure_resolver_plot():
        nonlocal fig_resolver, canvas_resolver, ax_resolver, line_resolver
        if fig_resolver is not None:
            return
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
        from matplotlib.figure import Figure
        fig_resolver = Figure(figsize=(7, 4), dpi=100, constrained_layout=True)
        ax_resolver = fig_resolver.add_subplot(111)
        ax_resolver.set_xlabel("SIN [ADC counts]")
        ax_resolver.set_ylabel("COS [ADC counts]")
        ax_resolver.set_title("Resolver trajectory / ellipse")
        ax_resolver.grid(True)
        line_resolver, = ax_resolver.plot([], [], ".", markersize=2)
        canvas_resolver = FigureCanvasTkAgg(fig_resolver, master=resolver_plot_frame)
        canvas_resolver.get_tk_widget().pack(fill="both", expand=True)
        state.resolver_plot = {"figure": fig_resolver, "canvas": canvas_resolver, "axes": ax_resolver, "line": line_resolver}

    def _on_tab_changed(_event=None):
        selected = notebook.select()
        if selected == str(trends_frame):
            _ensure_trends()
        elif selected == str(maps_frame):
            _ensure_maps()
        elif selected == str(resolver_frame):
            _ensure_resolver_plot()

    notebook.bind("<<NotebookTabChanged>>", _on_tab_changed, add="+")

    # стартовая конфигурация единого ползунка + спинбокса
    _configure_main_slider(state.mode_var.get())

    # Выдаём ссылки на графики/оси в state — чтобы контроллер мог обновлять
    state.trends = {}
    state.maps = {}
    state.resolver_plot = {}

    view = ViewRefs(
        root=root, style=style,
        toolbar=toolbar, conn_pill_wrap=pill_wrap,
        notebook=notebook, main_frame=main_frame, ind_frame=ind_frame, log_frame=log_frame,
        trends_frame=trends_frame, maps_frame=maps_frame, signals_frame=signals_frame,
        resolver_frame=resolver_frame,
        controls_container=controls_container, mode_frame=mode_frame, currents_frame=currents_frame,
        limits_frame=limits_frame, params_frame=params_frame, can_frame=can_frame,
        voltage_frame=voltage_frame, flux_frame=flux_frame,
        slider_frame=slider_frame, main_slider=main_slider, main_entry=main_entry,
        telem_tree=telem_tree, log_box=log_box,
        rx_signal_tree=rx_signal_tree, tx_signal_tree=tx_signal_tree,
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
    view.refresh_signal_trees = _refresh_signal_trees
    view.ensure_trends = _ensure_trends
    view.ensure_maps = _ensure_maps

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
    columns = list(TELEM_COLUMNS) + list(getattr(state, "dynamic_log_columns", []))
    with open(fname, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(columns)
        for row in state.log_rows:
            w.writerow([row.get(k, "") for k in columns])
    _ui_log(state, f"💾 exported: {fname}")


# удобный Getter для Entry (простой алиас, чтобы не импортировать напрямую вверху)
def Entry(parent, **kwargs):
    return ttk.Entry(parent, **kwargs)
