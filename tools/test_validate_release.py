from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / "tools" / "validate_release.py"


class ValidateReleaseChangelogTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="easyiot-release-test-")
        self.root = Path(self.temporary.name)
        (self.root / "tools").mkdir()
        (self.root / "include").mkdir()
        shutil.copy2(VALIDATOR, self.root / "tools" / "validate_release.py")
        (self.root / "platformio.ini").write_text(
            """[extra]
version = 9.201

[env:ESP8266_RELEASE]
build_flags = -D WEB_SECURE_ON

[env:ESP32_RELEASE]
build_flags = -D WEB_SECURE_ON

[env:ESP8266_DEBUG]
build_flags =
""",
            encoding="utf-8",
        )
        (self.root / "include" / "Constants.h").write_text(
            """constexpr const char *configUrl{"https://example.test/config"};
constexpr const char *otaUrl{"https://example.test/update"};
""",
            encoding="utf-8",
        )

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def run_validator(self, *arguments: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(self.root / "tools" / "validate_release.py"), *arguments],
            cwd=self.root,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def write_changelog(self, text: str) -> None:
        (self.root / "CHANGELOG.md").write_text(text, encoding="utf-8")

    def test_development_accepts_one_top_unreleased_section(self) -> None:
        self.write_changelog(
            "# Changelog\n\n## [Unreleased]\n\n- Pending fix.\n\n"
            "## [9.201] - 2026-08-28\n\n- Released.\n"
        )

        result = self.run_validator("--env", "ESP8266_DEBUG")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("one top Unreleased section", result.stdout)

    def test_strict_release_rejects_unreleased_section(self) -> None:
        self.write_changelog(
            "# Changelog\n\n## [Unreleased]\n\n- Pending fix.\n\n"
            "## [9.201] - 2026-08-28\n\n- Released.\n"
        )

        result = self.run_validator("--release", "--env", "ESP8266_RELEASE")

        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("moving CHANGELOG.md [Unreleased] notes", result.stdout)
        self.assertEqual(result.stdout.count("[FAIL]"), 1, result.stdout)

    def test_strict_release_accepts_version_as_newest_section(self) -> None:
        self.write_changelog(
            "# Changelog\n\n## [9.201] - 2026-08-28\n\n- Released.\n"
        )

        result = self.run_validator("--release", "--env", "ESP8266_RELEASE")

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("newest section is the version being released", result.stdout)

    def test_development_rejects_renamed_unversioned_heading(self) -> None:
        self.write_changelog(
            "# Changelog\n\n## Development after 9.201\n\n- Pending fix.\n\n"
            "## [9.201] - 2026-08-28\n\n- Released.\n"
        )

        result = self.run_validator("--env", "ESP8266_DEBUG")

        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("development requires either", result.stdout)

    def test_development_rejects_duplicate_unreleased_sections(self) -> None:
        self.write_changelog(
            "# Changelog\n\n## [Unreleased]\n\n- First.\n\n"
            "## [9.201] - 2026-08-28\n\n- Released.\n\n"
            "## [Unreleased]\n\n- Stale.\n"
        )

        result = self.run_validator("--env", "ESP8266_DEBUG")

        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("repeats section header", result.stdout)


if __name__ == "__main__":
    unittest.main()
