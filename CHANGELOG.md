# Changelog

All notable changes to this project are documented in this file.

## [9.17-dev] - 2026-02-12

### Build and Versioning
- Added support for local overrides via `platformio_override.ini`.
- Added `${extra.wifi_flags}` injection so local Wi-Fi settings stay out of git.
- Switched version define to string format: `VERSION='"${extra.version}"'`.
- Updated firmware reporting to use `String(VERSION)` for API, mDNS, and Home Assistant metadata.
- Improved `tools/extra_script.py` parsing for quoted `VERSION` values.
- Kept `platformio_override.ini` ignored as a local-only file.
- Added `tools/validate_release.sh` for pre-release metadata validation (version, changelog, release envs, OTA/config URL checks).
- Added automatic pre-build hooks in `tools/extra_script.py`:
  - run `tools/html_converter.sh`
  - run `tools/validate_release.sh`
- Added skip toggles in `platformio.ini`:
  - `SKIP_HTML_CONVERT`
  - `SKIP_RELEASE_VALIDATE`
- Enforced `WEB_SECURE_ON` in production/non-debug profiles and removed debug defaults from release builds (`platformio.ini`).
- Automated webpanel asset cache token versioning during HTML conversion using project version (`[extra] version`), removing manual hardcoded `?v=` updates.

### Security
- Stopped logging credential values in debug output (`src/CoreWiFi.cpp`, `src/ConfigOnofre.cpp`).
- Switched state-changing API routes to `POST` (`/reboot`, `/load-defaults`, `/templates/change`) and updated webpanel calls; temporary `GET` compatibility remains for older clients.
- Migrated CloudIO config and OTA endpoints from `http://` to `https://` in firmware constants (`include/Constants.h`).

### CloudIO
- Normalized firmware version in cloud config payload (strip `-dev` suffix for backend compatibility).
- Updated CloudIO config request to use secure client for HTTPS and added one-time silent HTTP fallback on connection/TLS failure to prevent restart loops (`src/CloudIO.cpp`).
- Kept serial logs clean by removing fallback/URL noise while preserving request status output (`src/CloudIO.cpp`).

### Webpanel
- Replaced `parseFloat` version compare with robust parser/comparator for `-dev` formats.
- Removed hardcoded API base URL and switched to same-origin requests.

### Code Quality
- Replaced deprecated ArduinoJson `containsKey()` checks with `isNull()` guards in config update flow.
- Added explicit ESP8266 no-op switch cases for ESP32-only drivers (`TMF882X`, `LD2410`) to clear switch warnings.

### Process and Release
- Added `docs/RELEASE_WORKFLOW.md` with CP/PR workflow steps.
- Added branch naming convention for CP branches.
- Added release checklist in repo docs.
- Added `tools/generate_release_notes.sh` to create `RELEASE_NOTES_DRAFT.md` from git history.

## [9.163] - Stable baseline

- Baseline before `9.17-dev` maintenance and build-flow updates.
