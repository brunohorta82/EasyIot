#ifndef DEVICE_LOG_H
#define DEVICE_LOG_H

#include <Arduino.h>

/**
 * A short operational log the device keeps for itself.
 *
 * Everything written with ArduinoLog goes to Serial and is compiled out of release
 * builds, so diagnosing what a tester reports has meant asking them for a USB
 * cable — and by then the boot they were describing is long gone. This is the
 * opposite trade: a handful of lines that always exist, kept in RAM, readable from
 * the panel over the network.
 *
 * It is deliberately small and deliberately not the debug log. Only moments worth
 * a line hours later belong here: boot, the network coming and going, the cloud,
 * an update, a watering cycle, a configuration refused. A per-event log would fill
 * the buffer before anyone read it.
 *
 * RAM is the constraint that shapes it: entries are fixed-size. ESP32 keeps the
 * storage static; ESP8266 allocates it once at boot so OTA can explicitly reclaim
 * those bytes before BearSSL, then recreate an empty history after a failed update.
 */

#ifdef ESP8266
constexpr size_t kDeviceLogLines{40};
#else
constexpr size_t kDeviceLogLines{80};
#endif
constexpr size_t kDeviceLogLineSize{88};

/** Appends one line, printf-style. Truncates rather than allocating. */
void deviceLog(const char *format, ...);

/** The whole buffer, oldest first, one entry per line, ready to be copied. */
String deviceLogText();

/** Uptime-stamped so the reader can tell a boot-time line from a later one. */
void deviceLogClear();

/** Releases the ESP8266 history storage before a memory-heavy OTA handshake.
 *  The next deviceLog/deviceLogText call recreates an empty buffer if the
 *  running firmware returns after a failed or unnecessary update. */
void deviceLogReleaseForUpdate();

#endif
