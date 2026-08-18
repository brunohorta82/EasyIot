#!/usr/bin/env python3
"""Host-only tests for the local EasyIot firmware export workflow."""

from __future__ import annotations

from datetime import date, timedelta
import importlib.util
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("export_firmware.py")
SPEC = importlib.util.spec_from_file_location("export_firmware", SCRIPT)
assert SPEC and SPEC.loader
export_firmware = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(export_firmware)


class FirmwareExportTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        (self.root / "platformio.ini").write_text(
            "[extra]\nversion = 9.177-dev\n\n"
            "[env:ESP8266_DEBUG]\n\n"
            "[env:ESP8266_RELEASE]\n\n"
            "[env:ESP32_DEBUG]\n\n"
            "[env:ESP32_TEST]\n",
            encoding="utf-8",
        )
        self.binary = self.root / "input.bin"
        self.binary.write_bytes(b"easyiot-firmware-one")
        self.output = self.root / "firmware_bins"

    def tearDown(self) -> None:
        self.temp.cleanup()

    def publish(self, **overrides: object) -> Path:
        values: dict[str, object] = {
            "environment": "ESP8266_DEBUG",
            "channel": None,
            "explicit_binary": self.binary,
            "output_root": self.output,
            "date_stamp": "2026-08-18",
            "project_root": self.root,
        }
        values.update(overrides)
        return export_firmware.publish(**values)

    def test_publish_creates_verified_environment_specific_candidate(self) -> None:
        candidate = self.publish()
        info, digest = export_firmware.verify_export(
            candidate, expected_channel="debug", expected_env="ESP8266_DEBUG"
        )
        self.assertEqual(info["Version"], "9.177-dev")
        self.assertEqual(digest, export_firmware.sha256(self.binary))
        self.assertEqual(len(list(candidate.glob("*.bin"))), 1)

    def test_identical_publish_is_idempotent(self) -> None:
        candidate = self.publish()
        before = (candidate / "BUILD_INFO.txt").read_bytes()
        repeated = self.publish()
        self.assertEqual(candidate, repeated)
        self.assertEqual((candidate / "BUILD_INFO.txt").read_bytes(), before)

    def test_release_environment_uses_a_separate_release_candidate(self) -> None:
        candidate = self.publish(environment="ESP8266_RELEASE")
        info, _ = export_firmware.verify_export(
            candidate,
            expected_channel="release",
            expected_env="ESP8266_RELEASE",
        )
        self.assertEqual(
            candidate.resolve(),
            (self.output / "candidate" / "release" / "ESP8266_RELEASE").resolve(),
        )
        self.assertEqual(info["Channel"], "release")

    def test_publish_replaces_only_an_owned_candidate(self) -> None:
        candidate = self.publish()
        self.binary.write_bytes(b"easyiot-firmware-two")
        self.publish()
        _, digest = export_firmware.verify_export(candidate)
        self.assertEqual(digest, export_firmware.sha256(self.binary))

        unowned = self.output / "candidate" / "debug" / "ESP32_DEBUG"
        unowned.mkdir(parents=True)
        (unowned / "keep.txt").write_text("user data\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "unowned"):
            self.publish(environment="ESP32_DEBUG")
        self.assertTrue((unowned / "keep.txt").is_file())

    def test_tampered_binary_fails_verification(self) -> None:
        candidate = self.publish()
        next(candidate.glob("*.bin")).write_bytes(b"tampered")
        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            export_firmware.verify_export(candidate)

    def test_ambiguous_automatic_binary_selection_fails_closed(self) -> None:
        build = self.root / ".pio" / "build" / "ESP8266_DEBUG"
        build.mkdir(parents=True)
        for stamp in ("17.08.2026", "18.08.2026"):
            (build / f"Firmware_ESP8266_DEBUG_9.177-dev - {stamp}.bin").write_bytes(
                stamp.encode("ascii")
            )
        with self.assertRaisesRegex(ValueError, "multiple matching binaries"):
            export_firmware.locate_binary(
                "ESP8266_DEBUG", "9.177-dev", None, project_root=self.root
            )

    def test_promote_preserves_candidate_and_refuses_changed_identity(self) -> None:
        candidate = self.publish()
        known_good_root = self.output / "known-good"
        promoted = export_firmware.promote(
            candidate_dir=candidate,
            known_good_root=known_good_root,
            hardware_tested_date="2026-08-18",
        )
        self.assertTrue(candidate.is_dir())
        self.assertTrue((promoted / "RESTORE.txt").is_file())
        repeated = export_firmware.promote(
            candidate_dir=candidate,
            known_good_root=known_good_root,
            hardware_tested_date="2026-08-18",
        )
        self.assertEqual(promoted, repeated)

        self.binary.write_bytes(b"different-bytes-same-version")
        changed_candidate = self.publish()
        with self.assertRaisesRegex(ValueError, "already exists"):
            export_firmware.promote(
                candidate_dir=changed_candidate,
                known_good_root=known_good_root,
                hardware_tested_date="2026-08-18",
            )

    def test_promote_rejects_invalid_test_dates(self) -> None:
        candidate = self.publish()
        with self.assertRaisesRegex(ValueError, "precede"):
            export_firmware.promote(
                candidate_dir=candidate,
                known_good_root=self.output / "known-good",
                hardware_tested_date="2026-08-17",
            )
        with self.assertRaisesRegex(ValueError, "future"):
            export_firmware.promote(
                candidate_dir=candidate,
                known_good_root=self.output / "known-good",
                hardware_tested_date=(date.today() + timedelta(days=1)).isoformat(),
            )

    def test_nonstandard_environment_requires_explicit_channel(self) -> None:
        with self.assertRaisesRegex(ValueError, "use --channel"):
            export_firmware.infer_channel("ESP32_TEST", None)
        self.assertEqual(
            export_firmware.infer_channel("ESP32_TEST", "debug"), "debug"
        )

    def test_publish_rejects_unknown_environment_and_channel_contradiction(self) -> None:
        with self.assertRaisesRegex(ValueError, "unknown PlatformIO environment"):
            self.publish(environment="ESP8266_TYP0")
        with self.assertRaisesRegex(ValueError, "implies 'debug'"):
            self.publish(channel="release")

    def test_explicit_generated_binary_must_match_environment(self) -> None:
        wrong = self.root / "Firmware_ESP32_DEBUG_9.177-dev - 18.08.2026.bin"
        wrong.write_bytes(b"esp32")
        with self.assertRaisesRegex(ValueError, "does not match environment"):
            export_firmware.locate_binary(
                "ESP8266_DEBUG", "9.177-dev", wrong, project_root=self.root
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
