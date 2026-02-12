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
2. [ ] Add `CHANGELOG.md` with version-by-version entries.

## Security & OTA (P1)

1. [ ] Move cloud config/OTA URLs from `http://` to secure transport and validate update path. File: `include/Constants.h`
2. [ ] Convert state-changing endpoints from GET to POST (`/reboot`, `/load-defaults`, `/templates/change`). File: `src/WebServer.cpp`
3. [ ] Ensure release profiles enforce `WEB_SECURE_ON` and avoid debug defaults in production builds. File: `platformio.ini`
4. [ ] Replace default credentials (`admin` / `xpto` / default AP secret) with first-boot forced change flow. File: `include/Constants.h`
5. [ ] Change captive portal save flow from GET query params to POST body (avoid leaking passwords in URL/history). Files: `include/CaptivePortal.h`, `src/WebServer.cpp`
6. [ ] Add OTA integrity check (signed firmware or hash validation) before applying update. File: `src/WebServer.cpp`

## Dependencies & Library Updates (P1/P2)

1. [ ] Pin PlatformIO platforms to known-good versions (`espressif32@...`, `espressif8266@...`) for reproducible builds. File: `platformio.ini`
2. [ ] Pin GitHub-based `lib_deps` to tags/commits (avoid floating `master/main`). File: `platformio.ini`
3. [ ] Add periodic dependency audit task (`pio pkg outdated` + compatibility notes per env).
4. [ ] Create dependency update policy doc (how to test + rollback by env).
5. [ ] Normalize `lib_deps` formatting and identifiers (remove ambiguous specs / spacing inconsistencies). File: `platformio.ini`

## Webpanel UX (P1/P2)

1. [ ] Add automatic version banner in webpanel footer.
2. [ ] Add firmware build date in API/system info payload.
3. [ ] Avoid external hard dependency for core UI assets (icons/fonts) by hosting fallback assets locally. Files: `webpanel/index.html`, `webpanel/js/index.js`

## Testing & CI (P2)

1. [ ] Add CI build checks for main envs (ESP8266 + ESP32).
2. [ ] Add smoke tests for boot, Wi-Fi, MQTT, OTA update path.
3. [ ] Add quick rollback notes for failed release/update.
4. [ ] Add CI checks for formatting/sanity of `platformio.ini` (flags, env inheritance, unsafe defaults).

## Process & Release (P2)

1. [ ] Add a short PR workflow guide (development -> cherry-pick branch -> upstream PR).
2. [ ] Add branch naming convention for external PRs.
3. [ ] Add a release checklist document in repo docs.
4. [ ] Add a small script to prepare release notes draft.
5. [ ] Add pre-release checklist item for credential and transport-security review.

#

# Done

## Build & Version

1. [x] Added support for `platformio_override.ini` local overrides.
2. [x] Added `wifi_flags` injection via `${extra.wifi_flags}`.
3. [x] Updated firmware version format support (example: `9.17-dev`).
4. [x] Updated code/version reporting to use string `VERSION`.
5. [x] Improved `extra_script.py` handling for quoted `VERSION` values.

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
