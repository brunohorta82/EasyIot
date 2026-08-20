#!/usr/bin/env python3
"""Report outdated PlatformIO packages without changing project declarations."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
PLATFORMIO_INI = ROOT / "platformio.ini"


def configured_environments(platformio_ini: Path = PLATFORMIO_INI) -> list[str]:
    environments: list[str] = []
    for raw_line in platformio_ini.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if line.startswith("[env:") and line.endswith("]"):
            environments.append(line[5:-1].strip())
    if not environments:
        raise RuntimeError(f"no PlatformIO environments found in {platformio_ini}")
    return environments


def platformio_command() -> str:
    for name in ("platformio", "pio"):
        command = shutil.which(name)
        if command:
            return command
    raise RuntimeError("PlatformIO is not installed or not on PATH")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--env",
        action="append",
        default=[],
        metavar="ENV",
        help="audit one environment; may be repeated (default: every configured environment)",
    )
    return parser.parse_args()


def selected_environments(requested: list[str], configured: list[str]) -> list[str]:
    if not requested:
        return configured
    unknown = [environment for environment in requested if environment not in configured]
    if unknown:
        raise RuntimeError("unknown PlatformIO environment(s): " + ", ".join(unknown))
    return list(dict.fromkeys(requested))


def audit_environment(command: str, environment: str) -> tuple[bool, str]:
    try:
        result = subprocess.run(
            [command, "pkg", "outdated", "-e", environment],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except OSError as error:
        return False, f"could not run PlatformIO: {error}"
    return result.returncode == 0, (result.stdout or "").strip()


def print_notes() -> None:
    print("Compatibility notes:")
    print("- Current is the locally resolved package version.")
    print("- Wanted is the newest version allowed by the current declaration.")
    print("- Latest may contain breaking changes and is not an upgrade recommendation.")
    print("- Git dependencies may compare against a moving branch rather than a release.")
    print("- An audit does not prove compatibility; build and test every affected target.")
    print("- This tool does not edit platformio.ini or update declared dependencies.")


def main() -> int:
    try:
        configured = configured_environments()
        environments = selected_environments(parse_args().env, configured)
        command = platformio_command()
    except (OSError, RuntimeError) as error:
        print(f"Dependency audit failed: {error}", file=sys.stderr)
        return 2

    print_notes()
    print(f"\nAuditing {len(environments)} environment(s) with: {command}")

    failed: list[str] = []
    for environment in environments:
        print(f"\n--- {environment} ---", flush=True)
        passed, output = audit_environment(command, environment)
        if output:
            print(output)
        if passed:
            print(f"[PASS] {environment}")
        else:
            print(f"[FAIL] {environment}")
            failed.append(environment)

    print("\n==============================================")
    print(
        f"Dependency audit: {len(environments) - len(failed)} passed, "
        f"{len(failed)} failed"
    )
    print("==============================================")
    if failed:
        print("Could not audit: " + ", ".join(failed))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
