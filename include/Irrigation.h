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
 *  - only one zone is ever open, because the water pressure does not feed two;
 *  - a cycle interrupted by a power cut is not resumed on the next boot. Coming
 *    back at three in the morning to finish a cycle nobody is watching is worse
 *    than skipping it;
 *  - rain is evaluated when the cycle starts, not per zone, so a shower halfway
 *    through does not leave half the garden watered.
 */

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
  std::vector<IrrigationProgram> programs;

  /** Reads /irrigation.json. A device with no such file simply has no programs. */
  void load();
  void save();

  /** Replaces the whole schedule from the panel/app payload and persists it. */
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

  /** Closes the open zone and forgets the cycle. */
  void stop();

  bool isRunning() const { return runningProgram >= 0; }

  /** True while a scheduled cycle is the reason this valve is open. */
  bool isRunningZone(const char *uniqueId) const;

private:
  int runningProgram{-1};   // index into programs, not the id
  size_t runningZone{0};    // index into that program's zones
  unsigned long zoneEndsAt{0};
  int lastStartedMinute{-1}; // guards against starting twice in the same minute
  int lastStartedDay{-1};

  void openZone();
  void closeCurrentZone();
  void advance();
  bool raining() const;
  const IrrigationProgram *running() const;
};

extern Irrigation irrigation;

#endif
