import re
import sys
import tkinter as tk
import unittest
from pathlib import Path
from types import SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "client"))

from controllers import Controllers
from state import AppState, FIELD_SPECS
from telemetry import Telemetry


class ImmediateRoot:
    def after(self, _delay, callback, *args):
        return callback(*args)


class DummyClient:
    def __init__(self):
        self.payloads = []

    def send_json_threadsafe(self, payload):
        self.payloads.append(payload)


class ClientServerContractTests(unittest.TestCase):
    def setUp(self):
        self.tcl = tk.Tcl()
        self.state = AppState(self.tcl)

    def test_every_indication_field_is_serialized_by_server(self):
        server = (ROOT / "server" / "ws_server.cpp").read_text(encoding="utf-8")
        serialized = set(re.findall(r'j\["([^"]+)"\]\s*=', server))
        self.assertEqual([], sorted(set(FIELD_SPECS) - serialized))

    def test_server_sample_populates_canonical_ui_fields(self):
        for key in FIELD_SPECS:
            self.state.entry_vars[key] = tk.StringVar(master=self.tcl, value="—")
        views = SimpleNamespace(telem_tree=None)
        telemetry = Telemetry(ImmediateRoot(), self.state, views, ui_log=lambda *_: None)

        sample = {key: float(index + 1) for index, key in enumerate(FIELD_SPECS)}
        sample.update({
            "Kl_15": True,
            "En_Is": False,
            "MCU_bDmpCActv": True,
            "MCU_ActualTorqueValid": True,
            "MCU_ActualSpeedValid": False,
            "MCU_SW_ver": "v42",
            "ResolverCalibrationStatus": 3,
            "ResolverCalibrationAckSequence": 17,
            "ResolverCalibrationCommand": 0.12345,
            "ResolverCalibrationError": -0.01234,
            "ResolverCalibrationState": "measuring",
            "ResolverCalibrationStatusCount": 42,
            "ResolverCalibrationCommandSequence": 7,
            "ResolverCalibrationEnableCommand": 1,
            "ResolverCalibrationActive": True,
            "ResolverCalibrationConverged": False,
            "can_mode": "resolver_rx",
            "can_rx_only": True,
        })

        telemetry._handle_model_data(sample)

        self.assertEqual("yes", self.state.entry_vars["Kl_15"].get())
        self.assertEqual("no", self.state.entry_vars["En_Is"].get())
        self.assertEqual("v42", self.state.entry_vars["MCU_SW_ver"].get())
        self.assertEqual("yes", self.state.resolver_flux_valid_var.get())
        self.assertEqual("yes", self.state.resolver_command_active_var.get())
        self.assertEqual("17", self.state.resolver_ack_sequence_var.get())
        self.assertEqual("0.12345", self.state.resolver_command_var.get())
        self.assertEqual("-0.01234", self.state.resolver_loop_error_var.get())
        self.assertEqual("42", self.state.resolver_status_count_var.get())
        self.assertEqual("7", self.state.resolver_command_sequence_var.get())
        self.assertEqual("yes", self.state.resolver_enable_command_var.get())
        self.assertEqual("yes", self.state.resolver_active_var.get())
        self.assertEqual("no", self.state.resolver_converged_var.get())
        self.assertEqual("ACTIVE — RX ONLY — TX BLOCKED", self.state.resolver_mode_var.get())

    def test_connection_status_is_not_replaced_by_send_notifications(self):
        telemetry = Telemetry(
            ImmediateRoot(), self.state, SimpleNamespace(telem_tree=None), ui_log=lambda *_: None
        )
        telemetry.on_status("WS connected")
        self.assertEqual("connected", self.state.conn_var.get())
        self.assertEqual("#1bb55c", self.state.conn_color.get())

        telemetry.on_status("sent: SendControl")
        self.assertEqual("connected", self.state.conn_var.get())
        self.assertEqual("#1bb55c", self.state.conn_color.get())

        telemetry.on_status("WS disabled")
        self.assertEqual("disabled", self.state.conn_var.get())
        self.assertEqual("#d72c20", self.state.conn_color.get())

    def test_torque_and_speed_use_server_canonical_setpoint(self):
        controller = Controllers(ImmediateRoot(), self.state)
        client = DummyClient()
        controller.bind_network(client)
        self.state.control_armed_var.set(True)

        self.state.mode_var.set("torque")
        self.state.torque_var.set(12.5)
        controller.send_control_now()
        torque_payload = client.payloads[-1]
        self.assertEqual(12.5, torque_payload["M_desired"])
        self.assertNotIn("Ms", torque_payload)

        self.state.mode_var.set("speed")
        self.state.speed_var.set(650)
        controller.send_control_now()
        speed_payload = client.payloads[-1]
        self.assertEqual(650.0, speed_payload["M_desired"])
        self.assertNotIn("ns", speed_payload)

    def test_nonzero_current_is_blocked_until_arm(self):
        controller = Controllers(ImmediateRoot(), self.state)
        client = DummyClient()
        controller.bind_network(client)
        self.state.Id_var.set("1.0")
        controller.send_torque_now()
        self.assertEqual([], client.payloads)


if __name__ == "__main__":
    unittest.main()
