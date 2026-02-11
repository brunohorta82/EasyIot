# EasyIot - To Do

Created by: Alexandru Hauzman  
Updated: 11.02.2026  
Current version: 9.17-dev

## Important Notes

- Track active work in Backlog.
- Move completed items to Done.

# Backlog

## Firmware & Versioning (P1)

1. [ ] Fix firmware version compare in webpanel for `-dev` formats (replace `parseFloat` logic). File: `webpanel/js/index.js`
2. [ ] Add version/OTA metadata validation before release.
3. [ ] Add `CHANGELOG.md` with version-by-version entries.

## Security & OTA (P1)

1. [ ] Move cloud config/OTA URLs from `http://` to secure transport and validate update path. File: `include/Constants.h`
2. [ ] Remove Wi-Fi password from debug logs (never print secrets). File: `src/CoreWiFi.cpp`
3. [ ] Convert state-changing endpoints from GET to POST (`/reboot`, `/load-defaults`, `/templates/change`). File: `src/WebServer.cpp`
4. [ ] Ensure release profiles enforce `WEB_SECURE_ON` and avoid debug defaults in production builds. File: `platformio.ini`

## Webpanel UX (P1/P2)

1. [ ] Remove hardcoded `baseUrl` and use same-origin requests. File: `webpanel/js/index.js`
2. [ ] Add automatic version banner in webpanel footer.
3. [ ] Add firmware build date in API/system info payload.

## Testing & CI (P2)

1. [ ] Add CI build checks for main envs (ESP8266 + ESP32).
2. [ ] Add smoke tests for boot, Wi-Fi, MQTT, OTA update path.
3. [ ] Add quick rollback notes for failed release/update.

## Process & Release (P2)

1. [ ] Add a short PR workflow guide (development -> cherry-pick branch -> upstream PR).
2. [ ] Add branch naming convention for external PRs.
3. [ ] Add a release checklist document in repo docs.
4. [ ] Add a small script to prepare release notes draft.

#

# Done

## Build & Version

1. [x] Added support for `platformio_override.ini` local overrides.
2. [x] Added `wifi_flags` injection via `${extra.wifi_flags}`.
3. [x] Updated firmware version format support (example: `9.17-dev`).
4. [x] Updated code/version reporting to use string `VERSION`.
5. [x] Improved `extra_script.py` handling for quoted `VERSION` values.

## Quick Release Flow

1. Bump version in `platformio.ini`.
2. Build target environments.
3. Validate OTA and displayed firmware version.
4. Commit and push changes.
5. Open PR and release notes.
