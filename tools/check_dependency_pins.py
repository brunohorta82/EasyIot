#!/usr/bin/env python3
"""Fail when a direct PlatformIO library dependency can move implicitly."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
PLATFORMIO_INI = ROOT / "platformio.ini"
FULL_GIT_SHA = re.compile(r"[0-9a-f]{40}")
EXACT_VERSION = re.compile(r"[0-9]+(?:\.[0-9]+)*(?:[-+][0-9A-Za-z.-]+)?")


def is_url_dependency(spec: str) -> bool:
    """Recognize VCS/URL dependencies with or without a conventional .git suffix."""

    target = spec.split("=", 1)[-1].strip()
    return "://" in target or target.startswith(("git@", "git+", "ssh:"))


def dependency_specs(platformio_ini: Path) -> list[tuple[int, str]]:
    """Return declared lib_deps entries, excluding inherited substitutions."""

    specs: list[tuple[int, str]] = []
    in_dependencies = False
    for line_number, raw_line in enumerate(
        platformio_ini.read_text(encoding="utf-8").splitlines(), start=1
    ):
        stripped = raw_line.strip()
        if not stripped or stripped.startswith(";"):
            continue
        if re.match(r"^lib_deps\s*=", stripped):
            in_dependencies = True
            remainder = stripped.split("=", 1)[1].strip()
            if remainder and not remainder.startswith("${"):
                specs.append((line_number, remainder))
            continue
        if in_dependencies and raw_line[:1].isspace():
            if not stripped.startswith("${"):
                specs.append((line_number, stripped))
            continue
        in_dependencies = False
    return specs


def pin_errors(platformio_ini: Path = PLATFORMIO_INI) -> list[str]:
    errors: list[str] = []
    for line_number, spec in dependency_specs(platformio_ini):
        if ".git" in spec or is_url_dependency(spec):
            fragment = spec.rsplit("#", 1)[1] if "#" in spec else ""
            if not FULL_GIT_SHA.fullmatch(fragment):
                errors.append(
                    f"line {line_number}: Git dependency must use a full 40-character "
                    f"commit SHA: {spec}"
                )
            continue

        if "@" not in spec:
            errors.append(
                f"line {line_number}: registry dependency has no exact version: {spec}"
            )
            continue
        version = spec.rsplit("@", 1)[1].strip()
        if not EXACT_VERSION.fullmatch(version):
            errors.append(
                f"line {line_number}: registry dependency must use an exact version: {spec}"
            )
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "platformio_ini",
        nargs="?",
        type=Path,
        default=PLATFORMIO_INI,
        help="PlatformIO configuration to check",
    )
    return parser.parse_args()


def main() -> int:
    path = parse_args().platformio_ini
    try:
        errors = pin_errors(path)
    except OSError as error:
        print(f"Dependency pin check failed: {error}", file=sys.stderr)
        return 2
    if errors:
        print("Dependency pin check failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    count = len(dependency_specs(path))
    print(f"[OK] {count} direct library declarations are immutable.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
