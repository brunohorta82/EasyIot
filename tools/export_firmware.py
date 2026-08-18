#!/usr/bin/env python3
"""Publish, verify, and promote local EasyIot firmware binaries."""

from __future__ import annotations

import argparse
from datetime import date, datetime
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import uuid


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_ROOT = ROOT / "firmware_bins"
EXPORT_MARKER = ".easyiot-firmware-export"
CHECKSUM_PATTERN = re.compile(r"^([0-9a-f]{64})  ([^/\\]+)$")
VERSION_PATTERN = re.compile(r"^[0-9]+(?:\.[0-9]+){1,2}(?:-[A-Za-z0-9._-]+)?$")
GENERATED_NAME_PATTERN = re.compile(r" - (\d{2}\.\d{2}\.\d{4})\.bin$")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    publish_parser = commands.add_parser(
        "publish", help="copy one PlatformIO binary into the current candidate slot"
    )
    publish_parser.add_argument("--env", required=True, help="PlatformIO environment")
    publish_parser.add_argument(
        "--channel",
        choices=("debug", "release"),
        help="inferred from an environment containing DEBUG or RELEASE when omitted",
    )
    publish_parser.add_argument(
        "--bin", type=Path, help="explicit binary; otherwise locate it in .pio/build/<env>"
    )
    publish_parser.add_argument(
        "--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT
    )
    publish_parser.add_argument(
        "--date", dest="date_stamp", help="build date in YYYY-MM-DD format"
    )

    verify_parser = commands.add_parser(
        "verify", help="verify a candidate or known-good export without changing it"
    )
    verify_parser.add_argument("--export-dir", required=True, type=Path)
    verify_parser.add_argument(
        "--expected-channel", choices=("debug", "release")
    )
    verify_parser.add_argument("--expected-env")

    promote_parser = commands.add_parser(
        "promote", help="retain a hardware-tested candidate as an immutable known-good build"
    )
    promote_parser.add_argument("--candidate-dir", required=True, type=Path)
    promote_parser.add_argument(
        "--known-good-root",
        type=Path,
        default=DEFAULT_OUTPUT_ROOT / "known-good",
    )
    promote_parser.add_argument(
        "--hardware-tested-date",
        required=True,
        help="test date in YYYY-MM-DD format, or 'today'",
    )
    return parser.parse_args()


def parse_iso_date(value: str, *, label: str) -> date:
    try:
        return datetime.strptime(value, "%Y-%m-%d").date()
    except ValueError as error:
        raise ValueError(f"{label} must use YYYY-MM-DD") from error


def filename_token(value: str) -> str:
    token = re.sub(r"[^A-Za-z0-9.+-]+", "_", value).strip("_")
    if not token:
        raise ValueError("value does not contain filename-safe characters")
    return token


def read_project_version(platformio_ini: Path = ROOT / "platformio.ini") -> str:
    section = ""
    for raw_line in platformio_ini.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].strip()
            continue
        if section == "extra" and re.match(r"^version\s*=", line):
            version = line.split("=", 1)[1].split(";", 1)[0].split("#", 1)[0].strip()
            if not VERSION_PATTERN.fullmatch(version):
                raise ValueError(f"invalid [extra] version in {platformio_ini}: {version!r}")
            return version
    raise ValueError(f"could not read [extra] version from {platformio_ini}")


def read_project_environments(platformio_ini: Path = ROOT / "platformio.ini") -> set[str]:
    environments: set[str] = set()
    for raw_line in platformio_ini.read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"\s*\[env:([^]]+)]\s*", raw_line)
        if match:
            environments.add(match.group(1).strip())
    return environments


def infer_channel(environment: str, explicit: str | None) -> str:
    upper = environment.upper()
    inferred = "release" if "RELEASE" in upper else "debug" if "DEBUG" in upper else None
    if inferred:
        if explicit and explicit != inferred:
            raise ValueError(
                f"environment {environment!r} implies {inferred!r}, not {explicit!r}"
            )
        return inferred
    if explicit:
        return explicit
    raise ValueError(
        f"cannot infer channel from environment {environment!r}; use --channel"
    )


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def locate_binary(
    environment: str,
    version: str,
    explicit: Path | None,
    *,
    project_root: Path = ROOT,
) -> Path:
    if explicit is not None:
        binary = explicit.expanduser().resolve()
        if not binary.is_file() or binary.suffix.lower() != ".bin":
            raise FileNotFoundError(f"firmware binary not found: {binary}")
        if binary.name.startswith("Firmware_") and not binary.name.startswith(
            f"Firmware_{environment}_"
        ):
            raise ValueError(
                f"binary name does not match environment {environment!r}: {binary.name}"
            )
        if binary.parent.parent.name == "build" and binary.parent.name != environment:
            raise ValueError(
                f"binary build directory does not match environment {environment!r}: "
                f"{binary.parent.name!r}"
            )
        return binary

    build_dir = project_root / ".pio" / "build" / environment
    matches = sorted(build_dir.glob(f"Firmware_{environment}_{version} - *.bin"))
    if len(matches) > 1:
        names = ", ".join(path.name for path in matches)
        raise ValueError(f"multiple matching binaries found; use --bin: {names}")
    if len(matches) == 1:
        return matches[0].resolve()

    fallback = build_dir / "firmware.bin"
    if fallback.is_file():
        return fallback.resolve()
    raise FileNotFoundError(
        f"no binary for {environment} {version} in {build_dir}; build it first"
    )


def resolve_build_date(binary: Path, explicit: str | None) -> date:
    if explicit:
        return parse_iso_date(explicit, label="build date")
    match = GENERATED_NAME_PATTERN.search(binary.name)
    if match:
        return datetime.strptime(match.group(1), "%d.%m.%Y").date()
    return date.today()


def git_state(project_root: Path = ROOT) -> tuple[str, str]:
    try:
        commit = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=project_root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        dirty = subprocess.run(
            ["git", "status", "--porcelain", "--untracked-files=no"],
            cwd=project_root,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        return commit, "yes" if dirty else "no"
    except (OSError, subprocess.CalledProcessError):
        return "unknown", "unknown"


def validate_owned_destination(destination: Path) -> None:
    if not destination.exists():
        return
    if destination.is_symlink() or not destination.is_dir():
        raise ValueError(f"output path is not a regular directory: {destination}")
    entries = [entry for entry in destination.iterdir() if entry.name != ".DS_Store"]
    marker = destination / EXPORT_MARKER
    if entries and (not marker.is_file() or marker.is_symlink()):
        raise ValueError(f"refusing to replace unowned directory: {destination}")


def read_build_info(directory: Path) -> dict[str, str]:
    path = directory / "BUILD_INFO.txt"
    if not path.is_file():
        raise ValueError(f"missing build information: {path}")
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if ": " in line:
            key, value = line.split(": ", 1)
            values[key] = value
    required = ("Version", "Environment", "Channel", "Build date", "Binary")
    for key in required:
        if not values.get(key):
            raise ValueError(f"missing {key!r} in {path}")
    if not VERSION_PATTERN.fullmatch(values["Version"]):
        raise ValueError(f"invalid version in {path}")
    if values["Channel"] not in ("debug", "release"):
        raise ValueError(f"invalid channel in {path}")
    parse_iso_date(values["Build date"], label="build date")
    if Path(values["Binary"]).name != values["Binary"]:
        raise ValueError(f"invalid binary name in {path}")
    return values


def verified_checksum(directory: Path) -> tuple[str, str]:
    manifest = directory / "SHA256SUMS.txt"
    if not manifest.is_file():
        raise ValueError(f"missing checksum manifest: {manifest}")
    lines = manifest.read_text(encoding="ascii").splitlines()
    if len(lines) != 1:
        raise ValueError(f"expected exactly one checksum in {manifest}")
    match = CHECKSUM_PATTERN.fullmatch(lines[0])
    if match is None:
        raise ValueError(f"malformed checksum in {manifest}")
    expected, name = match.groups()
    binary = directory / name
    if not binary.is_file() or binary.suffix.lower() != ".bin":
        raise ValueError(f"missing checksummed binary: {binary}")
    if sha256(binary) != expected:
        raise ValueError(f"checksum mismatch for {binary}")
    return name, expected


def verify_export(
    directory: Path,
    *,
    expected_channel: str | None = None,
    expected_env: str | None = None,
) -> tuple[dict[str, str], str]:
    export_dir = directory.expanduser().resolve()
    if (
        not export_dir.is_dir()
        or export_dir.is_symlink()
        or not (export_dir / EXPORT_MARKER).is_file()
        or (export_dir / EXPORT_MARKER).is_symlink()
    ):
        raise ValueError(f"not an EasyIot firmware export: {export_dir}")
    info = read_build_info(export_dir)
    name, digest = verified_checksum(export_dir)
    if info["Binary"] != name:
        raise ValueError("BUILD_INFO.txt and SHA256SUMS.txt name different binaries")
    expected_name = (
        f"ONOFRE_{filename_token(info['Environment'])}_"
        f"{filename_token(info['Version'])}_{info['Build date']}.bin"
    )
    if name != expected_name:
        raise ValueError(f"binary name does not match its build metadata: {name}")
    if expected_channel and info["Channel"] != expected_channel:
        raise ValueError(
            f"expected channel {expected_channel!r}, found {info['Channel']!r}"
        )
    if expected_env and info["Environment"] != expected_env:
        raise ValueError(
            f"expected environment {expected_env!r}, found {info['Environment']!r}"
        )
    return info, digest


def export_identity(info: dict[str, str]) -> tuple[str, ...]:
    """Return fields that identify the built artifact, excluding promotion metadata."""
    return tuple(
        info.get(key, "")
        for key in (
            "Version",
            "Environment",
            "Channel",
            "Build date",
            "Git commit",
            "Tracked changes present",
            "Binary",
        )
    )


def write_export(
    staging: Path,
    *,
    binary: Path,
    output_name: str,
    version: str,
    environment: str,
    channel: str,
    build_date: date,
    commit: str,
    dirty: str,
) -> None:
    output = staging / output_name
    shutil.copy2(binary, output)
    digest = sha256(output)
    (staging / "SHA256SUMS.txt").write_text(
        f"{digest}  {output.name}\n", encoding="ascii"
    )
    (staging / "BUILD_INFO.txt").write_text(
        "\n".join(
            (
                "EASYIOT FIRMWARE CANDIDATE",
                "",
                f"Version: {version}",
                f"Environment: {environment}",
                f"Channel: {channel}",
                f"Build date: {build_date.isoformat()}",
                f"Git commit: {commit}",
                f"Tracked changes present: {dirty}",
                f"Binary: {output.name}",
                "",
            )
        ),
        encoding="utf-8",
    )
    (staging / EXPORT_MARKER).write_text(
        "Owned by tools/export_firmware.py\n", encoding="ascii"
    )


def publish(
    *,
    environment: str,
    channel: str | None,
    explicit_binary: Path | None,
    output_root: Path,
    date_stamp: str | None,
    project_root: Path = ROOT,
) -> Path:
    platformio_ini = project_root / "platformio.ini"
    version = read_project_version(platformio_ini)
    environments = read_project_environments(platformio_ini)
    if environment not in environments:
        raise ValueError(f"unknown PlatformIO environment: {environment}")
    resolved_channel = infer_channel(environment, channel)
    binary = locate_binary(
        environment, version, explicit_binary, project_root=project_root
    )
    build_date = resolve_build_date(binary, date_stamp)
    destination = (
        output_root.expanduser().resolve()
        / "candidate"
        / resolved_channel
        / filename_token(environment)
    )
    validate_owned_destination(destination)

    output_name = (
        f"ONOFRE_{filename_token(environment)}_{filename_token(version)}_"
        f"{build_date.isoformat()}.bin"
    )
    if destination.is_dir() and (destination / EXPORT_MARKER).is_file():
        try:
            info, existing_digest = verify_export(destination)
            if (
                info["Version"] == version
                and info["Environment"] == environment
                and info["Channel"] == resolved_channel
                and info["Build date"] == build_date.isoformat()
                and existing_digest == sha256(binary)
            ):
                print(f"Firmware candidate is already current: {destination}")
                return destination
        except (OSError, UnicodeError, ValueError):
            pass

    destination.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(
        tempfile.mkdtemp(prefix=f".{destination.name}.new-", dir=destination.parent)
    )
    backup = destination.parent / f".{destination.name}.old-{uuid.uuid4().hex}"
    try:
        commit, dirty = git_state(project_root)
        write_export(
            staging,
            binary=binary,
            output_name=output_name,
            version=version,
            environment=environment,
            channel=resolved_channel,
            build_date=build_date,
            commit=commit,
            dirty=dirty,
        )
        if destination.exists():
            os.replace(destination, backup)
        try:
            os.replace(staging, destination)
        except Exception:
            if backup.exists() and not destination.exists():
                os.replace(backup, destination)
            raise
        if backup.exists():
            shutil.rmtree(backup)
    finally:
        if staging.exists():
            shutil.rmtree(staging)
    verify_export(
        destination,
        expected_channel=resolved_channel,
        expected_env=environment,
    )
    print(f"Firmware candidate published: {destination}")
    return destination


def promote(
    *,
    candidate_dir: Path,
    known_good_root: Path,
    hardware_tested_date: str,
) -> Path:
    candidate = candidate_dir.expanduser().resolve()
    tested = (
        date.today()
        if hardware_tested_date == "today"
        else parse_iso_date(hardware_tested_date, label="hardware-tested date")
    )
    if tested > date.today():
        raise ValueError("hardware-tested date must not be in the future")

    info, digest = verify_export(candidate)
    build_date = parse_iso_date(info["Build date"], label="build date")
    if tested < build_date:
        raise ValueError("hardware-tested date must not precede the build date")
    version_dir = filename_token(info["Version"])
    if not version_dir.lower().startswith("v"):
        version_dir = f"v{version_dir}"
    destination = (
        known_good_root.expanduser().resolve()
        / filename_token(info["Environment"])
        / version_dir
    )
    if destination.exists():
        existing_info, existing_digest = verify_export(destination)
        if export_identity(existing_info) == export_identity(info) and existing_digest == digest:
            print(f"Known-good firmware already exists: {destination}")
            return destination
        raise ValueError(
            "known-good version already exists with different firmware or metadata; "
            "use a new version instead of overwriting it"
        )

    destination.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(
        tempfile.mkdtemp(prefix=f".{version_dir}.new-", dir=destination.parent)
    )
    try:
        for name in (info["Binary"], "SHA256SUMS.txt"):
            shutil.copy2(candidate / name, staging / name)
        (staging / "BUILD_INFO.txt").write_text(
            (candidate / "BUILD_INFO.txt").read_text(encoding="utf-8")
            + f"Hardware-tested: {tested.isoformat()}\n"
            + f"Promoted: {date.today().isoformat()}\n"
            + "Status: hardware-tested known-good\n",
            encoding="utf-8",
        )
        (staging / "RESTORE.txt").write_text(
            "\n".join(
                (
                    "EASYIOT KNOWN-GOOD LOCAL FIRMWARE",
                    "",
                    f"Environment: {info['Environment']}",
                    f"Version: {info['Version']}",
                    f"Hardware-tested: {tested.isoformat()}",
                    f"Binary: {info['Binary']}",
                    "",
                    "Verify the board variant before flashing this application binary.",
                    "This ignored local directory is not an off-machine backup.",
                    "",
                )
            ),
            encoding="utf-8",
        )
        (staging / EXPORT_MARKER).write_text(
            "Owned by tools/export_firmware.py\n", encoding="ascii"
        )
        if destination.exists():
            raise ValueError(f"known-good destination appeared: {destination}")
        os.replace(staging, destination)
    finally:
        if staging.exists():
            shutil.rmtree(staging)
    verify_export(destination)
    print(f"Known-good firmware promoted: {destination}")
    return destination


def main() -> int:
    args = parse_args()
    try:
        if args.command == "publish":
            publish(
                environment=args.env,
                channel=args.channel,
                explicit_binary=args.bin,
                output_root=args.output_root,
                date_stamp=args.date_stamp,
            )
        elif args.command == "verify":
            info, digest = verify_export(
                args.export_dir,
                expected_channel=args.expected_channel,
                expected_env=args.expected_env,
            )
            print("EasyIot firmware export verified:")
            print(f"  Environment: {info['Environment']}")
            print(f"  Version: {info['Version']}")
            print(f"  Channel: {info['Channel']}")
            print(f"  Binary: {args.export_dir.expanduser().resolve() / info['Binary']}")
            print(f"  SHA-256: {digest}")
        else:
            promote(
                candidate_dir=args.candidate_dir,
                known_good_root=args.known_good_root,
                hardware_tested_date=args.hardware_tested_date,
            )
    except (FileNotFoundError, OSError, UnicodeError, ValueError) as error:
        raise SystemExit(f"Firmware export failed: {error}") from error
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
