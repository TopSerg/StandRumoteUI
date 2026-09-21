from __future__ import annotations

import json
import math
from datetime import datetime

from state import (
    AppState,
    FIELD_ALIASES,
    TELEM_COLUMNS,
    DEFAULT_RS_OHMS,
    DEFAULT_POLE_PAIRS,
)


class Telemetry:
    """
    Разбор входящих сообщений, обновление state и UI.
    Ничего не знает о сети: WSClient просто вызывает on_message/on_status/on_error.
    """

    def __init__(self, root, state: AppState, views, ui_log=print):
        self.root = root
        self.state = state
        self.views = views
        self.ui_log = ui_log

        # краткие ссылки на графики (делаем устойчиво к разным реализациям view)
        self._last_rs = DEFAULT_RS_OHMS
        self._last_pole_pairs = DEFAULT_POLE_PAIRS

    # ----------------- публичный API для WSClient -----------------

    def start_timers(self):
        # периодический рефреш графиков (минимальный, чтобы не грузить CPU)
        self.root.after(500, self._tick_trends)

    def on_status(self, msg: str):
        # Вызывается WSClient при изменении статуса
        def _ui():
            s = msg.lower().strip()
            if "connected" in s or "open" in s:
                self.state.conn_var.set("connected")
                self.state.conn_color.set("#1bb55c")  # зелёный
            elif "closing" in s or "closed" in s or "disconnected" in s:
                self.state.conn_var.set("disabled")
                self.state.conn_color.set("#d72c20")  # красный
            else:
                self.state.conn_var.set(s)
            self.ui_log(f"[WS] {msg}")

        self.root.after(0, _ui)

    def on_error(self, msg: str):
        self.root.after(0, lambda: self.ui_log(f"[ERR] {msg}"))

    def on_message(self, raw: str):
        # Поток WS: парсим тут, а обновляем GUI в main-потоке
        try:
            data = json.loads(raw)
        except Exception:
            self.root.after(0, lambda: self.ui_log("❌ JSON parse error"))
            return
        self.root.after(0, lambda: self._dispatch(data))

    # ----------------- внутренности -----------------

    def _dispatch(self, data: dict):
        """
        Универсальный роутинг:
        - CAN кадры (type=='can_frame' или direction+id) → _handle_can_frame
        - Телеметрия модели (ns/Ms/Idc/Isd/Ud/Uq/Id/Iq/...) → _handle_model_data
        - Остальное — просто логируем
        """
        try:
            # как в старом gui_ws: type=='can_frame'
            if data.get("type") == "can_frame":
                self._handle_can_frame(data)
                return

            if data.get("type") == "signal_catalog":
                self._handle_signal_catalog(data)
                return

            # fallback: по наличию id+direction
            if "id" in data and "direction" in data:
                self._handle_can_frame(data)
                return

            # всё остальное считаем телеметрией (по ключам)
            keys = set(data.keys())
            if keys & {
                "ns",
                "Ms",
                "Idc",
                "Isd",
                "Ud",
                "Uq",
                "Id",
                "Iq",
                "Flux",
                "Theta",
                "Temperature",
                "ZVFlux",
                "ZVTheta",
                "ZVTemperature",
                "ZVRs",
                "ZVTimeStamp",
                "ZVThetaCorr",
                "Emf",
                "Welectrical",
                "Wmechanical",
                "Rs",
                "TimeStamp",
                "ThetaCorr",
                "ResolverSine",
                "ResolverCosine",
                "ResolverAmplitude",
                "ResolverTheta",
                "ResolverThetaCorr",
                "FluxPositionError",
                "ResolverThetaCorrection",
                "ResolverElectricalSpeed",
                "ResolverCalibrationState",
                "can_mode",
                "dbc_signals",
            }:
                self._handle_model_data(data)
                return

            # fallback — просто сообщим в лог
            self.ui_log(f"[RX] {data}")

        except Exception as ex:
            self.ui_log(f"[TELEM] handler error: {ex}")

    # ---------- CAN ----------

    def _handle_signal_catalog(self, data: dict):
        signals = data.get("signals", [])
        if not isinstance(signals, list):
            return
        self.state.signal_catalog = signals
        self.state.selected_rx_signals = {
            str(item.get("signal_name"))
            for item in signals
            if item.get("direction") == "rx" and item.get("selected")
        }
        self.state.selected_tx_signals = {
            str(item.get("signal_name"))
            for item in signals
            if item.get("direction") == "tx" and item.get("selected")
        }
        dynamic = []
        for item in signals:
            if not item.get("selected"):
                continue
            col = self._dbc_log_column(item)
            if col and col not in dynamic:
                dynamic.append(col)
        self.state.dynamic_log_columns = dynamic
        self._sync_log_tree_columns()
        refresh = getattr(self.views, "refresh_signal_trees", None)
        if callable(refresh):
            refresh()
        self.ui_log(
            f"[SIGNALS] catalog updated: RX={len(self.state.selected_rx_signals)} "
            f"TX={len(self.state.selected_tx_signals)}"
        )

    def _log_dbc_signals(self, d: dict):
        signals = d.get("dbc_signals")
        if not isinstance(signals, list):
            return
        self.state.latest_dbc_signals = signals

    @staticmethod
    def _dbc_log_column(sample: dict) -> str:
        direction = str(sample.get("direction", "")).upper()
        name = str(sample.get("signal_name", "")).strip()
        if not name:
            return ""
        return f"{direction}.{name}" if direction else name

    def _sync_log_tree_columns(self):
        tree = getattr(self.views, "telem_tree", getattr(self.state, "telem_tree", None))
        if tree is None:
            return
        fixed = set(TELEM_COLUMNS)
        columns = list(TELEM_COLUMNS) + list(getattr(self.state, "dynamic_log_columns", []))
        try:
            tree.configure(columns=columns)
            for col in columns:
                tree.heading(col, text=col)
                width = 100 if col in fixed else max(120, min(220, len(col) * 8))
                tree.column(col, width=width, anchor="center")
        except Exception:
            pass

    def _handle_can_frame(self, data: dict):
        """
        Ожидаем поля:
          direction: "rx" / "tx"
          id, len, flags, data0..data7, ts (может быть отсутствует)
        В GUI держим по 12 полей на строку (id, data0..7, len, flags, ts).
        """
        direction = str(data.get("direction", "")).lower()
        line = self.state.can_rx_data if direction == "rx" else self.state.can_tx_data

        def _fmt_byte(v):
            try:
                iv = int(v) & 0xFF
                return f"{iv:02X}"
            except Exception:
                return "--"

        def _set(idx: int, text: str):
            try:
                line[idx].set(text)
            except Exception:
                pass

        try:
            _set(0, f"{int(data.get('id', 0)):#04x}")
        except Exception:
            _set(0, str(data.get("id", "")))

        for i in range(8):
            _set(1 + i, _fmt_byte(data.get(f"data{i}", 0)))
        _set(9, str(data.get("len", "")))
        _set(10, str(data.get("flags", "")))
        _set(
            11,
            data.get("ts", datetime.now().strftime("%H:%M:%S.%f")[:-3]),
        )

        self.ui_log(
            f"[CAN {direction.upper()}] id={line[0].get()} data="
            + " ".join(line[1 + i].get() for i in range(8))
        )

    # ---------- модель / телеметрия ----------

    def _get_alias(self, d: dict, key: str, default=None):
        # alias lookup по FIELD_ALIASES
        for k in (key, *FIELD_ALIASES.get(key, [])):
            if k in d:
                return d.get(k)
        return default

    @staticmethod
    def _as_float(x, default=None):
        try:
            return float(x)
        except Exception:
            return default

    def _handle_model_data(self, d: dict):
        """
        Разбираем телеметрию, заполняем:
        - поля отображения (entry_vars)
        - логбук (таблица + буфер)
        - буферы для трендов/карт
        """
        self._log_dbc_signals(d)

        # --- считать основные величины (с алиасами) ---
        Ud = self._as_float(self._get_alias(d, "Ud"))
        Uq = self._as_float(self._get_alias(d, "Uq"))
        Id = self._as_float(self._get_alias(d, "Id"))
        Iq = self._as_float(self._get_alias(d, "Iq"))
        Idc = self._as_float(d.get("Idc"))
        Isd = self._as_float(d.get("Isd"))
        Isq = self._as_float(d.get("Isq"))
        Udc = self._as_float(d.get("Udc"))
        Ms = self._as_float(d.get("Ms"))
        ns = self._as_float(d.get("ns"))
        m_grad_max = self._as_float(d.get("M_grad_max"))
        n_max = self._as_float(d.get("n_max"))

        igbt_u = self._as_float(d.get("MCU_IGBTTempU"))
        igbt_v = self._as_float(d.get("MCU_IGBTTempV"))
        igbt_w = self._as_float(d.get("MCU_IGBTTempW"))
        igbt_max = self._as_float(d.get("MCU_IGBTTempMax"))
        stator = self._as_float(d.get("MCU_TempCurrStr"))
        coolant = self._as_float(d.get("MCU_TempCurrCool"))
        m_max = self._as_float(d.get("M_max"))
        m_min = self._as_float(d.get("M_min"))
        mcu_ofs_al = self._as_float(d.get("MCU_OfsAl"))
        mcu_isd = self._as_float(d.get("MCU_Isd"))
        mcu_isq = self._as_float(d.get("MCU_Isq"))
        mcu_b_dmp = self._as_float(d.get("MCU_bDmpCActv"))
        mcu_gate = self._as_float(d.get("MCU_stGateDrv"))
        mcu_dmp_trq = self._as_float(d.get("MCU_DmpCTrqCurr"))
        mcu_work_mode = self._as_float(d.get("MCU_VCUWorkMode"))

        Flux = self._as_float(self._get_alias(d, "Flux"))
        Theta = self._as_float(self._get_alias(d, "Theta"))
        Temperature = self._as_float(self._get_alias(d, "Temperature"))
        TimeStamp = self._as_float(self._get_alias(d, "TimeStamp"))
        ThetaCorr = self._as_float(self._get_alias(d, "ThetaCorr"))
        ResolverSine = self._as_float(self._get_alias(d, "ResolverSine"))
        ResolverCosine = self._as_float(self._get_alias(d, "ResolverCosine"))
        ResolverAmplitude = self._as_float(self._get_alias(d, "ResolverAmplitude"))
        ResolverTheta = self._as_float(self._get_alias(d, "ResolverTheta"))
        ResolverThetaCorr = self._as_float(self._get_alias(d, "ResolverThetaCorr"))
        FluxPositionError = self._as_float(self._get_alias(d, "FluxPositionError"))
        ResolverThetaCorrection = self._as_float(self._get_alias(d, "ResolverThetaCorrection"))
        ResolverElectricalSpeed = self._as_float(self._get_alias(d, "ResolverElectricalSpeed"))
        ResolverCalibrationState = str(d.get("ResolverCalibrationState", ""))
        can_mode = str(d.get("can_mode", ""))

        def set_resolver_var(name: str, value, digits: int = 3):
            var = getattr(self.state, name, None)
            if var is None:
                return
            if value is None:
                var.set("—")
            else:
                var.set(f"{value:.{digits}f}")

        set_resolver_var("resolver_sine_var", ResolverSine)
        set_resolver_var("resolver_cosine_var", ResolverCosine)
        set_resolver_var("resolver_amplitude_var", ResolverAmplitude)
        set_resolver_var("resolver_theta_var", ResolverTheta, 4)
        set_resolver_var("resolver_theta_corr_var", ResolverThetaCorr, 4)
        set_resolver_var("resolver_flux_error_var", FluxPositionError, 5)
        set_resolver_var("resolver_theta_correction_var", ResolverThetaCorrection, 5)
        set_resolver_var("resolver_electrical_speed_var", ResolverElectricalSpeed, 1)
        if ResolverCalibrationState:
            suffix = " — CONVERGED" if d.get("ResolverCalibrationConverged") else ""
            self.state.resolver_auto_state_var.set(ResolverCalibrationState + suffix)
        if can_mode:
            self.state.resolver_mode_var.set(
                "ACTIVE — RX ONLY — TX BLOCKED" if d.get("can_rx_only") else can_mode.upper()
            )
        if d.get("can_rx_only") and ResolverSine is not None and ResolverCosine is not None:
            self.state.resolver_sine_min = (
                ResolverSine if self.state.resolver_sine_min is None
                else min(self.state.resolver_sine_min, ResolverSine)
            )
            self.state.resolver_sine_max = (
                ResolverSine if self.state.resolver_sine_max is None
                else max(self.state.resolver_sine_max, ResolverSine)
            )
            self.state.resolver_cosine_min = (
                ResolverCosine if self.state.resolver_cosine_min is None
                else min(self.state.resolver_cosine_min, ResolverCosine)
            )
            self.state.resolver_cosine_max = (
                ResolverCosine if self.state.resolver_cosine_max is None
                else max(self.state.resolver_cosine_max, ResolverCosine)
            )
            count = int(self.state.resolver_capture_count_var.get() or 0) + 1
            self.state.resolver_capture_count_var.set(str(count))
            sine_offset = (self.state.resolver_sine_max + self.state.resolver_sine_min) / 2.0
            cosine_offset = (self.state.resolver_cosine_max + self.state.resolver_cosine_min) / 2.0
            sine_amplitude = (self.state.resolver_sine_max - self.state.resolver_sine_min) / 2.0
            cosine_amplitude = (self.state.resolver_cosine_max - self.state.resolver_cosine_min) / 2.0
            self.state.resolver_sine_offset_var.set(f"{sine_offset:.2f}")
            self.state.resolver_cosine_offset_var.set(f"{cosine_offset:.2f}")
            self.state.resolver_sine_amplitude_var.set(f"{sine_amplitude:.2f}")
            self.state.resolver_cosine_amplitude_var.set(f"{cosine_amplitude:.2f}")
            self.state.resolver_gain_ratio_var.set(
                "—" if cosine_amplitude == 0 else f"{sine_amplitude / cosine_amplitude:.4f}"
            )

        # Emf: для UI — по ключу "Emf", для расчёта — по "motorEmfCalc" (как в gui_ws)
        Emf_ui_raw = self._get_alias(d, "Emf")
        Emf_calc_raw = self._get_alias(
            d, "motorEmfCalc", Emf_ui_raw
        )  # motorEmfCalc / emf / E_back
        Emf = self._as_float(Emf_ui_raw)
        Emf_calc = self._as_float(Emf_calc_raw)

        We = self._as_float(self._get_alias(d, "Welectrical"))
        Wm = self._as_float(self._get_alias(d, "Wmechanical"))
        Rs = self._as_float(self._get_alias(d, "Rs"), DEFAULT_RS_OHMS)
        pp = self._as_float(self._get_alias(d, "polePairs"), DEFAULT_POLE_PAIRS)

        if Rs is not None:
            self._last_rs = Rs
        if pp is not None:
            self._last_pole_pairs = pp

        # --- дополнить недостающие величины ---
        # 1) если НЕТ электрической скорости, но есть мех. и пары полюсов → восстановить We
        if We is None and (Wm is not None) and (self._last_pole_pairs is not None):
            try:
                We = Wm * float(self._last_pole_pairs)
            except Exception:
                pass

        # 2) если НЕТ ns, но есть Wm → восстановить обороты
        if ns is None and Wm is not None:
            ns = Wm * 60.0 / (2.0 * math.pi)

        # --- обновить "панель индикации" (entry_vars) если есть ---
        ev = getattr(self.state, "entry_vars", None)
        if isinstance(ev, dict):

            def put(name: str, val):
                if name not in ev:  # UI создаст поле позже — игнорируем
                    return
                try:
                    ev[name].set("" if val is None else f"{val:.3f}")
                except Exception:
                    pass

            put("Ud", Ud)
            put("Uq", Uq)
            put("Id", Id)
            put("Iq", Iq)
            put("direct current (Idc)", Idc)
            put("Stator current d (Isd)", Isd)
            put("Stator current q (Isq)", Isq)
            put("DC voltage (Udc)", Udc)
            put("Torque (Ms)", Ms)
            put("Speed rotation", ns)
            put("Flux", Flux)
            put("Theta", Theta)
            put("Temperature", Temperature)
            put("Coolant temperature", coolant)
            put("IGBT temperature U", igbt_u)
            put("IGBT temperature V", igbt_v)
            put("IGBT temperature W", igbt_w)
            put("IGBT temperature Max", igbt_max)
            put("Stator temperature", stator)
            put("M max", m_max)
            put("M min", m_min)
            put("M grad max", m_grad_max)
            put("n max", n_max)

        # --- логбук (в таблицу) ---
        row = {
            "ts": datetime.now().strftime("%H:%M:%S.%f")[:-3],
            "ns": ns,
            "Ms": Ms,
            "Udc": Udc,
            "Idc": Idc,
            "Isd": Isd,
            "Isq": Isq,
            "Ud": Ud,
            "Uq": Uq,
            "Id": Id,
            "Iq": Iq,
            "Flux": Flux,
            "Theta": Theta,
            "Temperature": Temperature,
            "StatorTemperature": stator,
            "Emf": Emf,
            "Welectrical": We,
            "motorRs": Rs,
            "Wmechanical": Wm,
            "Rs": Rs,
            "TimeStamp": TimeStamp,
            "ThetaCorr": ThetaCorr,
            "MCU_IGBTTempU": igbt_u,
            "MCU_IGBTTempV": igbt_v,
            "MCU_IGBTTempW": igbt_w,
            "MCU_IGBTTempMax": igbt_max,
            "MCU_TempCurrStr": stator,
            "MCU_TempCurrCool": coolant,
            "M_max": m_max,
            "M_min": m_min,
            "MCU_OfsAl": mcu_ofs_al,
            "MCU_Isd": mcu_isd,
            "MCU_Isq": mcu_isq,
            "MCU_bDmpCActv": mcu_b_dmp,
            "MCU_stGateDrv": mcu_gate,
            "MCU_DmpCTrqCurr": mcu_dmp_trq,
            "MCU_VCUWorkMode": mcu_work_mode,
            "ResolverSine": ResolverSine,
            "ResolverCosine": ResolverCosine,
            "ResolverAmplitude": ResolverAmplitude,
            "ResolverTheta": ResolverTheta,
            "ResolverThetaCorr": ResolverThetaCorr,
            "FluxPositionError": FluxPositionError,
            "ResolverThetaCorrection": ResolverThetaCorrection,
            "ResolverElectricalSpeed": ResolverElectricalSpeed,
            "ResolverCalibrationState": ResolverCalibrationState,
            "CanMode": can_mode,
        }
        for sample in getattr(self.state, "latest_dbc_signals", []) or []:
            if not isinstance(sample, dict):
                continue
            col = self._dbc_log_column(sample)
            if col:
                row[col] = self._as_float(sample.get("physical"), sample.get("physical"))
        self._append_log_row(row)

        # --- буферы для трендов ---
        self._push(self.state.trend_theta_ts, TimeStamp)
        self._push(self.state.trend_theta, Theta)

        # --- прямые Ld/Lq из телеметрии, если приходят ---
        Ld_direct = self._as_float(d.get("Ld"))
        Lq_direct = self._as_float(d.get("Lq"))
        if Ld_direct is not None and Id is not None and math.isfinite(Ld_direct):
            self._push(self.state.map_Id, Id)
            self._push(self.state.map_Ld, Ld_direct)
        if Lq_direct is not None and Iq is not None and math.isfinite(Lq_direct):
            self._push(self.state.map_Iq, Iq)
            self._push(self.state.map_Lq, Lq_direct)

        # --- расчёт Ld/Lq (онлайн по напряжениям/токам) ---
        # Модель PMSM (упрощ.): v_d = R_s i_d - ω L_q i_q
        #                       v_q = R_s i_q + ω L_d i_d + ω ψ_f  (ψ_f ≈ Emf/ω)
        
        Ld = None
        Lq = None

        psi_f = None
        if Emf_calc is not None and We not in (None, 0.0):
            try:
                psi_f = Emf_calc / We
            except Exception:
                psi_f = None

        # Lq = (Ud - Rs*Id) / (ω * Iq)  (как в gui_ws)
        if (
            Ud is not None
            and Id is not None
            and Iq not in (None, 0.0)
            and We not in (None, 0.0)
        ):
            try:
                R = Rs or self._last_rs
                Lq = (Ud - R * Id) / (We * Iq)
            except Exception:
                Lq = None

        # Ld = (Uq - Rs*Iq - ω*psi_f) / (ω * Id)
        if (
            Uq is not None
            and Id not in (None, 0.0)
            and Iq is not None
            and We not in (None, 0.0)
            and (Rs or self._last_rs) is not None
            and psi_f is not None
        ):
            try:
                R = Rs or self._last_rs
                Ld = (Uq - R * Iq - We * psi_f) / (We * Id)
            except Exception:
                Ld = None

        if Ld is not None and Id is not None and math.isfinite(Ld):
            self._push(self.state.map_Id, Id)
            self._push(self.state.map_Ld, Ld)
        if Lq is not None and Iq is not None and math.isfinite(Lq):
            self._push(self.state.map_Iq, Iq)
            self._push(self.state.map_Lq, Lq)

        # --- карта Torque/Power vs RPM ---
        rpm = ns
        if rpm is not None:
            self._push(self.state.map_ns, rpm)
            # P_mech = τ * ω_m (Вт) → кВт
            p_mech = None
            if Ms is not None and Wm is not None:
                p_mech = Ms * Wm / 1000.0
            # P_elec = u_d i_d + u_q i_q (Вт) → кВт
            p_elec = None
            if (
                Ud is not None
                and Id is not None
                and Uq is not None
                and Iq is not None
            ):
                p_elec = (Ud * Id + Uq * Iq) / 1000.0

            self._push(self.state.map_Ms, Ms)
            self._push(self.state.map_Pmech, p_mech)
            self._push(self.state.map_Pelec, p_elec)

    # ---------- вспомогательные ----------

    def _append_log_row(self, row: dict):
        """Добавить строку в буфер и в Treeview (если включено логирование)."""
        # как в gui_ws: если лог отключён — вообще ничего не пишем
        if not getattr(self.state, "log_enabled", None) or not self.state.log_enabled.get():
            return

        fixed = set(TELEM_COLUMNS)
        dynamic = getattr(self.state, "dynamic_log_columns", None)
        if dynamic is None:
            dynamic = []
            self.state.dynamic_log_columns = dynamic
        for col in row:
            if col not in fixed and "." in col and col not in dynamic:
                dynamic.append(col)

        # буфер
        self.state.log_rows.append(row)
        if len(self.state.log_rows) > self.state.max_rows:
            self.state.log_rows.pop(0)

        # таблица
        tree = getattr(self.views, "telem_tree", getattr(self.state, "telem_tree", None))
        if tree is None:
            return

        columns = list(TELEM_COLUMNS) + list(dynamic)
        try:
            if tuple(tree["columns"]) != tuple(columns):
                tree.configure(columns=columns)
                for col in columns:
                    tree.heading(col, text=col)
                    width = 100 if col in fixed else max(120, min(220, len(col) * 8))
                    tree.column(col, width=width, anchor="center")
        except Exception:
            pass

        values = []
        for col in columns:
            v = row.get(col, "")
            values.append(self._format_log_value(v))

        try:
            tree.insert("", "end", values=values)
            tree.yview_moveto(1.0) # автопрокрутка вниз
            # подрежем старые строки визуально, если очень много
            if len(tree.get_children()) > self.state.max_rows:
                tree.delete(tree.get_children()[0])
        except Exception:
            pass

    @staticmethod
    def _format_log_value(v):
        if isinstance(v, float):
            if math.isfinite(v):
                return f"{v:.3f}"
            return ""
        return v

    @staticmethod
    def _push(deq, val):
        if deq is None:
            return
        if val is None:
            return
        try:
            if isinstance(val, float) and not math.isfinite(val):
                return
            deq.append(val)
        except Exception:
            pass

    # ---------- рендер графиков (периодический) ----------

    def _tick_trends(self):
        try:
            tr = getattr(self.state, "trends", {}) or {}
            if not tr:
                return
            axes = tr.get("axes")
            lines = tr.get("lines")
            fig = tr.get("fig") or tr.get("figure")

            if axes and lines and self.state.trend_theta and self.state.trend_theta_ts:
                n = min(len(self.state.trend_theta), len(self.state.trend_theta_ts))
                lines[0].set_data(
                    list(self.state.trend_theta_ts)[-n:],
                    list(self.state.trend_theta)[-n:],
                )
                for ax in axes:
                    try:
                        ax.relim()
                        ax.autoscale_view()
                    except Exception:
                        pass
                if fig:
                    try:
                        fig.canvas.draw_idle()
                    except Exception:
                        pass
                return

            # ожидаем порядок линий: (l_ns,l_ms,l_idc,l_isd,l_id,l_iq,l_ud,l_uq)
            # и 4 оси: ax1..ax4
            if axes and lines and len(lines) >= 9:
                # считаем ось X как в gui_ws: время относительно последней точки
                ts = self.state.trend_ts
                if ts:
                    t0 = ts[-1]
                    xs_all = [(t - t0).total_seconds() for t in ts]

                    # ns, Ms
                    if self.state.trend_ns:
                        n = len(self.state.trend_ns)
                        xs = xs_all[-n:]
                        lines[0].set_data(xs, list(self.state.trend_ns))
                    if self.state.trend_Ms:
                        n = len(self.state.trend_Ms)
                        xs = xs_all[-n:]
                        lines[1].set_data(xs, list(self.state.trend_Ms))

                    # Idc / Isd
                    if self.state.trend_Idc:
                        n = len(self.state.trend_Idc)
                        xs = xs_all[-n:]
                        lines[2].set_data(xs, list(self.state.trend_Idc))
                    if self.state.trend_Isd:
                        n = len(self.state.trend_Isd)
                        xs = xs_all[-n:]
                        lines[3].set_data(xs, list(self.state.trend_Isd))

                    # Id/Iq/Ud/Uq — общая ось времени
                    if self.state.trend_Id:
                        n = len(self.state.trend_Id)
                        xs = xs_all[-n:]
                        lines[4].set_data(xs, list(self.state.trend_Id))
                    if self.state.trend_Iq:
                        n = len(self.state.trend_Iq)
                        xs = xs_all[-n:]
                        lines[5].set_data(xs, list(self.state.trend_Iq))
                    if self.state.trend_Ud:
                        n = len(self.state.trend_Ud)
                        xs = xs_all[-n:]
                        lines[6].set_data(xs, list(self.state.trend_Ud))
                    if self.state.trend_Uq:
                        n = len(self.state.trend_Uq)
                        xs = xs_all[-n:]
                        lines[7].set_data(xs, list(self.state.trend_Uq))

                    if self.state.trend_theta and self.state.trend_theta_ts:
                        n = min(len(self.state.trend_theta), len(self.state.trend_theta_ts))
                        lines[8].set_data(
                            list(self.state.trend_theta_ts)[-n:],
                            list(self.state.trend_theta)[-n:],
                        )

                    # autoscale
                    for ax in axes:
                        try:
                            ax.set_xlabel("seconds from now")
                            ax.relim()
                            ax.autoscale_view()
                        except Exception:
                            pass

                    if fig:
                        try:
                            fig.canvas.draw_idle()
                        except Exception:
                            pass
        finally:
            self.root.after(500, self._tick_trends)

    def _tick_maps(self):
        try:
            mp = getattr(self.state, "maps", {}) or {}
            if not mp:
                return
            fig = mp.get("fig") or mp.get("figure")

            # Ld(Id)
            sc_ld = mp.get("sc_ld")
            if sc_ld:
                try:
                    sc_ld.set_data(list(self.state.map_Id), list(self.state.map_Ld))
                    ax = mp.get("ax5a")
                    if ax:
                        ax.relim()
                        ax.autoscale_view()
                except Exception:
                    pass

            # Lq(Iq)
            sc_lq = mp.get("sc_lq")
            if sc_lq:
                try:
                    sc_lq.set_data(list(self.state.map_Iq), list(self.state.map_Lq))
                    ax = mp.get("ax5b")
                    if ax:
                        ax.relim()
                        ax.autoscale_view()
                except Exception:
                    pass

            # Torque & Power vs RPM
            ln_torque = mp.get("ln_torque")
            ln_pmech = mp.get("ln_pmech")
            ln_pelec = mp.get("ln_pelec")
            if ln_torque:
                try:
                    x = list(self.state.map_ns)

                    def _set_line(line, y_values):
                        y = list(y_values)
                        if not y:
                            line.set_data([], [])
                            return
                        n = min(len(x), len(y))
                        line.set_data(x[-n:], y[-n:])

                    _set_line(ln_torque, self.state.map_Ms)
                    if ln_pmech:
                        _set_line(ln_pmech, self.state.map_Pmech)
                    if ln_pelec:
                        _set_line(ln_pelec, self.state.map_Pelec)
                    ax6 = mp.get("ax6")
                    ax6r = mp.get("ax6r") or mp.get("ax6_right")
                    for ax in (ax6, ax6r):
                        if ax:
                            ax.relim()
                            ax.autoscale_view()
                except Exception:
                    pass

            if fig:
                try:
                    fig.canvas.draw_idle()
                except Exception:
                    pass
        finally:
            self.root.after(800, self._tick_maps)
