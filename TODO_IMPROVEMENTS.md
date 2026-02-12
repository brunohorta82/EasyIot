# EasyIot - To Do

Created by: Alexandru Hauzman  
Updated: 12.02.2026  
Current version: 9.17-dev

## Important Notes

- Track active work in Backlog.
- Move completed items to Done.

# Backlog

## Firmware & Versioning (P1)

1. [ ] Add version/OTA metadata validation before release.

## Security & OTA (P1)

1. [ ] Move cloud config/OTA URLs from `http://` to secure transport and validate update path. File: `include/Constants.h`
2. [ ] Convert state-changing endpoints from GET to POST (`/reboot`, `/load-defaults`, `/templates/change`). File: `src/WebServer.cpp`
3. [ ] Ensure release profiles enforce `WEB_SECURE_ON` and avoid debug defaults in production builds. File: `platformio.ini`

## Webpanel UX (P1/P2)

1. [ ] Add automatic version banner in webpanel footer.
2. [ ] Add firmware build date in API/system info payload.

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

## Security

1. [x] Stopped logging credential values in debug output (`src/CoreWiFi.cpp`, `src/ConfigOnofre.cpp`).

## Webpanel

1. [x] Fixed firmware version comparison for `-dev` formats (replaced `parseFloat` logic). File: `webpanel/js/index.js`
2. [x] Removed hardcoded `baseUrl` and switched to same-origin requests. File: `webpanel/js/index.js`

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
