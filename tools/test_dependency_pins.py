#!/usr/bin/env python3
"""Host-only tests for immutable PlatformIO dependency declarations."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("check_dependency_pins.py")
SPEC = importlib.util.spec_from_file_location("check_dependency_pins", SCRIPT)
assert SPEC and SPEC.loader
check_dependency_pins = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_dependency_pins)


class DependencyPinTests(unittest.TestCase):
    def check(self, body: str) -> list[str]:
        with tempfile.TemporaryDirectory() as temporary:
            ini = Path(temporary) / "platformio.ini"
            ini.write_text(body, encoding="utf-8")
            return check_dependency_pins.pin_errors(ini)

    def test_exact_registry_version_and_full_git_sha_pass(self) -> None:
        errors = self.check(
            "[base]\nlib_deps =\n"
            "    owner/Registry@1.2.3\n"
            "    Alias=https://github.com/example/project.git#"
            "0123456789abcdef0123456789abcdef01234567\n"
        )
        self.assertEqual(errors, [])

    def test_git_branch_tag_and_missing_fragment_fail(self) -> None:
        errors = self.check(
            "[base]\nlib_deps =\n"
            "    A=https://github.com/example/a.git\n"
            "    B=https://github.com/example/b.git#main\n"
            "    C=https://github.com/example/c.git#v1.2.3\n"
        )
        self.assertEqual(len(errors), 3)
        self.assertTrue(all("40-character commit SHA" in error for error in errors))

    def test_git_url_without_dot_git_still_requires_full_sha(self) -> None:
        errors = self.check(
            "[base]\nlib_deps =\n"
            "    Alias=https://github.com/example/project#main\n"
        )
        self.assertEqual(len(errors), 1)
        self.assertIn("40-character commit SHA", errors[0])

    def test_registry_ranges_and_unversioned_packages_fail(self) -> None:
        errors = self.check(
            "[base]\nlib_deps =\n"
            "    owner/A@^1.0.0\n"
            "    owner/B@~2.0.0\n"
            "    owner/C\n"
            "    owner/D@latest\n"
            "    owner/E@main\n"
        )
        self.assertEqual(len(errors), 5)
        self.assertIn("exact version", errors[0])
        self.assertIn("no exact version", errors[2])

    def test_inherited_lists_and_comments_are_ignored(self) -> None:
        errors = self.check(
            "[extra]\nlib_deps =\n"
            "    ; moving.git#main in a comment\n"
            "    owner/A@1.0.0\n"
            "[env:test]\nlib_deps = ${extra.lib_deps}\n"
            "    owner/B@2.0.0\n"
        )
        self.assertEqual(errors, [])

    def test_repository_platformio_dependencies_are_immutable(self) -> None:
        self.assertEqual(check_dependency_pins.pin_errors(), [])

    def test_platformio_prebuild_hook_runs_pin_check_before_web_generation(self) -> None:
        source = Path(__file__).with_name("extra_script.py").read_text(encoding="utf-8")
        pin_call = source.index("run_dependency_validation(None, None, env)")
        web_call = source.index("run_html_converter(None, None, env)")
        self.assertLess(pin_call, web_call)


if __name__ == "__main__":
    unittest.main(verbosity=2)
