#!/usr/bin/env python3
"""Host-only tests for the EasyIot project check runner."""

from __future__ import annotations

import importlib.util
import io
from pathlib import Path
import tempfile
import unittest
from contextlib import redirect_stdout


SCRIPT = Path(__file__).with_name("check_project.py")
SPEC = importlib.util.spec_from_file_location("check_project", SCRIPT)
assert SPEC and SPEC.loader
check_project = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_project)


class ProjectCheckTests(unittest.TestCase):
    def test_all_builds_use_debug_matrix_for_development_version(self) -> None:
        self.assertEqual(
            check_project.all_build_environments("9.177-dev"),
            check_project.DEBUG_BUILD_SET,
        )

    def test_all_builds_use_release_matrix_for_release_version(self) -> None:
        self.assertEqual(
            check_project.all_build_environments("9.177"),
            check_project.RELEASE_BUILD_SET,
        )

    def test_project_version_reads_only_extra_section(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            ini = Path(temporary) / "platformio.ini"
            ini.write_text(
                "[other]\nversion = wrong\n\n"
                "[extra]\nversion = 9.177-dev ; local development\n",
                encoding="utf-8",
            )
            self.assertEqual(check_project.project_version(ini), "9.177-dev")

    def test_platformio_environment_parser(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            ini = Path(temporary) / "platformio.ini"
            ini.write_text(
                "[ESP8266]\nboard = esp12e\n\n"
                "[env:ESP8266_DEBUG]\nextends = ESP8266\n\n"
                " [env:ESP32_DEBUG] \nextends = ESP32\n",
                encoding="utf-8",
            )
            self.assertEqual(
                check_project.platformio_environments(ini),
                {"ESP8266_DEBUG", "ESP32_DEBUG"},
            )

    def test_warning_step_is_reported_without_failing(self) -> None:
        def warn() -> None:
            raise check_project.CheckWarning("expected warning")

        output = io.StringIO()
        with redirect_stdout(output):
            result = check_project.run_step("Warning check", warn)

        self.assertEqual(result, "warning")
        self.assertIn("[WARNING] Warning check: expected warning", output.getvalue())


if __name__ == "__main__":
    unittest.main(verbosity=2)
