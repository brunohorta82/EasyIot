# EasyIot - To Do

Created by: Alexandru Hauzman  
Updated: 14.02.2026  
Current version: 9.17-dev

## Important Notes

- Track active work in Backlog.
- Move completed items to Done.

# Backlog

## Firmware & Versioning (P1)

## Security & OTA (P1)

1. [ ] Remove temporary CloudIO HTTP fallback after full TLS compatibility is confirmed on devices. File: `src/CloudIO.cpp`
2. [ ] Validate OTA update flow over HTTPS on remaining device variants (ESP32 / ESP32C3 / HAN). File: `src/WebServer.cpp`

## Webpanel UX (P1/P2)

1. [ ] Add firmware build date in API/system info payload.

## Testing & CI (P2)

1. [ ] Add CI build checks for main envs (ESP8266 + ESP32).
2. [ ] Add smoke tests for boot, Wi-Fi, MQTT, OTA update path.
3. [ ] Add quick rollback notes for failed release/update.

#

# Done

## Build & Version

1. [x] Added support for `platformio_override.ini` local overrides.
2. [x] Added `wifi_flags` injection via `${extra.wifi_flags}`.
3. [x] Updated firmware version format support (example: `9.17-dev`).
4. [x] Updated code/version reporting to use string `VERSION`.
5. [x] Improved `extra_script.py` handling for quoted `VERSION` values.
6. [x] Added `CHANGELOG.md` as the single release-history file.
7. [x] Added pre-release metadata validator (version/changelog/env/OTA URL checks). File: `tools/validate_release.sh`
8. [x] Added automatic pre-build hooks for HTML conversion and release validation with skip flags. Files: `tools/extra_script.py`, `platformio.ini`
9. [x] Enforced `WEB_SECURE_ON` for production/non-debug profiles and removed debug defaults from release builds. File: `platformio.ini`
10. [x] Automated webpanel asset cache version (`?v=`) from project version during build conversion (no manual hardcoded value updates). Files: `webpanel/index.html`, `tools/html_converter.sh`

## Security

1. [x] Stopped logging credential values in debug output (`src/CoreWiFi.cpp`, `src/ConfigOnofre.cpp`).
2. [x] Migrated CloudIO config and OTA endpoints from `http://` to `https://` and validated runtime behavior on device. Files: `include/Constants.h`, `src/CloudIO.cpp`, `src/WebServer.cpp`
3. [x] Converted state-changing endpoints from GET to POST (`/reboot`, `/load-defaults`, `/templates/change`). File: `src/WebServer.cpp`
4. [x] Added HTTPS-first CloudIO config request with one-time silent HTTP fallback to prevent restart loops when TLS path fails. File: `src/CloudIO.cpp`
5. [x] Validated OTA update flow over HTTPS on ESP8266 (`Update Success` + reconnect to CloudIO/MQTT). File: `src/WebServer.cpp`

## Webpanel

1. [x] Fixed firmware version comparison for `-dev` formats (replaced `parseFloat` logic). File: `webpanel/js/index.js`
2. [x] Removed hardcoded `baseUrl` and switched to same-origin requests. File: `webpanel/js/index.js`
3. [x] Added automatic firmware version display in webpanel footer (`version_lbl` from `/config`). Files: `webpanel/index.html`, `webpanel/js/index.js`

## Code Quality

1. [x] Replaced deprecated ArduinoJson `containsKey()` checks in config update path with `isNull()` guards. File: `src/ConfigOnofre.cpp`
2. [x] Added explicit ESP8266 no-op switch cases for ESP32-only sensor drivers (`TMF882X`, `LD2410`) to remove compiler switch warnings. File: `src/Sensors.cpp`

## Process & Release

1. [x] Added PR workflow guide (development -> cherry-pick branch -> upstream PR). File: `docs/RELEASE_WORKFLOW.md`
2. [x] Added branch naming convention for external CP branches. File: `docs/RELEASE_WORKFLOW.md`
3. [x] Added release checklist document in repo docs. File: `docs/RELEASE_WORKFLOW.md`
4. [x] Added script to generate release notes draft from commits. File: `tools/generate_release_notes.sh`

## Quick Release Flow

1. Bump version in `platformio.ini`.
2. Build target environments.
3. Validate OTA and displayed firmware version.
4. Commit and push changes.
5. Open PR and release notes.
