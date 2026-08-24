#ifndef IRRIGATION_H
#define IRRIGATION_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * Scheduled irrigation, running on the device.
 *
 * The schedule deliberately does not live in the cloud. Watering has to keep
 * working while the internet is down, and a cycle half-run because MQTT dropped
 * would leave one zone soaked and the rest dry. The panel and the app only edit
 * these programs; the equipment is what runs them.
 *
 * Rules that the whole feature rests on:
 *  - nothing is watered without a synced clock. A scheduler guessing the time is
 *    worse than one that refuses to run;
 *  - no more than maxConcurrentZones valves are open at once, because that is
 *    what the supply pressure feeds. How many is a property of the installation,
 *    not of the firmware, so it is configurable (1-5) and defaults to one;
 *  - a cycle interrupted by a power cut is not resumed on the next boot. Coming
 *    back at three in the morning to finish a cycle nobody is watching is worse
 *    than skipping it;
 *  - rain is evaluated when the cycle starts, not per zone, so a shower halfway
 *    through does not leave half the garden watered;
 *  - closing any open zone by hand cancels the cycle rather than pausing it.
 *    Someone shutting a valve wants the watering to stop, and with several zones
 *    open it would otherwise be unclear which ones survived the interruption.
 */

constexpr uint8_t kMaxConcurrentZones{5};

struct IrrigationZone
{
  char uniqueId[24]{};
  uint16_t minutes{0};
};

struct IrrigationProgram
{
  uint8_t id{0};
  bool enabled{true};
  uint16_t startMinute{0}; // minutes since local midnight
  uint8_t weekdays{0};     // bit 0 = Sunday … bit 6 = Saturday
  std::vector<IrrigationZone> zones;

  // Runtime only, never persisted: which day/minute this program last started,
  // so a cycle that ends inside its own start minute does not begin again.
  int lastRunDay{-1};
  int lastRunMinute{-1};

  bool runsOn(int weekday) const
  {
    return weekday >= 0 && weekday <= 6 && (weekdays & (1 << weekday));
  }
};

class Irrigation
{
public:
  bool enabled{true};
  bool skipOnRain{true};
  /** How many zones may water at the same time, 1..kMaxConcurrentZones. One is
      the safe default: a pump or a mains feed that cannot supply two zones will
      simply water both badly, and there is no way to detect that from here. */
  uint8_t maxConcurrentZones{1};
  std::vector<IrrigationProgram> programs;

  /** Reads /irrigation.json. A device with no such file simply has no programs. */
  void load();
  bool save();

  /** Replaces the whole in-memory schedule; the caller persists it after validation. */
  bool update(JsonObject &root);

  /** Serialises state + schedule under the "irrigation" key, for /config. */
  void json(JsonVariant &root);

  /** The same object, written straight into root: what the endpoints return. */
  void jsonBody(JsonVariant &irr);

  /** Advances the cycle. Cheap enough to call from the main loop. */
  void loop();

  /** Starts a program by id, ignoring its days, its on/off switch, the rain
      sensor and the clock — forcing it is an explicit act. False if it has no
      zone to water. */
  bool runProgram(uint8_t programId);

  /** Closes every open zone and forgets the cycle. */
  void stop();

  bool isRunning() const { return runningProgram >= 0; }

  /** True while a scheduled cycle is the reason this valve is open. */
  bool isRunningZone(const char *uniqueId) const;

  /** Seconds left and the length this zone was given, for a valve the cycle is
      watering. False when the cycle is not the reason it is open. Lets a panel
      draw a countdown from the device's own clock instead of guessing. */
  bool zoneCountdown(const char *uniqueId, unsigned long &left, unsigned long &total) const;

  /** The cap the valve interlock enforces, clamped whatever the stored value.
      Read from Actuator::changeState, which is the one place every command to a
      valve passes through, so the rule cannot be bypassed. */
  uint8_t openZoneLimit() const;

private:
  /** One zone of the cycle that is watering right now. Each carries its own
      deadline: zones in the same program rarely run for the same length. */
  struct ActiveZone
  {
    size_t index;                // into the running program's zones
    unsigned long endsAt;        // millis()
    unsigned long totalSeconds;  // what it was given, for a progress reading
  };

  int runningProgram{-1};       // index into programs, not the id
  std::vector<ActiveZone> active;
  size_t nextZone{0};           // next zone of the cycle waiting for a free slot
  int lastStartedMinute{-1};    // guards against starting twice in the same minute
  int lastStartedDay{-1};

  /** Opens zones until the concurrency limit is reached or the cycle runs out.
      Ends the cycle when there is nothing left open and nothing left to open. */
  void startPendingZones();
  void closeActiveZones();
  void clearRuntime();
  unsigned long secondsLeft(const ActiveZone &slot) const;
  bool raining() const;
  const IrrigationProgram *running() const;
};

extern Irrigation irrigation;

#endif
