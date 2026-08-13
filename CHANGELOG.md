# Changelog

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
