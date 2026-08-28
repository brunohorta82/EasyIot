#!/usr/bin/env python3
"""Enforce evidence-based ESP8266 static-RAM budgets from a linked ELF."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUDGET_FILE = Path(__file__).with_name("esp8266_ram_budgets.json")
DEFAULT_BUILD_ROOT = ROOT / ".pio" / "build"
EXPECTED_ENVIRONMENTS = {
    "ESP8266_RELEASE",
    "ESP8266-HAN_RELEASE",
    "ESP8266_DEBUG",
    "ESP8266-HAN_DEBUG",
}


class BudgetError(RuntimeError):
    """A configuration, tool, or ELF measurement error."""


def positive_integer(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise BudgetError(f"{field} must be a positive integer")
    return value


def load_budgets(
    path: Path = DEFAULT_BUDGET_FILE,
) -> tuple[str, tuple[str, ...], dict[str, dict[str, int]]]:
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except OSError as error:
        raise BudgetError(f"cannot read budget file {path}: {error}") from error
    except json.JSONDecodeError as error:
        raise BudgetError(f"invalid JSON in {path}: {error}") from error

    if not isinstance(document, dict) or document.get("schema_version") != 1:
        raise BudgetError("budget file must use schema_version 1")

    baseline_version = document.get("baseline_version")
    if not isinstance(baseline_version, str) or not re.fullmatch(
        r"[0-9]+(?:\.[0-9]+){1,2}", baseline_version
    ):
        raise BudgetError("baseline_version must be a release version such as 9.200")

    raw_sections = document.get("ram_sections")
    if (
        not isinstance(raw_sections, list)
        or not raw_sections
        or any(not isinstance(section, str) or not section.startswith(".") for section in raw_sections)
        or len(raw_sections) != len(set(raw_sections))
    ):
        raise BudgetError("ram_sections must be a non-empty list of unique ELF section names")

    raw_environments = document.get("environments")
    if not isinstance(raw_environments, dict):
        raise BudgetError("environments must be an object")
    missing = EXPECTED_ENVIRONMENTS - set(raw_environments)
    extra = set(raw_environments) - EXPECTED_ENVIRONMENTS
    if missing or extra:
        details = []
        if missing:
            details.append("missing " + ", ".join(sorted(missing)))
        if extra:
            details.append("unexpected " + ", ".join(sorted(extra)))
        raise BudgetError("budget environments do not match the protected set: " + "; ".join(details))

    environments: dict[str, dict[str, int]] = {}
    for name, raw_budget in raw_environments.items():
        if not isinstance(raw_budget, dict):
            raise BudgetError(f"{name} budget must be an object")
        baseline = positive_integer(raw_budget.get("baseline_bytes"), f"{name}.baseline_bytes")
        limit = positive_integer(raw_budget.get("limit_bytes"), f"{name}.limit_bytes")
        if baseline > limit:
            raise BudgetError(f"{name}.baseline_bytes cannot exceed limit_bytes")
        environments[name] = {"baseline_bytes": baseline, "limit_bytes": limit}

    return baseline_version, tuple(raw_sections), environments


def parse_section_sizes(output: str, required_sections: tuple[str, ...]) -> dict[str, int]:
    found: dict[str, int] = {}
    for line in output.splitlines():
        match = re.fullmatch(r"\s*(\.\S+)\s+(\d+)\s+(?:0x[0-9A-Fa-f]+|\d+)\s*", line)
        if not match or match.group(1) not in required_sections:
            continue
        section = match.group(1)
        if section in found:
            raise BudgetError(f"ELF size output contains duplicate {section} sections")
        found[section] = int(match.group(2))
    missing = set(required_sections) - set(found)
    if missing:
        raise BudgetError("ELF size output is missing required sections: " + ", ".join(sorted(missing)))
    return found


def locate_size_tool(explicit: str | None = None) -> Path:
    if explicit:
        path = Path(explicit).expanduser()
        if path.is_file():
            return path
        resolved = shutil.which(explicit)
        if resolved:
            return Path(resolved)
        raise BudgetError(f"ESP8266 size tool does not exist: {explicit}")

    resolved = shutil.which("xtensa-lx106-elf-size")
    if resolved:
        return Path(resolved)

    core_dir = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio"))
    executable = "xtensa-lx106-elf-size.exe" if os.name == "nt" else "xtensa-lx106-elf-size"
    candidate = core_dir / "packages" / "toolchain-xtensa" / "bin" / executable
    if candidate.is_file():
        return candidate
    raise BudgetError("cannot find xtensa-lx106-elf-size; build/install the ESP8266 toolchain first")


def locate_elf(environment: str, explicit: Path | None = None) -> Path:
    if explicit is not None:
        if not explicit.is_file():
            raise BudgetError(f"ELF does not exist: {explicit}")
        return explicit
    build_dir = DEFAULT_BUILD_ROOT / environment
    candidates = sorted(build_dir.glob("Firmware_*.elf"))
    if len(candidates) != 1:
        raise BudgetError(
            f"expected exactly one Firmware_*.elf in {build_dir}, found {len(candidates)}; "
            "build the environment first or pass --elf"
        )
    return candidates[0]


def measure_sections(elf: Path, size_tool: Path, sections: tuple[str, ...]) -> dict[str, int]:
    try:
        result = subprocess.run(
            [str(size_tool), "-A", str(elf)],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except OSError as error:
        raise BudgetError(f"could not run {size_tool}: {error}") from error
    if result.returncode != 0:
        raise BudgetError(f"size tool exited with {result.returncode}: {result.stdout.strip()}")
    return parse_section_sizes(result.stdout, sections)


def budget_report(
    environment: str,
    section_sizes: dict[str, int],
    budget: dict[str, int],
    baseline_version: str,
) -> tuple[bool, str]:
    used = sum(section_sizes.values())
    baseline = budget["baseline_bytes"]
    limit = budget["limit_bytes"]
    margin = limit - used
    delta = used - baseline
    breakdown = " + ".join(f"{name} {section_sizes[name]:,}" for name in section_sizes)
    status = "OK" if margin >= 0 else "FAIL"
    message = (
        f"[{status}] {environment} static RAM: {used:,} / {limit:,} bytes "
        f"({margin:+,} margin, {delta:+,} vs v{baseline_version} baseline); {breakdown}"
    )
    if margin < 0:
        message += (
            ". Do not raise the limit to make the build green: inspect the ELF change and repeat "
            "real-board HTTPS OTA validation before approving a new budget."
        )
    return margin >= 0, message


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--env", dest="environment", help="protected PlatformIO environment")
    parser.add_argument("--elf", type=Path, help="linked ELF; defaults to .pio/build/<env>/Firmware_*.elf")
    parser.add_argument("--size-tool", help="path/name of xtensa-lx106-elf-size")
    parser.add_argument("--budget-file", type=Path, default=DEFAULT_BUDGET_FILE)
    parser.add_argument("--validate-config", action="store_true", help="validate budgets without reading an ELF")
    args = parser.parse_args(argv)
    if not args.validate_config and not args.environment:
        parser.error("--env is required unless --validate-config is used")
    if args.validate_config and (args.environment or args.elf or args.size_tool):
        parser.error("--validate-config cannot be combined with --env, --elf, or --size-tool")
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        baseline_version, sections, environments = load_budgets(args.budget_file)
        if args.validate_config:
            print(
                f"[OK] ESP8266 RAM budgets cover {len(environments)} environments from "
                f"v{baseline_version}; sections: {', '.join(sections)}"
            )
            return 0
        if args.environment not in environments:
            raise BudgetError(f"no static-RAM budget for environment {args.environment}")
        elf = locate_elf(args.environment, args.elf)
        size_tool = locate_size_tool(args.size_tool)
        measured = measure_sections(elf, size_tool, sections)
        passed, message = budget_report(
            args.environment, measured, environments[args.environment], baseline_version
        )
        print(message)
        return 0 if passed else 1
    except BudgetError as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
