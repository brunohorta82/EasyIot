#!/usr/bin/env python3
"""
Cross-platform release metadata validator.

Matches the existing validate_release.sh behavior while avoiding shell/awk
dependencies so it works on Windows/macOS/Linux.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Iterable, List


ROOT = Path(__file__).resolve().parent.parent
PLATFORMIO_INI = ROOT / "platformio.ini"
CHANGELOG = ROOT / "CHANGELOG.md"
CONSTANTS = ROOT / "include" / "Constants.h"


def _strip_inline_comment(line: str) -> str:
    # INI uses ';' for comments; keep '#' for script compatibility.
    for mark in (";", "#"):
        idx = line.find(mark)
        if idx != -1:
            line = line[:idx]
    return line.rstrip()


def _read_lines(path: Path) -> List[str]:
    return path.read_text(encoding="utf-8").splitlines()


def _read_extra_version(lines: Iterable[str]) -> str:
    in_extra = False
    for raw in lines:
        line = raw.strip()
        if line == "[extra]":
            in_extra = True
            continue
        if line.startswith("["):
            in_extra = False
        if not in_extra:
            continue

        clean = _strip_inline_comment(raw).strip()
        match = re.match(r"^version\s*=\s*(.+)$", clean)
        if match:
            return match.group(1).strip()
    return ""


def _has_section(lines: Iterable[str], section_name: str) -> bool:
    needle = f"[{section_name}]"
    return any(raw.strip() == needle for raw in lines)


def _env_sections(lines: Iterable[str]) -> List[str]:
    out: List[str] = []
    for raw in lines:
        m = re.match(r"^\[(env:[^\]]+)\]\s*$", raw.strip())
        if m:
            out.append(m.group(1))
    return out


def _section_has_unflag_macro(lines: Iterable[str], env_name: str, macro_name: str) -> bool:
    section = f"[{env_name}]"
    in_section = False
    in_unflags = False

    for raw in lines:
        stripped = raw.strip()
        if stripped == section:
            in_section = True
            in_unflags = False
            continue

        if in_section and stripped.startswith("["):
            break
        if not in_section:
            continue

        clean = _strip_inline_comment(raw)
        if not clean.strip():
            continue

        key_match = re.match(r"^\s*([A-Za-z0-9_.-]+)\s*=\s*(.*)$", clean)
        if key_match:
            key = key_match.group(1)
            val = key_match.group(2).strip()
            in_unflags = key == "build_unflags"
            if in_unflags and val:
                tokens = val.split()
                for idx, token in enumerate(tokens):
                    if token == "-D" and idx + 1 < len(tokens) and tokens[idx + 1] == macro_name:
                        return True
                    if token.startswith("-D") and token[2:] == macro_name:
                        return True
            continue

        if in_unflags:
            tokens = clean.strip().split()
            for idx, token in enumerate(tokens):
                if token == "-D" and idx + 1 < len(tokens) and tokens[idx + 1] == macro_name:
                    return True
                if token.startswith("-D") and token[2:] == macro_name:
                    return True

    return False


def _find_constant_url(text: str, name: str) -> str:
    m = re.search(rf"{re.escape(name)}\{{\"([^\"]+)\"\}}", text)
    return m.group(1) if m else ""


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate release/version metadata before publishing firmware.",
        add_help=True,
    )
    parser.add_argument("--version", "-v", default="", help="Override version value")
    parser.add_argument("--env", "-e", default="", help="Validate a single environment (e.g. ESP8266_RELEASE)")
    parser.add_argument("--release", "-r", action="store_true", help="Strict mode for release builds")
    parser.add_argument("--fail-on-http", action="store_true", help="Fail if CloudIO URLs use http://")
    args = parser.parse_args()

    if not PLATFORMIO_INI.exists():
        print(f"[FAIL] Missing {PLATFORMIO_INI}")
        print("\nRelease metadata validation failed.\n")
        return 1

    lines = _read_lines(PLATFORMIO_INI)
    version = args.version.strip() or _read_extra_version(lines)

    failures = 0
    warnings = 0

    def ok(msg: str) -> None:
        print(f"[OK] {msg}")

    def warn(msg: str) -> None:
        nonlocal warnings
        print(f"\n[WARN] {msg}\n")
        warnings += 1

    def fail(msg: str) -> None:
        nonlocal failures
        print(f"[FAIL] {msg}")
        failures += 1

    if version:
        ok(f"Version detected: {version}")
    else:
        fail("Could not read version from platformio.ini [extra] section.")

    if version:
        if re.match(r"^[0-9]+(\.[0-9]+){1,2}(-[A-Za-z0-9._-]+)?$", version):
            ok("Version format looks valid.")
        else:
            fail(f"Version format is invalid: {version}")

        if args.release and "-dev" in version:
            fail(f"Strict release mode requires non-dev version (found: {version}).")

    if version:
        if CHANGELOG.exists():
            changelog_text = CHANGELOG.read_text(encoding="utf-8", errors="ignore")
            if f"## [{version}]" in changelog_text:
                ok(f"CHANGELOG.md contains entry for version {version}.")
            else:
                fail(f"CHANGELOG.md is missing section header: ## [{version}]")
        else:
            fail(f"CHANGELOG.md is missing section header: ## [{version}]")

    required_envs = ("env:ESP8266_RELEASE", "env:ESP32_RELEASE")
    for env_name in required_envs:
        if _has_section(lines, env_name):
            ok(f"Found required PlatformIO environment: {env_name}")
        else:
            fail(f"Missing required PlatformIO environment: {env_name}")

    if args.env:
        target_env = args.env if args.env.startswith("env:") else f"env:{args.env}"
        if _has_section(lines, target_env):
            ok(f"Scoped validation to environment: {target_env}")
            env_list = [target_env]
        else:
            fail(f"Requested environment not found in platformio.ini: {target_env}")
            env_list = []
    else:
        env_list = _env_sections(lines)
        if env_list:
            ok("No --env provided; validating all PlatformIO environments.")

    env_count = 0
    for env_name in env_list:
        if not env_name:
            continue
        env_count += 1
        env_upper = env_name.upper()
        if "DEBUG" in env_upper:
            ok(f"Skipping WEB_SECURE_ON enforcement for debug environment: {env_name}")
            continue

        if _section_has_unflag_macro(lines, env_name, "WEB_SECURE_ON"):
            if "TEST" in env_upper:
                warn(f"Test environment {env_name} disables WEB_SECURE_ON.")
            elif "RELEASE" in env_upper:
                if args.release:
                    fail(f"Release environment {env_name} disables WEB_SECURE_ON via build_unflags.")
                else:
                    warn(f"Release environment {env_name} disables WEB_SECURE_ON via build_unflags.")
            else:
                warn(f"Non-debug environment {env_name} disables WEB_SECURE_ON via build_unflags.")
        else:
            ok(f"Environment {env_name} keeps WEB_SECURE_ON enabled.")

    if env_count == 0:
        fail("No [env:*] sections found in platformio.ini")
    else:
        ok(f"Detected {env_count} PlatformIO environment(s).")

    if not CONSTANTS.exists():
        fail(f"Missing {CONSTANTS}")
    else:
        text = CONSTANTS.read_text(encoding="utf-8", errors="ignore")
        config_url = _find_constant_url(text, "configUrl")
        ota_url = _find_constant_url(text, "otaUrl")

        if config_url:
            ok(f"Found CloudIO config URL: {config_url}")
            if config_url.startswith("http://"):
                if args.fail_on_http:
                    fail(f"CloudIO config URL uses http:// ({config_url})")
                else:
                    warn(f"CloudIO config URL uses http:// ({config_url})")
        else:
            fail(f"Could not find CloudIO config URL in {CONSTANTS}")

        if ota_url:
            ok(f"Found OTA URL: {ota_url}")
            if ota_url.startswith("http://"):
                if args.fail_on_http:
                    fail(f"OTA URL uses http:// ({ota_url})")
                else:
                    warn(f"OTA URL uses http:// ({ota_url})")
        else:
            fail(f"Could not find OTA URL in {CONSTANTS}")

    print("\n==============================================")
    print(f"Validation summary: {failures} failure(s), {warnings} warning(s).")
    print("==============================================\n")

    if failures > 0:
        print("\nRelease metadata validation failed.\n")
        return 1

    print("\nRelease metadata validation passed.\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())

