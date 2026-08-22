# Changelog

All notable changes to this project are documented in this file.

## [9.189] - 2026-08-21

### Fixed
- **The Wi-Fi radio was being configured before it existed.** `setupWiFi()` opened
  with `WiFi.setSleep(false)` — the first Wi-Fi call in the function, so the driver
  was not initialised and the call did nothing. Modem sleep therefore stayed at its
  default, which is what a weak signal that slowly improves looks like. The boot log
  from a C6 said it out loud a few lines later: `reloadWiFiConfig()` calls
  `jw.disconnect()`, and that returned `ESP_ERR_WIFI_NOT_INIT` at 75 ms.
  `jw.enableSTA()` only sets a flag inside JustWifi, so it could not be relied on
  to bring the radio up. `WiFi.enableSTA(true)` now runs first, and the sleep
  setting is asserted again on connect, because JustWifi's disconnect drops STA and
  a reconnect can restore the default.

### Para testers
- **O sinal Wi-Fi fraco nos primeiros minutos, no OnOfre Rega.** A configuração do
  rádio estava a ser feita antes de o rádio existir, e a instrução que evita o modo
  de poupança não chegava a ser aplicada. Corrigido.
- Quem reportou isto: vale a pena reiniciar o equipamento depois de atualizar e ver
  se o sinal aparece logo bom, em vez de melhorar ao longo de minutos.

## [9.188] - 2026-08-21

### Changed
- Development baseline synchronized through upstream release 9.187.

### Fixed
- **ESP32-C6 Smart Bus pins can no longer be assigned to features.** GPIO6 and
  GPIO7 are the fixed SDA/SCL pins on this target and are removed from the
  configurable pin list. Stored actuator mappings are validated before setup so
  legacy mappings to reserved pins remain inactive instead of being driven.
- **Foreground Web requests can no longer starve behind feature loops.** A
  bounded handoff makes periodic actuator and sensor loops yield to a waiting
  request, while the WebUI retries short 409/BUSY responses instead of exposing
  transient lease contention to the user.
- **Web authentication no longer contends with live feature work.** API
  credentials are captured before feature tasks start and remain immutable for
  the boot. Credential edits use the controlled-restart path before the new
  snapshot becomes active.

### Validation
- `ESP8266_DEBUG`, `ESP32_DEBUG`, and the isolated
  `ESP32C6_IRRIGATION_DEBUG` build pass with the follow-up changes.
- On an ESP32-C6-DevKitM-1, captive provisioning, repeated two-tab WebUI
  navigation, name-only saving, actuator control, rejected duplicate mappings,
  and controlled pin remapping were exercised. The test exposed GPIO6/7 as
  fixed Smart Bus pins; the corrected firmware removes them from feature
  selection and rejects legacy stored mappings before actuator setup.
- One earlier uncaptured C6 stress run ended in a task-watchdog reset. It did not
  recur during the recorded two-tab run, but its task/backtrace was lost and the
  cause remains unconfirmed.

### Para testers
- **No OnOfre Rega, os pinos 6 e 7 deixaram de estar disponíveis** — são do barramento
  fixo e usá-los partia o equipamento. Configurações antigas que os usassem são
  recusadas.
- **O painel deixou de dar erros esporádicos** ao gravar ou comandar com dois
  separadores abertos.
- A password do painel só passa a valer depois de reiniciar, e o equipamento reinicia
  sozinho quando a mudas.


## [9.187] - 2026-08-21

### Changed
- Development baseline synchronized through upstream release 9.186.

### Fixed
- **Captive Wi-Fi repair preserves an installed template.** Browsers submit the
  hidden template field even when a configured device only needs new Wi-Fi
  details. That field is now ignored in recovery mode, so saving the network
  no longer fails or replaces the device's existing functions; first setup
  still validates an explicitly selected template.
- **A name-only save no longer restarts DHCP devices.** The configuration API
  reports the live DHCP lease for diagnostics, but the panel was sending that
  address back as a static-IP edit. DHCP saves now omit and ignore those static
  fields, so unrelated metadata changes do not restart the network.
- **Irrigation controls use the released backend routes.** The WebUI now calls
  `/irrigation-run` and `/irrigation-stop`. The v9.186 slash-path compatibility
  aliases remain available for already-open panels and use the same protected
  action handlers.
- **Automatic OTA no longer mistakes a lost connection for success.** The panel
  confirms an update from the reported result or a changed firmware version;
  an unavailable device remains unconfirmed and cannot produce a false green
  success message.
- **Live wiring edits are validated before hardware state changes.** `POST
  /config` validates the proposed final pin map, electrical roles and pin counts,
  then returns stable result codes for malformed requests, invalid pins, topology
  mismatches, conflicts and busy state. Changes that require hardware
  reinitialization make the affected feature inert and queue a controlled restart
  only after the HTTP response closes.
- **Sensor drivers now enforce their input topology.** Driver-specific pin counts
  and roles are checked during configuration, and runtime guards fail closed
  before malformed sensor input arrays can access a GPIO.
- **Feature configuration is serialized across execution contexts.** A
  non-blocking access lease coordinates the feature loops with Web, template,
  MQTT, irrigation and OTA operations; competing work reports busy or retries
  instead of racing live vectors and configuration strings.
- **Failed OTA attempts release their resources.** Manual uploads track ownership
  per request and abort/release on failure or disconnect. Automatic OTA restarts
  the Web server after failure or no update, and queues a device restart only
  after a successful replacement.
- **Cloud and Wi-Fi callbacks defer shared-state work to the main loop.** Cloud
  MQTT events and bounded complete commands are staged for main-loop handling,
  the watchdog ticker only signals pending work, and retained Cloud credentials
  plus Wi-Fi configuration use protected snapshots or deferred commits.
- **Unsafe configuration restore is disabled.** The old export omitted secrets,
  dropped sensors, regenerated actuator IDs and bypassed the new pin preflight.
  Export now identifies itself as a non-secret diagnostic snapshot until a
  complete, versioned and server-validated restore format is implemented.
- **Configuration files keep their previous known-good copy on write failure.**
  Device and irrigation JSON are written to a size-checked temporary file and
  then renamed over the target without deleting it first. Configuration,
  feature-creation, captive-provisioning and irrigation schedule save failures
  return HTTP 507 and restart into the previous stored configuration instead of
  reporting an undurable change as successful.
- **Garden valves cannot reopen from a stored runtime state.** Valve state is
  forced OFF during setup and serialized as OFF, while the live scheduler and
  wall button can still operate it after boot.
- **Replacing an irrigation schedule closes its old active valve first.** The
  previous code replaced the program list before stopping a running cycle, so a
  removed or reordered zone could lose its only reference while still open.
  Malformed whole-schedule payloads are now rejected before changing the active
  cycle or the in-memory program list.

### Validation
- Flashed the pre-v9.186-rebase `ESP8266_DEBUG` 9.186-dev build to the
  USB-powered test board and completed configured-device captive Wi-Fi repair.
  The POST was stored, the response was followed by a software restart, and the
  device returned to station mode.
- The WebUI retained both existing switch functions and their original pin map;
  Cloud connectivity recovered and both test switches operated ON and OFF.
- The rebased 9.187-dev head, live pin remapping, OTA recovery, ESP32 hardware,
  and C6 hardware remain open validation items and are not claimed by this bench
  test.

### Para testers
- **Alterações de configuração passaram a ser validadas antes de mexer no
  equipamento.** Uma configuração inválida é recusada em vez de deixar o
  equipamento num estado a meio.
- As gravações em disco passaram a ser atómicas: uma falha de energia a gravar já
  não deixa o ficheiro corrompido.
- Depois de arrancar, as válvulas de rega ficam sempre fechadas.


## [9.186] - 2026-08-20

### Fixed
- **The 9.184 fix did not reach anyone with the panel already open.** Renaming the
  action endpoints only helps a browser that reloaded the page: a firmware update
  does not reload an open tab, so a device on the new build was still being driven
  by the previous panel, calling `/irrigation/run` and `/irrigation/stop` — which
  the schedule handler kept claiming, deleting programs on a run and failing on a
  stop, exactly as before. Both old paths are now registered ahead of it and point
  at the right code. Verified in the simulator: all four paths answer 200, keep the
  programs, and stop clears the cycle.

### Para testers
- **O "Parar rega" e o "Regar agora" já funcionam mesmo sem recarregar a página.**
  Na 9.184 a correção só valia depois de recarregar o painel, e um separador aberto
  continuava a usar o antigo.
- Se ainda tiveres o painel aberto de antes, um recarregamento normal (F5) não faz
  mal, mas já não é obrigatório.

## [9.185] - 2026-08-20

### Added
- **An over-the-air update now shows what it is doing.** The device answered the
  request immediately and did the work in its main loop, so the panel knew nothing
  after asking: a failed update and a dead button looked identical, which is how it
  was reported. The device keeps the progress and, on failure, the reason from
  `getLastErrorString()`; `/config` reports it as `ota`, and the panel follows it
  with a bar and shows the error instead of staying silent. Losing the device
  mid-update is treated as the successful ending it usually is, since a device that
  finishes reboots.

### Changed
- The Telegram announcement now prefers a `### Para testers` section from the
  changelog entry when there is one. The channel is read by testers and the
  changelog is written in English for the repository; keeping the Portuguese
  summary in the same file means it cannot drift away from the entry it describes.

### Para testers
- **O botão de atualizar já mostra o que está a acontecer** — barra de progresso e,
  se falhar, a razão em vez de silêncio.
- As mensagens deste canal passam a ser escritas em português.

## [9.184] - 2026-08-20

### Fixed
- **"Regar agora" was deleting every program, and "Parar rega" always failed.**
  AsyncWebServer matches a plain URI as "exact, or prefix with a trailing slash",
  and the handler for `/irrigation` — the one that replaces the whole schedule — is
  registered before the actions, so it answered `/irrigation/run` and
  `/irrigation/stop` as well. A run request carries no `programs` key, so the
  schedule parsed as empty and was written to flash that way; a stop request
  carries no body at all, which is why the panel reported a failure. The actions
  moved to `/irrigation-run` and `/irrigation-stop`, which cannot be a prefix of
  the schedule path. Reproduced and verified both ways against the simulator.
- This also corrects the diagnosis behind the 9.182 note that said "Regar agora"
  merely *looked* like it deleted a program. It was deleting it, on the device.

### Notes
- The panel is embedded in the firmware, so both sides move together; nothing else
  called these endpoints yet.

### Para testers
- **O "Regar agora" apagava os programas em vez de os correr.** Corrigido: agora
  arranca o programa e o horário fica intacto.
- **O "Parar rega" dava sempre erro.** Corrigido.
- Se perdeste programas ao carregar em "Regar agora", tens de os criar outra vez —
  foram apagados no equipamento.

## [9.183] - 2026-08-20

### Fixed
- **A valve's wall button did nothing.** `Actuator::setup()` attaches button
  handlers under `if (isLight() || isSwitch())`, and a garden valve is neither —
  while the block right above it, which drives the output, does include valves. So
  commanding a valve worked and pressing its button did not, on a board whose
  template gives every valve a button and whose panel prints its pin. Valves now
  take a momentary press to toggle, matching the PUSH that `driverToInputMode()`
  already reported for the driver.

### Para testers
- **Os botões de parede das válvulas não faziam nada.** Já funcionam: um toque
  alterna a válvula.
- Atenção em bancada: sem sensor de chuva ligado, o equipamento lê "a chover" e os
  programas agendados não arrancam. Desliga "saltar o ciclo se estiver a chover"
  enquanto testas.

### Notes
- Two reports from the test group are still open and are not fixed here: "Regar
  agora" and "Parar rega" doing nothing. Renaming a feature on the same board
  worked, which proves authenticated JSON POSTs reach the device, so the fault is
  in the command path rather than the web server. Waiting on the HTTP status of a
  failing request to tell those apart.
- A rain sensor with nothing wired to it reads as raining (the input is pulled up
  and an open input is HIGH), and "skip the cycle when it rains" is on by default —
  so scheduled programs are skipped silently. Turn that option off while testing on
  a bench.

## [9.182] - 2026-08-20

### Fixed
- **The C6 brought its access point up and then answered 404 to everything.** The
  captive handler declared `canHandle()` non-const, which overrides the archived
  AsyncWebServer fork but not ESP32Async 3.x, where the method is const. The base
  returned false, and since `setupCaptivePortal()` resets the server and leaves
  that handler alone on the AP, the configuration page could not be reached at
  all. Both overloads are now present, the same way the PR before this one had to
  do for `isRequestHandlerTrivial()`.
- **Irrigation: editing a field no longer rebuilds the list it lives in.** Every
  change re-rendered the whole programs section, so the input being typed into was
  replaced — the focus jumped and a half-entered time snapped back. Only the total
  line and the minutes box now update in place.
- **Irrigation: "Regar agora" no longer looks like it deletes the program.** It
  reloaded the configuration afterwards, which threw away unsaved edits, and the
  confirmation resized the button so the second click could land on Remover
  beside it. Running now refuses while there are unsaved changes — the equipment
  runs its own copy by id, so a program it has never been told about cannot run —
  and the two buttons are no longer neighbours.

### Fixed
- Template changes on already-provisioned devices are now validated, queued for
  the main loop, saved, and followed by a controlled restart instead of reporting
  success while leaving the old template active. ESP32 feature readers are
  quiesced before their vectors are replaced.
- Captive-portal credentials are now accepted only from a non-cacheable POST
  submission, so Wi-Fi passwords no longer appear in the request URL. Invalid
  submissions return an error without changing the stored configuration. The
  custom async handler now explicitly parses POST bodies; the full flow was
  validated on ESP8266 from AP discovery through Wi-Fi reconnection.

### Changed
- Added a non-updating dependency audit runner that reports outdated PlatformIO
  packages per configured environment, continues after target-specific failures,
  and documents why available updates still require build and hardware validation.

## [9.181] - 2026-08-19

### Fixed
- **The C6 rebooted at the end of boot, so its access point never came up.**
  `setup()` created the features task with `xTaskCreatePinnedToCore(..., core 1)`,
  and the C6 is single-core: FreeRTOS asserts on `xCoreID < configNUMBER_OF_CORES`
  with assertions enabled, which panics and restarts the board. The task is now
  pinned only where there is a second core to pin it to. The C3 never hit this
  because its target defines `HAN_MODE`, which already took the unpinned branch —
  the condition was the product, not the chip.
  Anyone who installed 9.180 on a C6 should reinstall from `/flash/`.

### Added
- `ARDUINO_USB_MODE` / `ARDUINO_USB_CDC_ON_BOOT` on the C6, as the C3 already had:
  the DevKitC-1's only port is the native USB-Serial/JTAG, so without them `Serial`
  goes to UART0 pins and the board cannot explain itself over the port it is
  plugged in by.
- `ESP32C6_IRRIGATION_DEBUG`: the same image with the logs left in. The release env
  unflags `DEBUG_ONOFRE`, which is right for a product and useless for bringing a
  new board up.

## [9.180] - 2026-08-19

### Added
- **The irrigation controller is a released variant.** The ESP32-C6 build is in
  the release matrix, published as `ESP32-C6` and offered on `/flash/` as *OnOfre
  Rega*. The full-flash image comes from the build's own merged `.factory.bin`,
  which already puts the bootloader at 0x0 — where a C6 expects it, not 0x1000.

### Fixed
- **A C6 would have been offered an ESP32 image.** It reported `mcu: "ESP32"` and
  asked the generic update URL, because the C6 build also defines `ESP32`. Both are
  now variant-aware: it reports `ESP32-C6` and asks for that variant. Nothing was
  at risk in the field — no C6 had ever been published — but the first one to
  auto-update would have flashed firmware for another chip.

### Removed
- **Maker support**, as agreed. The 4 MB pin map and the `ESP32-MAKER-4MB` variant
  are gone from the firmware, and the cloud no longer accepts that variant: any
  device still reporting it stops being offered updates rather than receiving a
  wrong image.

### Notes
- Still not validated on hardware. Nothing has run a real irrigation cycle, and
  *OnOfre Rega* on `/flash/` now makes an unvalidated image installable — worth
  keeping to the test group until a board has watered something.

### Build
- The web panel minifiers are pinned project dependencies (`package.json` +
  `package-lock.json`, both now tracked — they were in `.gitignore`, which is why
  they only ever existed as global installs) instead of `npm install -g`, and
  `tools/html_converter.py` resolves them from `node_modules/.bin` before falling
  back to `PATH`. Their exact output is embedded in the firmware image, so the
  build must not depend on whatever version a machine happens to have; `npm ci`
  from an empty tree reproduces the generated headers byte for byte. Install with
  `npm ci`; a missing tool now says so and names the command. CI uses `npm ci`
  with the npm cache instead of a global install.
- Replaced `html-minifier` with the maintained fork `html-minifier-terser`. The
  original is unmaintained and carries a ReDoS advisory with no fix available,
  which is not something to pin permanently. It takes the same arguments and
  produces byte-identical output for this panel, so the embedded assets match the
  ones already published in 9.179; the old name is still accepted so an existing
  global install keeps working.

### Fixed
- Documented the reproducible ESP32-C6 build failure: the first C6 run after an
  ESP32 build dies in `arduino.py` with `Path(None)` because both platforms
  install as `espressif32` and share one directory, leaving the framework package
  unresolved until the failed run relinks it. Run it again, or isolate the tree
  with `PLATFORMIO_CORE_DIR`. It was previously noted as intermittent and
  uncharacterised.

## [9.179] - 2026-08-19

### Added
- **Irrigation runs on the board.** Up to eight programs, each with a start time,
  the weekdays it runs on, and the minutes to give each zone. The schedule lives
  in the equipment, not in the cloud: watering has to keep working when the
  internet is down, and a cycle half-run because MQTT dropped would leave one
  zone soaked and the rest dry. New in the panel as the REGA tab, which only
  appears on a board that actually has valves.
- **Endpoints:** `POST /irrigation` replaces the schedule (and answers with what
  the device kept, including zones it dropped), `POST /irrigation/run` forces a
  program now, `/irrigation/stop` closes the open zone — on GET as well as POST,
  because stopping the water is the one thing someone may need to do from a phone
  browser bar with a wet lawn and no app. `/config` carries the whole schedule
  plus what is running, so the panel gets it in one read.

### Rules the feature rests on
- Nothing is watered without a synced clock. A scheduler guessing the hour is
  worse than one that refuses to run, and the panel says so instead of looking
  broken.
- A cycle interrupted by a power cut is not resumed on the next boot. Coming back
  at three in the morning to finish a cycle nobody is watching is worse than
  skipping it.
- Rain is evaluated when the cycle starts, not per zone, so a shower halfway
  through does not leave half the garden watered.
- Closing the running valve — wall button, app, cloud, panel — ends the cycle
  rather than reopening the valve under the person who just closed it.
- A valve opened by a program answers to the program's timer, not to its own
  autoOff: the default 30 minutes would otherwise cut a longer zone short.
- Forcing a program by hand ignores its days, its on/off switch, the rain sensor
  and the clock. It is an explicit act.

### Notes
- Programs are stored in their own file, `/irrigation.json`. An update can never
  lose a working configuration because of a key it did not recognise, and a device
  coming from 9.178 simply starts with no programs.
- A program due while another is still running is skipped for that day rather than
  queued; two cycles cannot share the water anyway.
- Not yet validated on hardware: the ESP32-C6 irrigation build compiles and the
  API was exercised against the simulator, but no board has run a real cycle.

## [9.178] - 2026-08-19

### Added
- **The device has a clock.** NTP_SERVER and TZ_INFO had been in the build for a
  long time but were never applied — nothing called configTime(), so the only
  real timestamps came from a HAN meter's own clock over Modbus. It syncs when the
  network comes up, and reports whether it has the time at all: `/config` carries
  `clockSynced` and `clockNow`, and Diagnostics shows them, because a schedule
  that refuses to run must be able to say why.
- **Only one irrigation zone opens at a time.** The supply pressure will not feed
  two. The guard sits in `Actuator::changeState`, which every path goes through —
  wall button, MQTT, cloud, web panel — so it cannot be bypassed; anywhere else it
  would be advice rather than a rule.

### Notes
- Groundwork for schedules that run on the board rather than in the cloud, so a
  watering cycle survives an internet outage. The program model and the state
  machine that walks the zones come next; the cards come after those.
- Settled while designing: no watering without a synced clock, no resuming a
  cycle after a reboot, and rain is evaluated when a program starts rather than
  mid-cycle.
- Not yet exercised on hardware. This is logic that drives relays connected to
  water, so a bench test — five LEDs on the valve pins — should come before it
  reaches a real installation.
## [9.177] - 2026-08-19

### Added
- **An irrigation build for the ESP32-C6 (DevKitC-1).** The C6 exists only in
  Arduino 3.x / IDF 5.x, so it runs on its own platform and leaves the fleet's
  environments untouched. Wiring mirrors the ESPHome unit it replaces — valves on
  GPIO 4/5/2/10/11, buttons on 18-22, rain sensor on 3 — so a board can be
  reflashed without being rewired. The model topped out at four valves, so a
  fifth valve, three more inputs and a rain-sensor pin were added, and the
  irrigation template now builds five zones each bound to its own button.
  Per-zone watering time needed no new code: an actuator's autoOff already
  expires a running output and restarts on each turn-on.
- **Panel: reworked function editor and status layout**, per-function cards with
  count and category, and clearer system sections (#119).
- **Tools: `check_project.py`** for repeatable source and build checks (#121),
  and **`export_firmware.py`** to preserve and promote locally built binaries
  (#120).

### Fixed
- **Cover open/closed was inverted in the device panel.** The firmware counts how
  CLOSED a shutter is (OFF_OPEN=0, ON_CLOSE=100), but the panel printed that
  straight as "% aberto" and filled the slider with it, while the command path
  inverted — so a closed cover read "100% aberto" and what you saw was not what
  you set. The panel now converts once, in both directions.
- **Artefact names no longer break esptool 5.x.** Builds were named
  "Firmware_env_version - date"; esptool 5.x parses its CLI with click and reads
  the lone dash as an option, failing with "Path '-' does not exist". This would
  have broken any platform upgrade, not just the C6.

### Notes
- The C6 environment must be built on its own: PlatformIO cannot hold two
  packages named "espressif32" in one process, so pairing it with an ESP32
  environment fails inside the platform builder. CI is unaffected — one
  environment per job.
- C6 images are not published for OTA yet: the cloud's hardware allowlist and
  the web-flasher chip family still have to learn the new variant.

## [9.176] - 2026-08-17

### Added
- **The firmware section lists every published build for the board and links each
  one for download.** Getting the right `.bin` meant knowing the device's variant
  and hand-building a URL, and handing someone the wrong variant is the one
  mistake here that costs a USB recovery — so the list is filtered to the board's
  own MCU, newest first, with the running version marked. It reads the release
  server's existing `all-versions` and `download` routes; no backend change.

## [9.175] - 2026-08-17

### Fixed
- **`/auto-update` accepts GET**, like `/reboot`, `/load-defaults` and
  `/templates/change` already did. It was the only one registered for POST
  alone, so pasting it into a browser answered 404 — which is exactly what
  someone does when talking a user through an update remotely.

## [9.174] - 2026-08-17

### Fixed
- **A remote UPDATE command was often swallowed.** `isAutoUpdateRequested()`
  cleared the flag as it answered, and `loop()` called it twice per iteration:
  once through `checkInternalRoutines()`, which performs the update, and again as
  a guard deciding whether to skip normal work. When the MQTT command arrived
  after the first call — `loopWiFi()` yields in between, so the async callback
  lands there routinely — the guard consumed the request and the update never
  ran. Pressing the button appeared to do nothing at all. Asking is now
  non-destructive and only the code that performs the update takes the request.

## [9.173] - 2026-08-15

### Changed
- **Sensor cards are measured in accessory tiles.** The sensor grid now shares the
  tiles' column basis, so a card can be sized in whole tiles: a reading with
  units — temperature, humidity, light, distance — and a meter take three, while
  a door, window, motion or rain sensor is one word and keeps one.

### Fixed
- **Accessory tiles all share one height.** A name that wrapped to two lines or a
  cover's slider made its tile taller than its neighbours, leaving the grid
  ragged. Rows stretch to match and the pin line is anchored to the bottom edge,
  so every tile lines up regardless of what it holds.

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
