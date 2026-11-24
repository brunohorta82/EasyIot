#!/usr/bin/env python3
"""Host-only tests for the OTA slot size guard."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("check_app_size.py")
SPEC = importlib.util.spec_from_file_location("check_app_size", SCRIPT)
assert SPEC and SPEC.loader
check_app_size = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_app_size)

PARTITIONS = """# Name, Type, SubType, Offset, Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x1C0000,
app1,     app,  ota_1,   0x1D0000,0x1C0000,
spiffs,   data, spiffs,  0x390000,0x30000,
"""

INI = """[platformio]
default_envs = A

[BASE]
board_build.partitions = parts.csv

[env:A]
extends = BASE

[env:B]
extends = env:A

[env:PLAIN]
board = esp12e
"""


class AppSizeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / "parts.csv").write_text(PARTITIONS)
        (self.root / "platformio.ini").write_text(INI)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def image(self, size: int) -> Path:
        path = self.root / f"app_{size}.bin"
        path.write_bytes(b"\0" * size)
        return path

    def test_slot_is_the_ota_partition(self) -> None:
        self.assertEqual(check_app_size.ota_slot_size(self.root / "parts.csv"), 0x1C0000)

    def test_parses_suffixed_sizes(self) -> None:
        self.assertEqual(check_app_size.parse_size("1792K"), 0x1C0000)
        self.assertEqual(check_app_size.parse_size("2M"), 0x200000)

    def test_partition_table_is_found_through_extends(self) -> None:
        size, slot = check_app_size.check("B", self.image(100), self.root)
        self.assertEqual((size, slot), (100, 0x1C0000))

    def test_env_without_partition_table_is_skipped(self) -> None:
        self.assertIsNone(check_app_size.check("PLAIN", None, self.root))

    def test_rejects_the_irrigation_9_234_image(self) -> None:
        # The real numbers: 1835696 bytes against a 0x1C0000 slot.
        image = self.image(1835696)
        (self.root / "parts.csv").write_text(PARTITIONS)
        size, slot = check_app_size.check("A", image, self.root)
        self.assertEqual(slot - size, -688)

    def test_main_fails_on_overflow_and_passes_at_the_limit(self) -> None:
        original = check_app_size.ROOT
        check_app_size.ROOT = self.root
        try:
            self.assertEqual(check_app_size.main(["--env", "A", "--bin", str(self.image(0x1C0000 + 1))]), 1)
            self.assertEqual(check_app_size.main(["--env", "A", "--bin", str(self.image(0x1C0000))]), 0)
        finally:
            check_app_size.ROOT = original

    def test_application_image_never_picks_the_factory_image(self) -> None:
        build = self.root / "build"
        build.mkdir()
        (build / "Firmware_A_1.0.factory.bin").write_bytes(b"f")
        (build / "Firmware_A_1.0.bin").write_bytes(b"a")
        self.assertEqual(check_app_size.application_image(build).name, "Firmware_A_1.0.bin")


if __name__ == "__main__":
    unittest.main()
