"""Unit tests for the Quake III input bridge validation (no game required)."""
import os
import unittest
from unittest import mock

import input_bridge


class InputBridgeTests(unittest.TestCase):
    def test_rejects_unknown_fields(self):
        with self.assertRaises(ValueError):
            input_bridge.execute_action({"move": "forward", "console": "quit"})

    def test_rejects_invalid_direction(self):
        with self.assertRaises(ValueError):
            input_bridge.execute_action({"move": "teleport"})

    def test_rejects_unbounded_duration(self):
        with self.assertRaises(ValueError):
            input_bridge.execute_action({"duration_ms": 251})

    def test_rejects_invalid_mouse_delta(self):
        with self.assertRaises(ValueError):
            input_bridge.execute_action({"look_x": 101})

    def test_rejects_non_boolean_buttons(self):
        with self.assertRaises(ValueError):
            input_bridge.execute_action({"jump": 1})

    @mock.patch("input_bridge._windows_pulse")
    @mock.patch("input_bridge.require_game_foreground", return_value="Quake III Arena")
    @mock.patch("input_bridge.platform.system", return_value="Windows")
    def test_valid_action_is_sent_as_bounded_pulse(self, _system, _focus, pulse):
        result = input_bridge.execute_action({
            "move": "forward", "strafe": "left", "jump": True,
            "attack": False, "look_x": 8, "look_y": -4, "duration_ms": 100
        })
        pulse.assert_called_once_with(["w", "a", "space"], False, 8, -4, 100)
        self.assertTrue(result["executed"])
        self.assertEqual(result["backend"], "windows-user32")

    @mock.patch.dict(os.environ, {"WAYLAND_DISPLAY": "wayland-0"}, clear=False)
    @mock.patch("input_bridge.platform.system", return_value="Linux")
    def test_wayland_requires_explicit_focus_opt_in(self, _system):
        with mock.patch.dict(os.environ, {"Q3_AGENT_ALLOW_UNVERIFIED_WAYLAND_FOCUS": "0"}):
            with self.assertRaisesRegex(RuntimeError, "Wayland cannot expose"):
                input_bridge.require_game_foreground()

    @mock.patch.dict(os.environ, {"WAYLAND_DISPLAY": "wayland-0", "Q3_AGENT_ALLOW_UNVERIFIED_WAYLAND_FOCUS": "1"}, clear=False)
    @mock.patch("input_bridge.platform.system", return_value="Linux")
    def test_wayland_opt_in_is_explicit(self, _system):
        self.assertIn("not verifiable", input_bridge.require_game_foreground())

    @mock.patch("input_bridge.foreground_window", return_value="Firefox")
    def test_requires_quake_window_focus(self, _focus):
        with self.assertRaises(RuntimeError):
            input_bridge.require_game_foreground()


if __name__ == "__main__":
    unittest.main()
