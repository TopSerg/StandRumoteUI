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
        self._tr = getattr(views, "trends", getattr(state, "trends", {}))
        self._mp = getattr(views, "maps", getattr(state, "maps", {}))

        self._last_rs = DEFAULT_RS_OHMS
        self._last_pole_pairs = DEFAULT_POLE_PAIRS

    # ----------------- публичный API для WSClient -----------------

    def start_timers(self):
        # периодический рефреш графиков (минимальный, чтобы не грузить CPU)
        self.root.after(500, self._tick_trends)
        self.root.after(800, self._tick_maps)

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
                "Emf",
                "Welectrical",
                "Wmechanical",
            }:
                self._handle_model_data(data)
                return

            # fallback — просто сообщим в лог
            self.ui_log(f"[RX] {data}")

        except Exception as ex:
            self.ui_log(f"[TELEM] handler error: {ex}")

    # ---------- CAN ----------

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
        # --- считать основные величины (с алиасами) ---
        Ud = self._as_float(self._get_alias(d, "Ud"))
        Uq = self._as_float(self._get_alias(d, "Uq"))
        Id = self._as_float(self._get_alias(d, "Id"))
        Iq = self._as_float(self._get_alias(d, "Iq"))
        Idc = self._as_float(d.get("Idc"))
        Isd = self._as_float(d.get("Isd"))
        Ms = self._as_float(d.get("Ms"))
        ns = self._as_float(d.get("ns"))

        igbt = self._as_float(d.get("MCU_IGBTTempU"))
        stator = self._as_float(d.get("MCU_TempCurrStr"))

        Flux = self._as_float(self._get_alias(d, "Flux"))
        Theta = self._as_float(self._get_alias(d, "Theta"))
        Temperature = self._as_float(self._get_alias(d, "Temperature"))

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
            put("Torque (Ms)", Ms)
            put("Speed rotation", ns)
            put("Flux", Flux)
            put("Theta", Theta)
            put("Temperature", Temperature)
            put("IGBT temperature", igbt)
            put("Stator temperature", stator)

        # --- логбук (в таблицу) ---
        self._append_log_row(
            {
                "ts": datetime.now().strftime("%H:%M:%S.%f")[:-3],
                "ns": ns,
                "Ms": Ms,
                "Idc": Idc,
                "Isd": Isd,
                "Ud": Ud,
                "Uq": Uq,
                "Id": Id,
                "Iq": Iq,
                "Flux": Flux,
                "Theta": Theta,
                "Temperature": Temperature,
                "Emf": Emf,
                "Welectrical": We,
                "motorRs": Rs,
                "Wmechanical": Wm,
            }
        )

        # --- буферы для трендов ---
        self._push(self.state.trend_ns, ns)
        self._push(self.state.trend_Ms, Ms)
        self._push(self.state.trend_Idc, Idc)
        self._push(self.state.trend_Isd, Isd)
        self._push(self.state.trend_Ud, Ud)
        self._push(self.state.trend_Uq, Uq)
        self._push(self.state.trend_Id, Id)
        self._push(self.state.trend_Iq, Iq)
        self._push(self.state.trend_ts, datetime.now())

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

        # буфер
        self.state.log_rows.append(row)
        if len(self.state.log_rows) > self.state.max_rows:
            self.state.log_rows.pop(0)

        # таблица
        tree = getattr(self.views, "telem_tree", getattr(self.state, "telem_tree", None))
        if tree is None:
            return

        values = []
        for col in TELEM_COLUMNS:
            v = row.get(col, "")
            if isinstance(v, float):
                if math.isfinite(v):
                    v = f"{v:.3f}"
                else:
                    v = ""
            values.append(v)

        try:
            tree.insert("", "end", values=values)
            tree.yview_moveto(1.0) # автопрокрутка вниз
            # подрежем старые строки визуально, если очень много
            if len(tree.get_children()) > self.state.max_rows:
                tree.delete(tree.get_children()[0])
        except Exception:
            pass

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
            tr = self._tr
            if not tr:
                return
            axes = tr.get("axes")
            lines = tr.get("lines")
            fig = tr.get("fig") or tr.get("figure")

            # ожидаем порядок линий: (l_ns,l_ms,l_idc,l_isd,l_id,l_iq,l_ud,l_uq)
            # и 4 оси: ax1..ax4
            if axes and lines and len(lines) >= 8:
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
            mp = self._mp
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
                x = list(self.state.map_ns)
                try:
                    ln_torque.set_data(x, list(self.state.map_Ms))
                    if ln_pmech:
                        ln_pmech.set_data(x, list(self.state.map_Pmech))
                    if ln_pelec:
                        ln_pelec.set_data(x, list(self.state.map_Pelec))
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
