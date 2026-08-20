# EasyIot Release Workflow

This document defines the standard flow for development, upstream cherry-pick PRs, and release validation.

## Branch Naming

- `development`: main working branch in fork.
- `cp-<topic>`: upstream cherry-pick PR branch (example: `cp-warning-cleanup-config-sensors`).
- Keep topic names short and descriptive with kebab-case.

## Upstream CP/PR Flow

1. Work and test on `development`.
2. Commit focused changes.
3. Push `development` to `origin`.
4. Create CP branch from upstream base:
   - `git fetch upstream`
   - `git switch -c cp-<topic> upstream/master`
5. Cherry-pick the commit(s):
   - `git cherry-pick -x <commit_sha>`
6. Resolve conflicts if needed, then continue:
   - `git add <resolved-files>`
   - `GIT_EDITOR=true git cherry-pick --continue`
7. Push CP branch:
   - `git push -u origin cp-<topic>`
8. Open PR from `Hauzman:cp-<topic>` to `brunohorta82:master`.

## Release Checklist

1. Confirm version string in `platformio.ini` (`[extra] version = ...`).
2. Ensure local secrets stay in `platformio_override.ini` only.
3. Validate release metadata:
   - `tools/validate_release.sh`
   - strict release mode: `tools/validate_release.sh --release --fail-on-http`
4. Build release targets:
   - `platformio run -e ESP8266_RELEASE`
   - `platformio run -e ESP32_RELEASE`
5. Validate key runtime checks on device:
   - Wi-Fi connect
   - CloudIO HTTP result
   - MQTT connect
   - basic actuator/sensor control
6. Update `CHANGELOG.md` for the release/dev line.
7. Push `development` and prepare required CP PR(s).
8. Keep open CP PRs minimal and grouped by scope.

## Project Checks

Run the fast, non-destructive source checks before committing:

- `python3 tools/check_project.py --quick`

Run the same checks and one selected PlatformIO environment:

- `python3 tools/check_project.py --build ESP8266_DEBUG`

Run the recommended build matrix for the current version:

- `python3 tools/check_project.py --all`

For a version containing `-dev`, `--all` builds `ESP8266_DEBUG`,
`ESP8266-HAN_DEBUG`, and `ESP32_DEBUG`. For a release version, it builds the
three environments published by CI: `ESP8266_RELEASE`,
`ESP8266-HAN_RELEASE`, and `ESP32_RELEASE`.

The quick checks validate Python and JavaScript syntax, host tests, release
metadata, generated web-header parity, conflict markers, and whitespace. Web
asset parity is checked in a temporary directory, so the command does not
rewrite working-tree headers.

## Dependency Audit

Run the periodic dependency audit across every configured PlatformIO
environment:

- `python3 tools/audit_dependencies.py`

Audit only selected environments when investigating a specific target:

- `python3 tools/audit_dependencies.py --env ESP8266_DEBUG`
- `python3 tools/audit_dependencies.py --env ESP8266_DEBUG --env ESP32_DEBUG`

The runner executes `pio pkg outdated` independently for each environment, so a
broken platform does not hide the results for other targets. It reports
`Current`, `Wanted`, and `Latest` versions but never edits `platformio.ini` or
updates declared packages. PlatformIO may still refresh its own registry cache
or prepare an environment while inspecting it. The runner exits nonzero if any
environment could not be audited, while still printing results for the remaining
environments.

Treat the output as an investigation list, not an upgrade recommendation.
`Wanted` follows the current declaration; `Latest` may be incompatible, and Git
dependencies may point at a moving branch. Upgrade one dependency family at a
time, then run the relevant build matrix and hardware checks before changing a
pin.

## Local Wi-Fi Override

1. Copy `platformio_override.example.ini` to `platformio_override.ini`.
2. Uncomment `WIFI_SSID` and `WIFI_SECRET` in the local copy and replace the placeholders.
3. Never commit `platformio_override.ini`; it is intentionally ignored by Git.
4. Keep `platformio.ini` free of real Wi-Fi credentials.

## Local Firmware Binaries

`tools/export_firmware.py` preserves disposable PlatformIO outputs under the
ignored `firmware_bins/` directory. Each environment has its own candidate slot,
so ESP8266, HAN, and ESP32 binaries cannot be confused.

Successful environments whose names contain `DEBUG` or `RELEASE` publish their
candidate automatically at the end of `platformio run`. This post-build step also
runs when PlatformIO reuses an unchanged cached binary. Environments without an
explicit channel in their name are not guessed; export those manually with
`--channel debug` or `--channel release`.

Build a development candidate:

- `platformio run -e ESP8266_DEBUG`

Build a release candidate:

- `platformio run -e ESP8266_RELEASE`

The equivalent manual publish command is:

- `python tools/export_firmware.py publish --env ESP8266_DEBUG`

Verify a candidate without modifying it:

- `python tools/export_firmware.py verify --export-dir firmware_bins/candidate/debug/ESP8266_DEBUG --expected-env ESP8266_DEBUG`

After that exact binary passes hardware testing, deliberately retain it as
known-good firmware:

- `python tools/export_firmware.py promote --candidate-dir firmware_bins/candidate/debug/ESP8266_DEBUG --hardware-tested-date today`

Candidates are replaceable, but an existing known-good version is immutable: the
tool refuses to overwrite it with different bytes or metadata. Every export has
`BUILD_INFO.txt` and `SHA256SUMS.txt`. The ignored local directory is not an
off-machine backup; archive important known-good firmware separately.

## Quick Rollback

A rollback installs a previously hardware-tested build for the exact device
variant. Do not choose a binary from the MCU family alone: standard, HAN, and
other board profiles can use different firmware and partition layouts.

### Prepare Before Deployment

1. In the WebUI, open **System -> Access & Backup** and export the device
   configuration.
2. Verify the candidate that passed hardware testing:
   - `python3 tools/export_firmware.py verify --export-dir firmware_bins/candidate/debug/ESP8266_DEBUG --expected-env ESP8266_DEBUG`
3. Promote that exact candidate to the immutable local known-good area:
   - `python3 tools/export_firmware.py promote --candidate-dir firmware_bins/candidate/debug/ESP8266_DEBUG --hardware-tested-date today`
4. Archive the known-good binary, `BUILD_INFO.txt`, and `SHA256SUMS.txt` outside
   the ignored local `firmware_bins/` directory.

### Device WebUI Is Reachable

1. Identify the current board/variant and select the matching previous
   application binary. A local known-good export can be checked again with:
   - `python3 tools/export_firmware.py verify --export-dir firmware_bins/known-good/<ENV>/v<VERSION> --expected-env <ENV>`
2. Open **System -> Firmware**, choose that `.bin`, and send it to the device.
3. Keep the device powered until the upload completes and it restarts.
4. Reopen the WebUI and verify the displayed firmware version, Wi-Fi, CloudIO or
   MQTT connection, and basic actuator/sensor operation.

This application-only OTA path normally preserves the stored configuration. Do
not factory-reset the device as part of a routine rollback.

### Device WebUI Is Unreachable or It Boot-Loops

1. Use the matching full-flash `ONOFRE_<MCU>_WEBFLASH_<VERSION>.bin` artifact
   through the browser installer at `/flash/`, or use the same artifact from the
   CI run that built the release.
2. Prefer a published, hardware-tested full-flash image. Do not use an
   application-only `ONOFRE_<MCU>_RELEASE_<VERSION>.bin` as a blank-chip image
   for ESP32-family devices; it does not include the bootloader and partition
   data.
3. Expect full-flash recovery to be capable of erasing configuration. Once the
   device is reachable, restore the configuration export made before deployment.
4. If rebuilding from source is unavoidable, check out the exact known-good
   commit/tag, select the exact PlatformIO environment, build it, and record the
   binary and checksum used.

Do not copy raw flash offsets between ESP8266, ESP32, and ESP32-C6 recovery
commands. Their complete images and layouts differ. If the device cannot enter
its serial bootloader, physical access to its boot/reset controls and a working
USB data connection is required; WebUI recovery is no longer possible.

### Contain and Verify the Rollback

1. The release owner should stop offering the faulty artifact before rolling
   devices back, so an automatic update cannot immediately reinstall it.
2. Record the affected variant, failed version, observed failure, rollback
   version, binary checksum, and configuration-restoration result.
3. Treat the rollback as complete only after restart, reconnect, control, and
   relevant sensor checks pass on the real device.
4. Correct and revalidate the release before publishing it again.

## Practical Notes

- Prefer one CP per logical scope (security, webpanel, docs, etc.).
- If a PR is closed or merged, delete local/remote CP branch to keep repo clean.
- If `git cherry-pick --continue` fails due to editor, use:
  - `GIT_EDITOR=true git cherry-pick --continue`
- Build hooks are automatic via `tools/extra_script.py` and
  `tools/post_extra_script.py`:
  - HTML converter runs before build
  - Release metadata validation runs before build
  - Local DEBUG/RELEASE candidate export runs after a successful build
- Optional skip defines in `platformio.ini` (`[extra] default_flags`):
  - `SKIP_HTML_CONVERT`
  - `SKIP_RELEASE_VALIDATE`

## Release Notes Draft

- Generate a draft from recent commits:
  - `python tools/generate_release_notes.py` (Windows: `py -3 tools/generate_release_notes.py`)
- Generate from a specific range:
  - `python tools/generate_release_notes.py --range <from..to>`
- Change output file:
  - `python tools/generate_release_notes.py --count 25 --output RELEASE_NOTES_DRAFT.md`
- Default output path is repo root:
  - `RELEASE_NOTES_DRAFT.md`
- Shell wrapper (macOS/Linux):
  - `tools/generate_release_notes.sh`
