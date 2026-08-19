#ifndef DEVICE_CLOCK_H
#define DEVICE_CLOCK_H

#include <Arduino.h>

/**
 * Wall-clock time for the device.
 *
 * NTP_SERVER and TZ_INFO have been defined in platformio.ini for a long time but
 * were never used: nothing in the firmware ever called configTime(), so the only
 * real timestamps came from a HAN meter's own clock over Modbus. Anything
 * scheduled on the device needs a clock of its own, and needs to know when it
 * does NOT have one — a scheduler guessing the time is worse than one that
 * refuses to run.
 */
void setupDeviceClock();

/** True once NTP has returned a plausible date (i.e. not the 1970 epoch). */
bool clockSynced();

/** Minutes since local midnight, or -1 while unsynced. */
int clockMinuteOfDay();

/** 0 = Sunday … 6 = Saturday, or -1 while unsynced. */
int clockWeekday();

/** ISO-8601 local time for diagnostics, or an empty string while unsynced. */
String clockNowIso();

#endif
