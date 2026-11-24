#!/usr/bin/env python3
"""Refuse an OTA application image that does not fit the slot it will be written to.

The size PlatformIO prints after a build ("Flash: 94.1% used") is the sum of the
ELF's loadable sections, not the image a device downloads. The .bin also carries
.eh_frame, segment headers, padding and the SHA-256 footer, so the two can differ
by more than 100 KB. ESP32C6_IRRIGATION 9.233 and 9.234 reported 94% and still
came out 688 bytes larger than their OTA slot: every irrigation controller asked
to update was handed an image it could not even begin to write.

This check reads the partition table the env actually uses and compares it with
the real .bin. Environments without a partition table (ESP8266) are skipped.
"""

from __future__ import annotations

import argparse
import configparser
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# Below this much headroom a release still passes, but it says so: 9.232 shipped
# with 992 bytes to spare and the next release no longer fitted.
WARN_HEADROOM = 32 * 1024


class SizeError(Exception):
    pass


def parse_size(text: str) -> int:
    value = text.strip().upper()
    if not value:
        raise SizeError("empty partition size")
    multiplier = 1
    if value.endswith("K"):
        multiplier, value = 1024, value[:-1]
    elif value.endswith("M"):
        multiplier, value = 1024 * 1024, value[:-1]
    return int(value, 0) * multiplier


def ota_slot_size(partition_csv: Path) -> int:
    """Size of the smallest OTA app slot; a lone factory app if there is no OTA."""
    ota, factory = [], []
    with partition_csv.open(newline="") as handle:
        for row in csv.reader(handle):
            if not row or row[0].strip().startswith("#"):
                continue
            fields = [field.strip() for field in row]
            if len(fields) < 5 or fields[1] != "app":
                continue
            size = parse_size(fields[4])
            (ota if fields[2].startswith("ota_") else factory).append(size)
    if ota:
        return min(ota)
    if factory:
        return min(factory)
    raise SizeError(f"{partition_csv.name} declares no app partition")


def env_option(ini: configparser.ConfigParser, env: str, option: str) -> str | None:
    """Look an option up the way PlatformIO does: the env, then what it extends."""
    section, seen = f"env:{env}", set()
    while section and section not in seen:
        seen.add(section)
        if not ini.has_section(section):
            raise SizeError(f"platformio.ini has no [{section}]")
        if ini.has_option(section, option):
            return ini.get(section, option).strip()
        section = ini.get(section, "extends").strip() if ini.has_option(section, "extends") else None
    return None


def application_image(build_dir: Path) -> Path:
    # The same choice the release workflow makes: the renamed application image,
    # never the merged .factory.bin beside it.
    for candidate in sorted(build_dir.glob("Firmware_*.bin")):
        if not candidate.name.endswith(".factory.bin"):
            return candidate
    fallback = build_dir / "firmware.bin"
    if fallback.is_file():
        return fallback
    raise SizeError(f"no application image in {build_dir}")


def check(env: str, image: Path | None, root: Path | None = None) -> tuple[int, int] | None:
    root = root or ROOT
    ini = configparser.ConfigParser(interpolation=None, strict=False)
    ini.read(root / "platformio.ini")
    partitions = env_option(ini, env, "board_build.partitions")
    if not partitions:
        return None
    slot = ota_slot_size(root / partitions)
    image = image or application_image(root / ".pio" / "build" / env)
    return image.stat().st_size, slot


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--env", required=True)
    parser.add_argument("--bin", type=Path, help="application image (default: the env's build output)")
    args = parser.parse_args(argv)
    try:
        result = check(args.env, args.bin)
    except (SizeError, OSError, ValueError) as error:
        print(f"::error::{args.env}: {error}")
        return 1
    if result is None:
        print(f"{args.env}: no partition table declared, nothing to check")
        return 0
    size, slot = result
    headroom = slot - size
    print(f"{args.env}: image {size} bytes, OTA slot {slot} bytes, headroom {headroom} bytes")
    if headroom < 0:
        print(f"::error::{args.env}: the image is {-headroom} bytes larger than its OTA slot; "
              "devices would refuse the update")
        return 1
    if headroom < WARN_HEADROOM:
        print(f"::warning::{args.env}: only {headroom} bytes left in the OTA slot")
    return 0


if __name__ == "__main__":
    sys.exit(main())
