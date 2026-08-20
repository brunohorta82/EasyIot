# EasyIot - To Do

- Created by: Alexandru Hauzman
- Updated: 16.08.2026
- Current upstream version: 9.168

## Important Notes

- Track active work in Backlog.
- Move completed items to Done.

# Backlog

## Firmware & Versioning (P1)

## Security & OTA (P1)

1. [ ] (Blocked) Remove temporary CloudIO HTTP fallback after full TLS compatibility is confirmed on devices, including weak-signal scenarios. Blocked: the whole fleet must be on >= 9.161 first, otherwise devices that fail the TLS handshake have no way to sync. File: `src/CloudIO.cpp`
2. [x] Convert state-changing endpoints from GET to POST (`/reboot`, `/load-defaults`, `/templates/change`); temporary GET compatibility kept for older clients. File: `src/WebServer.cpp`
3. [ ] (Blocked - no boards available) Validate OTA update flow over HTTPS on remaining device variants (ESP32C3 / HAN). File: `src/WebServer.cpp`
4. [ ] Replace default credentials (`admin` / `xpto` / default AP secret) with a first-boot forced change flow. File: `include/Constants.h`
5. [ ] Change captive portal save flow from GET query params to POST body (avoid leaking passwords in URL/history). Files: `include/CaptivePortal.h`, `src/WebServer.cpp`
6. [ ] Add OTA integrity check (signed firmware or hash validation) before applying update. File: `src/WebServer.cpp`

## Dependencies & Library Updates (P1/P2)

1. [ ] Pin PlatformIO platforms to known-good versions (`espressif32@...`, `espressif8266@...`) for reproducible builds. File: `platformio.ini`
2. [x] Pin GitHub-based `lib_deps` to tags/registry versions (avoid floating `master/main`) — SHT4x pinned to 1.1.2 after upstream renamed a header and broke every Linux build; the remaining git URLs are still unpinned. File: `platformio.ini`
3. [ ] Add periodic dependency audit task (`pio pkg outdated` + compatibility notes per env).

## Webpanel UX (P1/P2)

1. [ ] Improve the Functions tab layout: make the add-function form clearer and more compact, visually delimit each configured feature as its own card, and use a responsive two-column grid on wider screens that stacks to one column on phones. Files: `webpanel/index.html`, `webpanel/css/styles.css`, `webpanel/js/index.js`
2. [ ] Add a subtle left-edge health indicator to relevant status badges and diagnostic cards: green for healthy/connected, orange for degraded/retrying, red only for actual errors/disconnection, and gray for disabled/unknown. Keep the existing text or icon so status never relies on color alone; normal actuator OFF states and intentionally disabled MQTT must not appear as errors. Files: `webpanel/index.html`, `webpanel/css/styles.css`, `webpanel/js/index.js`

## Testing & CI (P2 - Deferred / Later)

1. [x] CI build checks for main envs — `.github/workflows/firmware-ota.yml` builds all four release envs (ESP8266, ESP8266-HAN, ESP32, ESP32-MAKER-4MB) and publishes them to the OTA folder on tag/dispatch.
2. [ ] Add smoke tests for boot, Wi-Fi, MQTT, OTA update path.

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
11. [x] Resolved strict release-validation mismatch for `9.17` by adding matching changelog header and confirmed full all-env build pass for release snapshot.

## Security

1. [x] Stopped logging credential values in debug output (`src/CoreWiFi.cpp`, `src/ConfigOnofre.cpp`).
2. [x] Migrated CloudIO config and OTA endpoints from `http://` to `https://` and validated runtime behavior on device. Files: `include/Constants.h`, `src/CloudIO.cpp`, `src/WebServer.cpp`
3. [x] Converted state-changing endpoints from GET to POST (`/reboot`, `/load-defaults`, `/templates/change`). File: `src/WebServer.cpp`
4. [x] Added HTTPS-first CloudIO config request with one-time silent HTTP fallback to prevent restart loops when TLS path fails. File: `src/CloudIO.cpp`
5. [x] Validated OTA update flow over HTTPS on ESP8266 (`Update Success` + reconnect to CloudIO/MQTT). File: `src/WebServer.cpp`
6. [x] Validated OTA update flow over HTTPS on ESP32 (`Update Success` + reboot + reconnect to CloudIO/MQTT; `HTTPS result: 200`, `fallback=0`). File: `src/WebServer.cpp`

## Webpanel

1. [x] Fixed firmware version comparison for `-dev` formats (replaced `parseFloat` logic). File: `webpanel/js/index.js`
2. [x] Removed hardcoded `baseUrl` and switched to same-origin requests. File: `webpanel/js/index.js`
3. [x] Added automatic firmware version display in webpanel footer (`version_lbl` from `/config`). Files: `webpanel/index.html`, `webpanel/js/index.js`
4. [x] Added firmware build date to the API/system information payload. File: `src/ConfigOnofre.cpp`
5. [x] Made binary actuator Overview rows full-width native controls with keyboard and `aria-pressed` support, plus green ON and neutral OFF accents. Files: `webpanel/css/styles.css`, `webpanel/js/index.js`
6. [x] Restored the running firmware version in the persistent header beside the device metadata. Files: `webpanel/index.html`, `webpanel/js/index.js`
7. [x] Added bounded browser-only climate history graphs for DS18B20, DHT11/21/22, and SHT4x sensors with separate temperature/humidity scales, stable-state context, timestamps, and no device flash writes. Persistent day/week history remains a separate storage/API design. Files: `webpanel/css/styles.css`, `webpanel/js/index.js`

## Code Quality

1. [x] Replaced deprecated ArduinoJson `containsKey()` checks in config update path with `isNull()` guards. File: `src/ConfigOnofre.cpp`
2. [x] Added explicit ESP8266 no-op switch cases for ESP32-only sensor drivers (`TMF882X`, `LD2410`) to remove compiler switch warnings. File: `src/Sensors.cpp`

## Process & Release

1. [x] Added PR workflow guide (development -> cherry-pick branch -> upstream PR). File: `docs/RELEASE_WORKFLOW.md`
2. [x] Added branch naming convention for external CP branches. File: `docs/RELEASE_WORKFLOW.md`
3. [x] Added release checklist document in repo docs. File: `docs/RELEASE_WORKFLOW.md`
4. [x] Added script to generate release notes draft from commits. File: `tools/generate_release_notes.sh`
5. [x] Added an exact-variant rollback playbook for WebUI OTA recovery, full-flash recovery, configuration restore, containment, and post-rollback verification. File: `docs/RELEASE_WORKFLOW.md`

## Security & API

1. [x] Converted state-changing endpoints to support `POST` (`/reboot`, `/load-defaults`, `/templates/change`) and switched webpanel calls to `POST` while keeping temporary `GET` compatibility. Files: `src/WebServer.cpp`, `webpanel/js/index.js`

## Security

1. [x] Stopped logging credential values in debug output (`src/CoreWiFi.cpp`, `src/ConfigOnofre.cpp`).

## Webpanel

1. [x] Fixed firmware version comparison for `-dev` formats (replaced `parseFloat` logic). File: `webpanel/js/index.js`
2. [x] Removed hardcoded `baseUrl` and switched to same-origin requests. File: `webpanel/js/index.js`
## Quick Release Flow

1. Bump version in `platformio.ini`.
2. Build target environments.
3. Validate OTA and displayed firmware version.
4. Commit and push changes.
5. Open PR and release notes.
