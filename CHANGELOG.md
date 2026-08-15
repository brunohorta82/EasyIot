# Changelog

All notable changes to this project are documented in this file.

## [9.163] - 2026-08-15

### Fixed
- **A silent HAN meter no longer resets the device.** Three defects on the meter
  read path combined into a reboot loop whenever a meter stopped answering,
  which matches the MQTT reconnect flapping seen on HAN units:
  - the clock read filled a six-word array with an `i <= available()` loop,
    writing one element past the end and corrupting the stack
    (`std::array::operator[]` does not bounds-check, so it failed silently);
  - `setError()` was commented out in the first read's error branch, so a meter
    that stopped answering was polled another seven times instead of once, each
    blocking for the full 2 s Modbus timeout;
  - that timeout was a tight spin with no yield, holding the CPU outright — past
    the ESP8266 software watchdog at ~3.2 s, resetting the device and dropping
    its MQTT session.

## [9.162] - 2026-08-15

### Changed
- Web panel API: state-changing routes (`/reboot`, `/load-defaults`,
  `/templates/change`) moved from `GET` to `POST`, with temporary `GET`
  compatibility kept for older clients (#100).
- Web panel now calls its own origin instead of a hardcoded API base URL, so it
  works on whatever address the device is reached at (#93).
- CloudIO config sync: per-attempt timeouts and HTTPS retries with backoff before
  the one-time plain-HTTP fallback, which is still in place for devices whose TLS
  handshake fails (#105).
- Firmware build date exposed in the config/system API payload (#104).
- `WEB_SECURE_ON` enforced on the remaining release profiles, so the on-device web
  panel always requires its credentials (#99).

### Fixed
- Cleared config/sensor compiler warnings: deprecated ArduinoJson `containsKey()`
  replaced with `isNull()` guards, and explicit no-op cases added for ESP32-only
  drivers on ESP8266 (#94).

### Build
- Release builds also produce full-flash images for the browser installer at
  `/flash/` (ESP8266 as-is, ESP32 merged with bootloader/partitions/boot_app0).
- Cross-platform build tooling: the HTML converter and release validator are now
  Python, so builds no longer depend on a bash/macOS-specific toolchain (#108).

### Notes
- Serial-console improvements (boot banner, reset reason, cleaner logs) ship in
  #107 but are compiled out of release builds, which unflag `DEBUG_ONOFRE`.

## [9.161] - 2026-08-14

### Fixed
- **CloudIO watchdog no longer stops permanently on HTTP 204.** When the
  `/devices/config` sync returned 204 (device not adopted — or a transient
  server-side condition such as a deploy or database pressure), the firmware
  called `stopCloudIOWatchdog()` and never retried the cloud connection again:
  the device stayed connected to the local broker (Home Assistant kept working)
  but disappeared from CloudIO until a manual power cycle or a WiFi drop.
  Affected mainly HAN/PZEM energy meters, whose frequent MQTT reconnects made
  them far more likely to hit a bad window. The watchdog now backs off for
  ~30 minutes after a 204 and then asks again, forever. A successful sync
  clears the backoff.
- **Firmware only built on case-insensitive filesystems.** `src/Sensors.cpp`
  included `<SensirionI2CSht4x.h>` while the library ships
  `SensirionI2cSht4x.h`, so every Linux build (including CI) failed. The SHT4x
  dependency is now pinned instead of tracking upstream HEAD.

### Build
- CI builds all four release environments and publishes them to the OTA folder.

## [9.17] - 2026-02-14

### Release
- Promoted stable release from `9.17-dev`.
- Includes CloudIO/webpanel/build-flow improvements from the 9.17 development cycle.
- Added `## [9.17]` release section to satisfy strict release validation checks.
- Verified all configured PlatformIO environments build successfully for the 9.17 release snapshot.

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
- Validated OTA update flow over HTTPS on ESP8266 (`Update Success`) with successful reconnect to CloudIO/MQTT after reboot.

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

## Baseline - before 9.17-dev

- Snapshot before the `9.17-dev` maintenance and build-flow updates. Labelled
  `9.163` at the time, renamed here so it cannot be mistaken for the released
  9.163 above.
