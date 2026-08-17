# Changelog

All notable changes to this project are documented in this file.

## [9.172] - 2026-08-15

### Changed
- **Accessories are tiles, not full-width rows.** A switch is a small thing and
  a row per accessory wasted most of the screen; a grid shows a whole
  installation at once, with an icon per type, its state in words, and a green
  tint when on. Each tile carries a cog that jumps straight to that feature's
  settings, highlighted on arrival, instead of hunting a long list.
- **The meter card shows contracted power beside each period's reading**, the
  way the meter's own panel presents them — the pair only means something read
  together. The tariff being billed keeps its traffic light.

## [9.171] - 2026-08-15

### Changed
- **The meter card was rebuilt around what a meter actually reads.** Import,
  meter health and export now lead as three figures side by side, the tariff
  periods carry a traffic light so the one being billed is findable at a glance,
  and the raw instantaneous values follow. It was a flat key/value list before,
  which buried exactly the numbers people open the panel for. A meter card is
  also given two columns: readings carry units and were wrapping mid-value.

### Fixed
- **Binary sensors say what they mean.** A door read "ativo" instead of open or
  closed, and motion and rain showed nothing at all because their payloads were
  never handled. Each driver now gets its own words, matching the values the
  firmware publishes.
- **Cloud status distinguishes three states** — no credentials (never adopted),
  adopted but disconnected, and connected. It previously reported only whether
  credentials existed, so "não configurada" on a working device sent people
  looking for a problem that was not there.

## [9.170] - 2026-08-15

### Added
- **Climate sensors now have compact live history graphs in Overview.**
  DS18B20, DHT11/21/22, and SHT4x readings keep a bounded browser-only history
  with separate temperature and humidity scales, min/max or stable-state
  context, sample timestamps, and a visible newest-reading marker. History
  resets when the page is reloaded and never writes samples to device flash.

## [9.169] - 2026-08-15

### Added
- **The panel announces an available update and offers it in one click**, naming
  the version, as the old panel did. The device only knows its own version, so
  the panel asks the release server; without this nobody discovers that a fix
  exists. Versions are compared component by component as numbers — string
  comparison ranks 9.99 above 9.100.

### Fixed
- **HAN demand-control values reach Home Assistant again.** The meter's registers
  are read as a chain of nine guarded steps, and 9.163 re-enabled `setError()` on
  a failed read to stop a silent meter from resetting the device. That also meant
  a single failed register aborted every later step, so `demandControlT1..T3` —
  the eighth read — silently vanished from the payload and the Home Assistant
  templates found nothing. A miss now costs only its own field; two misses in a
  row still end the pass and pause polling, which is what protects the device.

### Changed
- **Binary actuators can now be controlled from their complete Overview row.**
  The row is a keyboard-accessible pressed button instead of limiting control
  to the small switch graphic, with a green left accent when on and a neutral
  accent when off.
- **The firmware version is visible again in the persistent header.** Device
  name, chip ID, board type, and the running firmware version now stay together
  so the installed build can be identified without opening Diagnostics.

### Fixed
- **Unsaved feature edits now survive Diagnostics refreshes.** The periodic
  diagnostics update no longer replaces locally edited feature data with the
  last configuration stored on the device.
- **Live feature events stay attached after configuration changes.** Event
  listeners are removed and rebound to the current features after loading,
  adding, or saving configuration, so new names and features update without a
  browser reload.

## [9.168] - 2026-08-15

### Fixed
- **Saving worked again.** Moving the save button into the header left it
  disabled with nothing to enable it, so pressing it did nothing at all — a
  changed device or feature name was simply lost. markDirty()/clearDirty() now
  drive the button, and it reports progress on itself instead of writing to a
  status element that the same change had deleted; that write sat outside the
  try block, so it threw before the request was ever sent and left the button
  stuck disabled with no way to retry.

## [9.167] - 2026-08-15

### Added
- **A PZEM v3's accumulated energy can be reset from the panel**, so the counter
  no longer needs the button on the meter itself. `POST /sensors/reset-energy
  {id}` queues the request rather than performing it: the meter sits on a serial
  bus the sensor's loop owns, and reaching for it from the web handler's context
  would corrupt a reading in flight. The reset runs on the next poll and the
  counter reads zero from that reading onwards.

  Only PZEM v3 offers the command, so the endpoint is gated and the panel draws
  the button only on those cards. The HAN is the utility's meter and has no such
  command — asking for one returns 404.

## [9.166] - 2026-08-15

### Added
- **Light and dark themes**, remembered per browser. The panel was dark-only,
  which is unreadable on a phone in daylight next to a distribution board. It
  follows the system preference until told otherwise, and the choice is applied
  before the first paint so the page never flashes the wrong theme. It lives in
  the browser rather than device config: it belongs to whoever is looking, and
  storing it on the device would burn flash for a preference the next browser
  would not share.

### Fixed
- **Manual firmware upload is back.** The rebuilt panel dropped the file upload
  to `POST /update`, which is the recovery path when the cloud cannot be
  reached — the situation the on-device panel exists for. It returns with
  upload progress, the device's own variant named in the warning, and a
  magic-byte check: both ESP8266 and ESP32 images start with `0xE9`, and
  refusing anything else saves a USB recovery after an accidental pick.
- **Configuration restore is back**, alongside export. The exported file again
  carries `backup: true`, which is what makes `POST /config` rebuild features
  instead of editing them — without it an exported file did not restore
  properly. A restore is refused if the file belongs to a different chip.
- **Input mode is editable again** (momentary button vs latching switch). The
  firmware derives an actuator's driver from it on every save, so it decides
  how a wall switch behaves on identical wiring.

### Build
- **The embedded panel is always regenerated.** `run_html_converter` hung off a
  PreAction on `$PROGPATH`, which only fires when a link happens. Changing
  `webpanel/` alone left nothing to recompile, so the link was skipped, the
  converter never ran, and the build reported success while embedding the
  previous panel. CI was never affected — a fresh checkout always relinks — but
  a local build could quietly produce firmware whose panel did not match source.

## [9.165] - 2026-08-15

### Added
- **The on-device web panel was rebuilt.** It is what people reach for when the
  cloud is unreachable, which is exactly when it mattered least before. Five
  tabs — overview, pinout, features, diagnostics, system. The pinout is drawn
  from the board's own pin list, so it never offers a GPIO the firmware would
  refuse, and it names the feature holding each one.
- **Energy meters get a card of their own.** A HAN publishes around twenty
  fields and the panel showed only instantaneous power, dropping voltage,
  tariff and the export reading. Exporting to the grid now reads negative and
  green, matching the cloud panel.
- **Feature wiring is editable.** `ConfigOnofre::update()` re-maps a pin only
  when it is valid for the board and owned by no other feature, and parks the
  pins a feature gives up so no relay stays latched on a GPIO nothing drives.
- **Diagnostics on the local API** — free heap, fragmentation, largest free
  block, uptime, reset reason, sketch size and the board's usable pins.

### Fixed
- **Creating a feature on a pin already in use is refused.** `prepareNewFeature`
  checked only that a pin exists on the board, never that it was free, so a
  second feature could be created on top of the first and both drivers ended up
  on one GPIO. Sensors skipped validation entirely — a bogus pin was accepted
  and surfaced only as a driver that never read anything. Both paths validate
  now and return the new code 5 for a pin already in use.
- **Destructive buttons no longer depend on `confirm()`**, which sandboxed
  frames block silently — the template buttons looked dead — and which is easy
  to misfire on a phone. They ask for a second click on the button itself.

### Notes
- Pin arrays stay optional on `POST /config`: a payload without them leaves the
  wiring untouched, so v9 panels, the mobile apps and restored backups behave
  exactly as before. Updates remain non-destructive to stored configuration.

## [9.164] - 2026-08-15

### Fixed
- **MQTT callbacks no longer read past a payload or reboot on a failed allocation.**
  The CloudIO handler built a variable-length array on an async callback's stack
  from the broker-supplied length (up to MQTT_MAX_PACKET_SIZE) and filled it with
  `strlcpy`, which walks the source looking for a terminator a raw payload need not
  have. It now uses a fixed buffer and copies exactly the bytes received; payloads
  split across packets are ignored rather than acted on as fragments. The local
  broker handler wrote into whatever `malloc` returned — on a fragmented heap that
  is null, and the device rebooted instead of dropping one message.
- **Sensor polling no longer outranks the network stack on ESP32.** The task asked
  for priority 100; FreeRTOS clamps that to the top of the range, which placed it
  above the WiFi and TCP/IP tasks. It now runs at 2, below them.

### Build
- Registry dependencies are pinned to the versions in use (ArduinoJson 7.4.3,
  PubSubClient 2.8, AsyncMqttClient 0.9.0, DallasTemperature 4.0.6, PZEM004T 1.1.5).
  Twelve of eighteen dependencies previously floated, which is how a renamed header
  in an unpinned SHT4x broke every Linux build. The remaining git URLs still track
  their default branch.

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
