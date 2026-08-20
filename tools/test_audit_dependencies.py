#!/usr/bin/env python3
"""Host-only tests for the PlatformIO dependency audit runner."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).with_name("audit_dependencies.py")
SPEC = importlib.util.spec_from_file_location("audit_dependencies", SCRIPT)
assert SPEC and SPEC.loader
audit_dependencies = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit_dependencies)


class DependencyAuditTests(unittest.TestCase):
    def test_environment_parser_preserves_platformio_order(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            ini = Path(temporary) / "platformio.ini"
            ini.write_text(
                "[base]\nboard = esp12e\n\n"
                "[env:ESP8266_DEBUG]\nextends = base\n\n"
                " [env:ESP32_DEBUG] \nextends = base\n",
                encoding="utf-8",
            )
            self.assertEqual(
                audit_dependencies.configured_environments(ini),
                ["ESP8266_DEBUG", "ESP32_DEBUG"],
            )

    def test_all_environments_are_selected_by_default(self) -> None:
        configured = ["ESP8266_DEBUG", "ESP32_DEBUG"]
        self.assertEqual(
            audit_dependencies.selected_environments([], configured), configured
        )

    def test_requested_environments_are_deduplicated(self) -> None:
        self.assertEqual(
            audit_dependencies.selected_environments(
                ["ESP32_DEBUG", "ESP32_DEBUG"],
                ["ESP8266_DEBUG", "ESP32_DEBUG"],
            ),
            ["ESP32_DEBUG"],
        )

    def test_unknown_environment_is_rejected(self) -> None:
        with self.assertRaisesRegex(RuntimeError, "unknown PlatformIO environment"):
            audit_dependencies.selected_environments(
                ["NOT_REAL"], ["ESP8266_DEBUG"]
            )

    @mock.patch.object(audit_dependencies.subprocess, "run")
    def test_audit_uses_outdated_command(self, run: mock.Mock) -> None:
        run.return_value = mock.Mock(returncode=0, stdout="Packages are up-to-date")
        passed, output = audit_dependencies.audit_environment(
            "/usr/bin/pio", "ESP8266_DEBUG"
        )
        self.assertTrue(passed)
        self.assertEqual(output, "Packages are up-to-date")
        self.assertEqual(
            run.call_args.args[0],
            ["/usr/bin/pio", "pkg", "outdated", "-e", "ESP8266_DEBUG"],
        )

    @mock.patch.object(audit_dependencies.subprocess, "run")
    def test_failed_environment_is_reported(self, run: mock.Mock) -> None:
        run.return_value = mock.Mock(returncode=1, stdout="platform setup failed\n")
        passed, output = audit_dependencies.audit_environment(
            "/usr/bin/pio", "ESP32C6_IRRIGATION"
        )
        self.assertFalse(passed)
        self.assertEqual(output, "platform setup failed")


if __name__ == "__main__":
    unittest.main(verbosity=2)
