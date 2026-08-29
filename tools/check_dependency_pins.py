#!/usr/bin/env python3
"""Fail when a PlatformIO platform, library, or CI build tool can move implicitly."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shlex
import sys
from urllib.parse import urlsplit


ROOT = Path(__file__).resolve().parents[1]
PLATFORMIO_INI = ROOT / "platformio.ini"
RELEASE_WORKFLOW = ROOT / ".github" / "workflows" / "firmware-ota.yml"
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


def platform_specs(platformio_ini: Path) -> list[tuple[int, str]]:
    """Return concrete PlatformIO platform declarations."""

    specs: list[tuple[int, str]] = []
    for line_number, raw_line in enumerate(
        platformio_ini.read_text(encoding="utf-8").splitlines(), start=1
    ):
        stripped = raw_line.strip()
        if not stripped or stripped.startswith(";"):
            continue
        match = re.match(r"^platform\s*=\s*(.+)$", stripped)
        if not match:
            continue
        value = match.group(1).split(";", 1)[0].strip()
        if value and not value.startswith("${"):
            specs.append((line_number, value))
    return specs


def fixed_github_release_asset(url: str) -> bool:
    """Accept a versioned GitHub release asset, never latest or a branch archive."""

    parsed = urlsplit(url)
    if parsed.scheme != "https" or parsed.netloc != "github.com":
        return False
    if parsed.query or parsed.fragment:
        return False
    parts = [part for part in parsed.path.split("/") if part]
    if len(parts) < 6 or parts[2:4] != ["releases", "download"]:
        return False
    release = parts[4].lower()
    return release not in {"latest", "main", "master"} and bool(parts[5:])


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

    for line_number, spec in platform_specs(platformio_ini):
        if is_url_dependency(spec):
            fragment = spec.rsplit("#", 1)[1] if "#" in spec else ""
            if FULL_GIT_SHA.fullmatch(fragment) or fixed_github_release_asset(spec):
                continue
            errors.append(
                f"line {line_number}: platform URL must use a full 40-character "
                f"Git commit SHA or a fixed GitHub release asset: {spec}"
            )
            continue

        if "@" not in spec or not EXACT_VERSION.fullmatch(spec.rsplit("@", 1)[1]):
            errors.append(
                f"line {line_number}: registry platform must use an exact version: {spec}"
            )
    return errors


def ci_tool_errors(workflow: Path = RELEASE_WORKFLOW) -> list[str]:
    """Require exact PlatformIO Core and IntelHex versions in release CI."""

    errors: list[str] = []
    install_tokens: list[str] | None = None
    install_line = 0
    for line_number, raw_line in enumerate(
        workflow.read_text(encoding="utf-8").splitlines(), start=1
    ):
        stripped = raw_line.strip()
        run_match = re.match(r"^(?:-\s*)?run:\s*(.+)$", stripped)
        if not run_match or "pip install" not in run_match.group(1):
            continue
        tokens = shlex.split(run_match.group(1))
        if "platformio" in " ".join(tokens).lower():
            install_tokens = tokens
            install_line = line_number
            break

    if install_tokens is None:
        return ["release workflow has no PlatformIO installation command"]

    if "-U" in install_tokens or "--upgrade" in install_tokens:
        errors.append(
            f"workflow line {install_line}: release build tools must not use a floating upgrade"
        )

    for package in ("platformio", "intelhex"):
        matches = [token for token in install_tokens if token.lower().startswith(f"{package}==")]
        if len(matches) != 1:
            errors.append(
                f"workflow line {install_line}: {package} must appear once with an exact == version"
            )
            continue
        version = matches[0].split("==", 1)[1]
        if not EXACT_VERSION.fullmatch(version):
            errors.append(
                f"workflow line {install_line}: {package} has an invalid exact version: {version}"
            )
    return errors


def repository_pin_errors() -> list[str]:
    return pin_errors() + ci_tool_errors()


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
        check_repository_ci = path.resolve() == PLATFORMIO_INI.resolve()
        if check_repository_ci:
            errors.extend(ci_tool_errors())
    except OSError as error:
        print(f"Dependency pin check failed: {error}", file=sys.stderr)
        return 2
    if errors:
        print("Dependency pin check failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    library_count = len(dependency_specs(path))
    platform_count = len(platform_specs(path))
    message = (
        f"[OK] {library_count} direct library and {platform_count} platform "
        "declarations are immutable."
    )
    if check_repository_ci:
        message += " Release CI tools are pinned."
    print(message)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
