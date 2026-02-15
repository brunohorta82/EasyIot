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

1. [ ] (Blocked) Remove temporary CloudIO HTTP fallback after full TLS compatibility is confirmed on devices (including weak-signal scenarios). File: `src/CloudIO.cpp`
2. [ ] (Blocked - no boards available) Validate OTA update flow over HTTPS on remaining device variants (ESP32 / ESP32C3 / HAN). File: `src/WebServer.cpp`

## Webpanel UX (P1/P2)

1. [ ] Remove hardcoded `baseUrl` and use same-origin requests. File: `webpanel/js/index.js`
2. [ ] Add automatic version banner in webpanel footer.
3. [ ] Add firmware build date in API/system info payload.
4. [ ] Avoid external hard dependency for core UI assets (icons/fonts) by hosting fallback assets locally. Files: `webpanel/index.html`, `webpanel/js/index.js`

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

## Quick Release Flow

1. Bump version in `platformio.ini`.
2. Build target environments.
3. Validate OTA and displayed firmware version.
4. Commit and push changes.
5. Open PR and release notes.
