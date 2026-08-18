#!/usr/bin/env python3
"""Run repeatable EasyIot source checks and optional PlatformIO builds."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from typing import Callable, Iterable


ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
GENERATED_HEADERS = ("IndexHtml.h", "IndexJs.h", "StylesMinCss.h")
DEBUG_BUILD_SET = ("ESP8266_DEBUG", "ESP8266-HAN_DEBUG", "ESP32_DEBUG")
RELEASE_BUILD_SET = ("ESP8266_RELEASE", "ESP8266-HAN_RELEASE", "ESP32_RELEASE")
TEXT_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".css",
    ".h",
    ".hpp",
    ".html",
    ".ini",
    ".js",
    ".json",
    ".md",
    ".py",
    ".sh",
    ".yml",
    ".yaml",
}
CONFLICT_MARKER = re.compile(r"^(<<<<<<<(?: .*)?|=======$|>>>>>>>(?: .*)?)$")


class CheckFailure(RuntimeError):
    """Expected validation failure with a user-facing message."""


class CheckWarning(RuntimeError):
    """Successful validation that found a condition requiring attention."""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--quick",
        action="store_true",
        help="run source checks only (the default when no build option is supplied)",
    )
    parser.add_argument(
        "--build",
        action="append",
        default=[],
        metavar="ENV",
        help="also build one PlatformIO environment; may be repeated",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="build the debug matrix for -dev versions, otherwise the release matrix",
    )
    args = parser.parse_args()
    if args.quick and (args.build or args.all):
        parser.error("--quick cannot be combined with --build or --all")
    if args.build and args.all:
        parser.error("--build and --all cannot be combined")
    return args


def run_command(
    command: list[str],
    *,
    cwd: Path = ROOT,
    show_output: bool = True,
    stream_output: bool = False,
) -> str:
    try:
        if stream_output:
            result = subprocess.run(command, cwd=cwd, check=False, text=True)
        else:
            result = subprocess.run(
                command,
                cwd=cwd,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
    except OSError as error:
        raise CheckFailure(f"could not run {command[0]}: {error}") from error
    if result.returncode != 0:
        output = (result.stdout or "").strip()
        detail = f"\n{output}" if output else ""
        raise CheckFailure(
            f"command exited with {result.returncode}: {' '.join(command)}{detail}"
        )
    if show_output and result.stdout and result.stdout.strip():
        print(result.stdout.rstrip())
    return result.stdout or ""


def require_command(name: str) -> str:
    command = shutil.which(name)
    if command is None:
        raise CheckFailure(f"required command is not installed or not on PATH: {name}")
    return command


def project_version(platformio_ini: Path = ROOT / "platformio.ini") -> str:
    section = ""
    for raw_line in platformio_ini.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].strip()
            continue
        if section == "extra" and re.match(r"^version\s*=", line):
            return line.split("=", 1)[1].split(";", 1)[0].split("#", 1)[0].strip()
    raise CheckFailure("could not read [extra] version from platformio.ini")


def platformio_environments(platformio_ini: Path = ROOT / "platformio.ini") -> set[str]:
    environments: set[str] = set()
    for raw_line in platformio_ini.read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"\s*\[env:([^]]+)]\s*", raw_line)
        if match:
            environments.add(match.group(1).strip())
    return environments


def all_build_environments(version: str) -> tuple[str, ...]:
    return DEBUG_BUILD_SET if "-dev" in version else RELEASE_BUILD_SET


def check_python_syntax() -> None:
    failures: list[str] = []
    for path in sorted(TOOLS.glob("*.py")):
        try:
            compile(path.read_bytes(), str(path), "exec")
        except (OSError, SyntaxError) as error:
            failures.append(f"{path.relative_to(ROOT)}: {error}")
    if failures:
        raise CheckFailure("Python syntax errors:\n" + "\n".join(failures))


def check_unit_tests() -> None:
    output = run_command(
        [
            sys.executable,
            "-B",
            "-m",
            "unittest",
            "discover",
            "-s",
            "tools",
            "-p",
            "test_*.py",
            "-q",
        ],
        show_output=False,
    )
    match = re.search(r"Ran (\d+) tests?", output)
    print(f"{match.group(1) if match else 'All'} host tests passed.")


def check_release_metadata() -> None:
    output = run_command([sys.executable, str(TOOLS / "validate_release.py")])
    warning_count = len(re.findall(r"^\[WARN]", output, flags=re.MULTILINE))
    if warning_count:
        raise CheckWarning(f"metadata validator reported {warning_count} warning(s)")


def check_javascript() -> None:
    node = require_command("node")
    run_command([node, "--check", str(ROOT / "webpanel" / "js" / "index.js")])


def check_generated_web_assets() -> None:
    with tempfile.TemporaryDirectory(prefix="easyiot-web-check-") as temporary:
        temp_root = Path(temporary)
        (temp_root / "tools").mkdir()
        (temp_root / "include").mkdir()
        shutil.copy2(ROOT / "platformio.ini", temp_root / "platformio.ini")
        shutil.copy2(TOOLS / "html_converter.py", temp_root / "tools" / "html_converter.py")
        shutil.copytree(ROOT / "webpanel", temp_root / "webpanel")
        run_command(
            [sys.executable, str(temp_root / "tools" / "html_converter.py")],
            cwd=temp_root,
        )
        stale = [
            name
            for name in GENERATED_HEADERS
            if not (ROOT / "include" / name).is_file()
            or (ROOT / "include" / name).read_bytes()
            != (temp_root / "include" / name).read_bytes()
        ]
    if stale:
        raise CheckFailure(
            "generated web headers are stale: "
            + ", ".join(f"include/{name}" for name in stale)
            + "; run python tools/html_converter.py"
        )


def repository_text_files() -> Iterable[Path]:
    git = require_command("git")
    result = subprocess.run(
        [git, "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )
    for raw_path in result.stdout.split(b"\0"):
        if not raw_path:
            continue
        path = ROOT / raw_path.decode("utf-8", errors="surrogateescape")
        if path.suffix.lower() in TEXT_SUFFIXES and path.is_file():
            yield path


def check_conflict_markers() -> None:
    matches: list[str] = []
    for path in repository_text_files():
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeError:
            continue
        for line_number, line in enumerate(lines, start=1):
            if CONFLICT_MARKER.fullmatch(line):
                matches.append(f"{path.relative_to(ROOT)}:{line_number}: {line}")
    if matches:
        raise CheckFailure("unresolved conflict markers:\n" + "\n".join(matches))


def check_whitespace() -> None:
    git = require_command("git")
    run_command([git, "diff", "--check"])
    run_command([git, "diff", "--cached", "--check"])


def platformio_command() -> str:
    for name in ("platformio", "pio"):
        command = shutil.which(name)
        if command:
            return command
    raise CheckFailure("PlatformIO is not installed or not on PATH")


def build_environment(environment: str) -> None:
    known = platformio_environments()
    if environment not in known:
        raise CheckFailure(f"unknown PlatformIO environment: {environment}")
    run_command(
        [platformio_command(), "run", "-e", environment],
        stream_output=True,
    )


def run_step(name: str, operation: Callable[[], None]) -> str:
    print(f"\n--- {name} ---", flush=True)
    try:
        operation()
    except CheckWarning as warning:
        print(f"[WARNING] {name}: {warning}")
        return "warning"
    except (CheckFailure, OSError, subprocess.SubprocessError) as error:
        print(f"[FAIL] {name}: {error}")
        return "failed"
    print(f"[PASS] {name}")
    return "passed"


def main() -> int:
    args = parse_args()
    checks: list[tuple[str, Callable[[], None]]] = [
        ("Python syntax", check_python_syntax),
        ("Host unit tests", check_unit_tests),
        ("Release metadata", check_release_metadata),
        ("JavaScript syntax", check_javascript),
        ("Generated web assets", check_generated_web_assets),
        ("Conflict markers", check_conflict_markers),
        ("Whitespace errors", check_whitespace),
    ]
    results = [run_step(name, operation) for name, operation in checks]

    builds = list(args.build)
    if args.all:
        version = project_version()
        builds = list(all_build_environments(version))
        print(f"\nBuild matrix for {version}: {', '.join(builds)}", flush=True)
    for environment in builds:
        results.append(
            run_step(
                f"PlatformIO build: {environment}",
                lambda environment=environment: build_environment(environment),
            )
        )

    passed = results.count("passed")
    warned = results.count("warning")
    failed = results.count("failed")
    print("\n==============================================")
    print(f"Project checks: {passed} passed, {warned} warning(s), {failed} failed")
    print("==============================================")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
