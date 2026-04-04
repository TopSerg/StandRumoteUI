from __future__ import annotations

import csv
from datetime import datetime
from typing import Optional, Callable

from state import GEAR_MAP, TELEM_COLUMNS, AppState, MOTOR_MODE_MAP


class Controllers:
    """
    Собирает весь UI-контрол: обработчики кнопок, переключателей, передач.
    Ничего «не рисует» — только читает/пишет state и вызывает методы сети.
    """

    def __init__(self, root, state: AppState):
        self.root = root
        self.state = state
        self.views = None          # присвоится через attach_views()
        self.client = None         # присвоится через bind_network()
        self.client_factory = None  # функция (url)->WSClient

    # ---- wiring ----

    def attach_views(self, views) -> None:
        """Даём контроллеру ссылки на виджеты (log_box, telem_tree, sliders...)."""
        self.views = views

    def bind_network(self, client) -> None:
        """Подключаем WSClient (из network.py)."""
        self.client = client

    def bind_network_factory(self, factory) -> None:
        """Передаём фабрику WSClient(url)->client."""
        self.client_factory = factory

    def _normalize_ws_url(self, addr: str) -> str:
        a = (addr or "").strip()
        if not a:
            return ""
        # разрешим ввод "192.168.1.10" или "192.168.1.10:9000" или "ws://..."
        if a.startswith("ws://") or a.startswith("wss://"):
            return a
        if ":" not in a:
            a = a + ":9000"
        return "ws://" + a


    def disconnect_ws(self) -> None:
        if self.client:
            try:
                self.client.stop()
            except Exception:
                pass
            self.client = None


    def connect_ws(self, addr: str) -> None:
        if not self.client_factory:
            self.ui_log("[WS] client factory not set", "ERR")
            return

        url = self._normalize_ws_url(addr)
        if not url:
            self.ui_log("[WS] empty address", "ERR")
            return

        # остановим прошлый клиент, если был
        self.disconnect_ws()

        # создаём новый
        try:
            client = self.client_factory(url)
        except Exception as e:
            self.ui_log("[WS] create failed:", e, "ERR")
            return

        self.client = client
        try:
            client.start()
            self.ui_log("[WS] connect:", url)
        except Exception as e:
            self.ui_log("[WS] start failed:", e, "ERR")


    # ---- утилиты ----

    def ui_log(self, *parts) -> None:
        """Единая точка логирования в текстовое окно (и в будущем — в статусбар)."""
        if not self.views or not getattr(self.views, "log_box", None):
            return
        msg = " ".join(str(p) for p in parts).strip()
        self.views.log_box.insert("end", msg + "\n")
        self.views.log_box.see("end")

    def _get_float(self, var, name: str) -> float:
        try:
            return float(var.get() or 0.0)
        except Exception:
            self.ui_log(f"[UI] {name}: некорректное значение", "ERR")
            raise

    def _get_int(self, var, name: str) -> int:
        try:
            return int(float(var.get()))
        except Exception:
            self.ui_log(f"[UI] {name}: некорректное значение", "ERR")
            raise

    def _gear_code_or_none(self) -> Optional[int]:
        try:
            return GEAR_MAP.get(self.state.gear_var.get(), 2)  # 2 = N
        except Exception:
            return None

    # ---- публичные хэндлеры, которые прокинем во view ----

    def handlers(self) -> dict[str, Callable]:
        return {
            "send_all": self.send_all,
            "send_cmd": self.send_cmd,
            "set_mode": self.set_mode,
            "set_gear": self.set_gear_from_ui,
            "send_limits": self.send_limits_now,
            "send_torque": self.send_torque_now,
            "send_control_now": self.send_control_now,
            "apply_mode": self.apply_mode,           # аналог старого set_mode_from_ui
            "set_mode_from_ui": self.apply_mode,     # синоним для совместимости

            "connect_ws": self.connect_ws_from_ui,

            # синонимы на всякий случай
            "send_limits_now": self.send_limits_now,
            "send_torque_now": self.send_torque_now,

            # опционально — пригодится во view:
            "on_speed_released": self.on_speed_released,
            "on_torque_released": self.on_torque_released,
            "toggle_logging": self.toggle_logging,
            "clear_log": self.clear_log,
            "export_csv": self.export_csv,
            "send_fake_can": self.send_fake_can_from_fields,
        }

    # ---- отправка «сервисных» команд ----

    def send_cmd(self, cmd: str) -> None:
        if not self.client:
            self.ui_log("[WS] клиент не привязан", "ERR")
            return
        self.client.send_cmd_threadsafe(cmd)

    # ---- основная кнопка "Отправить" ----

    def send_all(self) -> None:
        """
        Поведение совпадает с исходным:
        - сначала всегда SendLimits
        - режим 'currents': SendControl(En_Is=1, Kl_15=0 [+GearCtrl, MotorCtrl, ReqState])
                           затем SendTorque(Isd/Iq)
        - режим 'speed'   : SendControl(En_Is=0, Kl_15=1, ns [+GearCtrl, MotorCtrl, ReqState])
        - режим 'torque'  : SendControl(En_Is=0, Kl_15=1, Ms [+GearCtrl, MotorCtrl, ReqState])
        """
        if not self.client:
            self.ui_log("[WS] клиент не привязан", "ERR")
            return

        # как в старом gui_ws: сначала отправляем лимиты
        self.send_limits_now()

        gear_code = self._gear_code_or_none()
        mode = self.state.mode_var.get()
        motor_code = MOTOR_MODE_MAP.get(mode, 1)

        if mode == "currents":
            # режим тока: общий контроль + токи Id/Iq
            try:
                isd = self._get_float(self.state.Id_var, "Id")
                isq = self._get_float(self.state.Iq_var, "Iq")
            except Exception:
                return

            ctrl = {
                "cmd": "SendControl",
                "En_Is": True,
                "Kl_15": False,
            }
            if gear_code is not None:
                ctrl["GearCtrl"] = int(gear_code)
            if motor_code is not None:
                ctrl["MotorCtrl"] = int(motor_code)
                ctrl["ReqState"] = int(motor_code)

            self.client.send_json_threadsafe(ctrl)
            self.client.send_json_threadsafe({
                "cmd": "SendTorque",
                "En_Is": True,
                "Isd": isd,
                "Isq": isq,
            })
            self.ui_log(
                f"[UI] ▶ Отправлено: режим Токи (Id/Iq={isd:.2f}/{isq:.2f})"
                + (f", Gear={gear_code}" if gear_code is not None else "")
            )

        elif mode == "torque":
            # моментный режим: посылаем Ms
            try:
                Ms = self._get_float(self.state.torque_var, "Ms")
            except Exception:
                return

            ctrl = {
                "cmd": "SendControl",
                "En_Is": False,
                "Kl_15": True,
                "Ms": Ms,
            }
            if gear_code is not None:
                ctrl["GearCtrl"] = int(gear_code)
            if motor_code is not None:
                ctrl["MotorCtrl"] = int(motor_code)
                ctrl["ReqState"] = int(motor_code)

            self.client.send_json_threadsafe(ctrl)
            self.ui_log(
                f"[UI] ▶ Отправлено: режим Момент (Ms={Ms:.1f})"
                + (f", Gear={gear_code}" if gear_code is not None else "")
            )

        else:
            # режим частоты: только SendControl с ns
            try:
                ns = self._get_float(self.state.speed_var, "ns")
            except Exception:
                return

            ctrl = {
                "cmd": "SendControl",
                "En_Is": False,
                "Kl_15": True,
                "ns": ns,
            }
            if gear_code is not None:
                ctrl["GearCtrl"] = int(gear_code)
            if motor_code is not None:
                ctrl["MotorCtrl"] = int(motor_code)
                ctrl["ReqState"] = int(motor_code)

            self.client.send_json_threadsafe(ctrl)
            self.ui_log(
                f"[UI] ▶ Отправлено: режим Частота (ns={ns:.0f})"
                + (f", Gear={gear_code}" if gear_code is not None else "")
            )

    # ---- точечные команды (кнопки в блоках) ----

    def send_control_now(self) -> None:
        """Применить текущий режим и ключевые флаги (и, если режим 'speed/torque', то ns/Ms)."""
        if not self.client:
            self.ui_log("[WS] клиент не привязан", "ERR")
            return

        mode = self.state.mode_var.get()
        motor_code = MOTOR_MODE_MAP.get(mode, 1)

        if mode == "speed":
            try:
                ns = self._get_float(self.state.speed_var, "ns")
            except Exception:
                return
            payload = {
                "cmd": "SendControl",
                "En_Is": False,
                "Kl_15": True,
                "ns": ns,
            }
            if motor_code is not None:
                payload["MotorCtrl"] = int(motor_code)
                payload["ReqState"] = int(motor_code)
            self.client.send_json_threadsafe(payload)
            self.ui_log(f"[UI] SendControl: Частота (ns={ns:.0f}, MotorCtrl={motor_code})", "UI")

        elif mode == "torque":
            try:
                Ms = self._get_float(self.state.torque_var, "Ms")
            except Exception:
                return
            payload = {
                "cmd": "SendControl",
                "En_Is": False,  # momentный режим — En_Is=0, Kl_15=1
                "Kl_15": True,
                "Ms": Ms,
            }
            if motor_code is not None:
                payload["MotorCtrl"] = int(motor_code)
                payload["ReqState"] = int(motor_code)
            self.client.send_json_threadsafe(payload)
            self.ui_log(f"[UI] SendControl: Момент (Ms={Ms:.1f}, MotorCtrl={motor_code})", "UI")

        else:
            payload = {
                "cmd": "SendControl",
                "En_Is": True,
                "Kl_15": False,
            }
            if motor_code is not None:
                payload["MotorCtrl"] = int(motor_code)
                payload["ReqState"] = int(motor_code)
            self.client.send_json_threadsafe(payload)
            self.ui_log("[UI] SendControl: Токи (En_Is=1, Kl_15=0, MotorCtrl={})".format(motor_code), "UI")

    def send_limits_now(self) -> None:
        """Отправить лимиты (M_min/M_max/M_grad_max/n_max)."""
        if not self.client:
            self.ui_log("[WS] клиент не привязан", "ERR")
            return

        try:
            payload = {
                "cmd": "SendLimits",
                "M_min": self._get_float(self.state.M_min_var, "M_min"),
                "M_max": self._get_float(self.state.M_max_var, "M_max"),
                "M_grad_max": self._get_int(self.state.M_grad_max_var, "M_grad_max"),
                "n_max": self._get_int(self.state.n_max_var, "n_max"),
            }
            print(f"M_min = {payload['M_min']}, M_max = {payload['M_max']}")
        except Exception:
            return

        self.client.send_json_threadsafe(payload)
        self.ui_log("[UI] SendLimits", payload, "UI")

    def send_torque_now(self) -> None:
        """Отправить Id/Iq (всегда с En_Is=True, чтобы зафиксировать токовый режим)."""
        if not self.client:
            self.ui_log("[WS] клиент не привязан", "ERR")
            return
        try:
            Id = self._get_float(self.state.Id_var, "Id")
            Iq = self._get_float(self.state.Iq_var, "Iq")
        except Exception:
            return

        self.client.send_json_threadsafe({
            "cmd": "SendTorque",
            "En_Is": True,
            "Isd": Id,
            "Isq": Iq
        })
        self.ui_log(f"[UI] SendTorque: Id={Id:.2f}, Iq={Iq:.2f}", "UI")

    # ---- режим и передача ----

    def set_mode(self, val: str) -> None:
        """Выбор режима из UI (радиокнопки)."""
        self.state.mode_var.set(val)
        self.update_mode_controls()
        self.ui_log(
            "[UI] Режим выбран: "
            + (
                "Токи (Id/Iq) — будет отправлено En_Is=1, Kl_15=0"
                if val == "currents" else
                "Момент (Ms) — будет отправлено En_Is=0, Kl_15=1"
                if val == "torque" else
                "Частота (ns) — будет отправлено En_Is=0, Kl_15=1"
            )
            + " → нажмите «Отправить»"
        )

    def apply_mode(self) -> None:
        """
        Аналог старого set_mode_from_ui:
        - speed/torque: только SendControl
        - currents: SendControl + SendTorque
        """
        mode = self.state.mode_var.get()
        if mode in ("speed", "torque"):
            self.send_control_now()
        else:  # currents
            self.send_control_now()
            self.send_torque_now()

    def set_gear_from_ui(self) -> None:
        """Кнопка/радиокнопка передачи."""
        if not self.client:
            self.ui_log("[WS] клиент не привязан", "ERR")
            return
        sel = self.state.gear_var.get()
        code = GEAR_MAP[sel]
        self.client.send_json_threadsafe({
            "cmd": "SendControl",
            "GearCtrl": int(code)
        })
        self.ui_log(f"[UI] Gear set to {sel} (code {code})", "UI")

    def connect_ws_from_ui(self) -> None:
        addr = ""
        try:
            addr = self.state.ws_addr_var.get()
        except Exception:
            pass
        self.connect_ws(addr)

    # ---- визуальные состояния (enable/disable) ----

    def update_mode_controls(self) -> None:
        """
        Настройка UI под выбранный режим.
        1) Если view предоставляет колбэк configure_main_slider(mode) — делегируем ему.
        2) Иначе: если мэппинг speed_* и torque_* указывает на ОДИН и тот же виджет,
           ничего не выключаем (единый ползунок). Если разные — старая логика.
        """
        # 1) делегируем, если есть
        configure = getattr(self.views, "configure_main_slider", None) if self.views else None
        if callable(configure):
            try:
                configure(self.state.mode_var.get())
            finally:
                return

        # 2) совместимость со старой раскладкой
        if not self.views or not getattr(self.views, "widgets", None):
            return
        w = self.views.widgets

        ss = w.get("speed_slider"); se = w.get("speed_entry")
        ts = w.get("torque_slider"); te = w.get("torque_entry")

        same_slider = (ss is not None and ss is ts)
        same_entry  = (se is not None and se is te)

        # Единый ползунок/поле — ничего не дизейблим
        if same_slider or same_entry:
            return

        # Старая двухползунковая логика
        mode = self.state.mode_var.get()
        if mode == "speed":
            for obj in (ss,):
                try: obj.state(["!disabled"])
                except Exception: pass
            for obj in (se,):
                try: obj.config(state="normal")
                except Exception: pass
            for obj in (ts,):
                try: obj.state(["disabled"])
                except Exception: pass
            for obj in (te,):
                try: obj.config(state="disabled")
                except Exception: pass
        else:
            for obj in (ss,):
                try: obj.state(["disabled"])
                except Exception: pass
            for obj in (se,):
                try: obj.config(state="disabled")
                except Exception: pass
            for obj in (ts,):
                try: obj.state(["!disabled"])
                except Exception: pass
            for obj in (te,):
                try: obj.config(state="normal")
                except Exception: pass

    # ---- уведомления по отпусканию слайдеров (как в исходнике) ----

    def on_speed_released(self, _evt=None) -> None:
        if self.state.mode_var.get() == "speed":
            self.ui_log("[UI] ns изменён локально → нажмите «Отправить»")

    def on_torque_released(self, _evt=None) -> None:
        if self.state.mode_var.get() == "currents":
            self.ui_log("[UI] Id/Iq изменены локально → нажмите «Отправить»")

    # ---- журнал телеметрии (верхняя вкладка Logbook) ----

    def toggle_logging(self) -> None:
        self.ui_log("📒 logging:", "ON" if self.state.log_enabled.get() else "OFF")

    def clear_log(self) -> None:
        self.state.log_rows.clear()
        tree = getattr(self.views, "telem_tree", None)
        if tree is not None:
            for i in tree.get_children():
                tree.delete(i)
        self.ui_log("🧹 journal cleared")

    def export_csv(self) -> None:
        # простой экспорт в файл рядом с клиентом
        fname = f"logbook_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        with open(fname, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f, delimiter=";")
            w.writerow(TELEM_COLUMNS)
            for row in self.state.log_rows:
                w.writerow([row.get(k, "") for k in TELEM_COLUMNS])
        self.ui_log(f"💾 exported: {fname}")

    # ---- утилита для отправки тестового CAN из полей (как в исходнике) ----

    def send_fake_can_from_fields(self) -> None:
        """Собрать и отправить тестовый CAN-кадр из текущих UI-полей."""
        if not self.client:
            self.ui_log("[WS] клиент не привязан", "ERR")
            return
        try:
            Id = self._get_float(self.state.Id_var, "Id")
            Iq = self._get_float(self.state.Iq_var, "Iq")
            torque = self._get_float(self.state.torque_var, "Ms")
            speed = self._get_float(self.state.speed_var, "ns")
        except Exception:
            return

        can_msg = {
            "cmd": "FakeCAN",
            "direction": "tx",
            "id": 0x555,
            "len": 8,
            "flags": 0,
            "ts": 10,  # как в старом gui_ws (можно заменить на time.time() при желании)
            "data0": int(Id * 10) & 0xFF,
            "data1": int(Iq * 10) & 0xFF,
            "data2": int(torque) & 0xFF,
            "data3": int(speed / 10) & 0xFF,
            "data4": 0,
            "data5": 0,
            "data6": 0,
            "data7": 0,
        }
        self.client.send_json_threadsafe(can_msg)
        self.ui_log("[UI] FakeCAN sent", can_msg)
