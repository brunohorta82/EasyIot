#include "Irrigation.h"
#include "ConfigOnofre.h"
#include "DeviceClock.h"
#include <LittleFS.h>
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif

extern ConfigOnofre config;
Irrigation irrigation;

namespace
{
  constexpr size_t maxPrograms{8};
  constexpr uint16_t maxZoneMinutes{240};

  Actuator *findZone(const char *uniqueId)
  {
    for (auto &a : config.actuatores)
    {
      if (a.isGardenValve() && strcmp(a.uniqueId, uniqueId) == 0)
        return &a;
    }
    return nullptr;
  }
}

void Irrigation::load()
{
  if (!LittleFS.exists(configFilenames::irrigation))
    return;
  File file = LittleFS.open(configFilenames::irrigation, "r");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error)
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s Irrigation file unreadable, starting with no programs." CR, tags::config);
#endif
    return;
  }
  JsonObject root = doc.as<JsonObject>();
  update(root);
}

void Irrigation::save()
{
  File file = LittleFS.open(configFilenames::irrigation, "w");
  if (!file)
    return;
  JsonDocument doc;
  doc["enabled"] = enabled;
  doc["skipOnRain"] = skipOnRain;
  JsonArray list = doc["programs"].to<JsonArray>();
  for (auto &p : programs)
  {
    JsonObject o = list.add<JsonObject>();
    o["id"] = p.id;
    o["enabled"] = p.enabled;
    o["startMinute"] = p.startMinute;
    o["weekdays"] = p.weekdays;
    JsonArray zs = o["zones"].to<JsonArray>();
    for (auto &z : p.zones)
    {
      JsonObject zo = zs.add<JsonObject>();
      zo["uniqueId"] = z.uniqueId;
      zo["minutes"] = z.minutes;
    }
  }
  serializeJson(doc, file);
  file.close();
}

bool Irrigation::update(JsonObject &root)
{
  // The payload replaces the schedule wholesale: the panel always sends every
  // program, and merging would make a removal indistinguishable from an omission.
  enabled = root["enabled"] | true;
  skipOnRain = root["skipOnRain"] | true;

  std::vector<IrrigationProgram> parsed;
  JsonArray list = root["programs"].as<JsonArray>();
  for (JsonObject o : list)
  {
    if (parsed.size() >= maxPrograms)
      break;
    IrrigationProgram p;
    p.id = o["id"] | (uint8_t)(parsed.size() + 1);
    p.enabled = o["enabled"] | true;
    p.startMinute = min(1439, max(0, (int)(o["startMinute"] | 0)));
    p.weekdays = (uint8_t)(o["weekdays"] | 0) & 0x7F;
    for (JsonObject zo : o["zones"].as<JsonArray>())
    {
      const char *id = zo["uniqueId"] | "";
      // A zone naming a feature this device does not have is dropped rather than
      // kept as a hole in the cycle: the panel would show a program that waters
      // something invisible.
      if (!findZone(id))
        continue;
      IrrigationZone z;
      strlcpy(z.uniqueId, id, sizeof(z.uniqueId));
      z.minutes = (uint16_t)min((int)maxZoneMinutes, max(1, (int)(zo["minutes"] | 1)));
      p.zones.push_back(z);
    }
    parsed.push_back(p);
  }
  programs = parsed;

  // A schedule change must not leave a zone open from the cycle it replaced.
  if (isRunning())
    stop();
  return true;
}

void Irrigation::json(JsonVariant &root)
{
  JsonVariant irr = root["irrigation"].to<JsonObject>();
  jsonBody(irr);
}

void Irrigation::jsonBody(JsonVariant &irr)
{
  irr["enabled"] = enabled;
  irr["skipOnRain"] = skipOnRain;
  const IrrigationProgram *p = running();
  if (p && runningZone < p->zones.size())
  {
    JsonObject run = irr["running"].to<JsonObject>();
    run["zone"] = p->zones[runningZone].uniqueId;
    run["programId"] = p->id;
    long left = (long)(zoneEndsAt - millis());
    run["secondsLeft"] = left > 0 ? left / 1000 : 0;
  }
  else
  {
    irr["running"] = nullptr;
  }
  JsonArray list = irr["programs"].to<JsonArray>();
  for (auto &prog : programs)
  {
    JsonObject o = list.add<JsonObject>();
    o["id"] = prog.id;
    o["enabled"] = prog.enabled;
    o["startMinute"] = prog.startMinute;
    o["weekdays"] = prog.weekdays;
    JsonArray zs = o["zones"].to<JsonArray>();
    for (auto &z : prog.zones)
    {
      JsonObject zo = zs.add<JsonObject>();
      zo["uniqueId"] = z.uniqueId;
      zo["minutes"] = z.minutes;
    }
  }
}

const IrrigationProgram *Irrigation::running() const
{
  if (runningProgram < 0 || (size_t)runningProgram >= programs.size())
    return nullptr;
  return &programs[runningProgram];
}

bool Irrigation::raining() const
{
  for (auto &s : config.sensors)
  {
    if (s.driver != SensorDriver::RAIN || s.state.isEmpty())
      continue;
    JsonDocument doc;
    if (deserializeJson(doc, s.state))
      continue;
    if (strcmp(doc["rain"] | "", Payloads::rainOnPayload) == 0)
      return true;
  }
  return false;
}

bool Irrigation::isRunningZone(const char *uniqueId) const
{
  const IrrigationProgram *p = running();
  if (!p || runningZone >= p->zones.size())
    return false;
  return strcmp(p->zones[runningZone].uniqueId, uniqueId) == 0;
}

void Irrigation::openZone()
{
  const IrrigationProgram *p = running();
  if (!p)
    return;
  // Walk past zones whose valve has since been removed; changeState below closes
  // whatever else is open, so the one-zone rule holds without repeating it here.
  while (runningZone < p->zones.size() && !findZone(p->zones[runningZone].uniqueId))
    runningZone++;
  if (runningZone >= p->zones.size())
  {
    runningProgram = -1;
    return;
  }
  const IrrigationZone &z = p->zones[runningZone];
  Actuator *valve = findZone(z.uniqueId);
  zoneEndsAt = millis() + (unsigned long)z.minutes * 60000ul;
#ifdef DEBUG_ONOFRE
  Log.notice("%s Irrigation: %s for %d min." CR, tags::actuatores, valve->name, z.minutes);
#endif
  valve->changeState(StateOrigin::INTERNAL, ActuatorState::ON_CLOSE);
}

void Irrigation::closeCurrentZone()
{
  const IrrigationProgram *p = running();
  if (!p || runningZone >= p->zones.size())
    return;
  Actuator *valve = findZone(p->zones[runningZone].uniqueId);
  if (valve && valve->state == ActuatorState::ON_CLOSE)
    valve->changeState(StateOrigin::INTERNAL, ActuatorState::OFF_OPEN);
}

void Irrigation::advance()
{
  closeCurrentZone();
  runningZone++;
  const IrrigationProgram *p = running();
  if (!p || runningZone >= p->zones.size())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Irrigation cycle finished." CR, tags::actuatores);
#endif
    runningProgram = -1;
    return;
  }
  openZone();
}

void Irrigation::stop()
{
  closeCurrentZone();
  runningProgram = -1;
  runningZone = 0;
}

bool Irrigation::runProgram(uint8_t programId)
{
  for (size_t i = 0; i < programs.size(); i++)
  {
    if (programs[i].id != programId)
      continue;
    if (programs[i].zones.empty())
      return false;
    // Forcing a program by hand is an explicit act: it ignores the program's
    // days, its own on/off switch, and the rain sensor. It does not need a clock
    // either — the zone timer is a stopwatch, not a calendar.
    if (isRunning())
      stop();
    runningProgram = (int)i;
    runningZone = 0;
    programs[i].lastRunDay = clockWeekday();
    programs[i].lastRunMinute = clockMinuteOfDay();
    openZone();
    return isRunning();
  }
  return false;
}

void Irrigation::loop()
{
  if (isRunning())
  {
    // Someone may have closed the running valve from a wall button, the app or
    // the cloud. That is a takeover, not a pause: the cycle ends rather than
    // silently reopening the valve under the person who just closed it.
    const IrrigationProgram *p = running();
    Actuator *valve = p && runningZone < p->zones.size() ? findZone(p->zones[runningZone].uniqueId) : nullptr;
    if (!valve || valve->state != ActuatorState::ON_CLOSE)
    {
#ifdef DEBUG_ONOFRE
      Log.notice("%s Irrigation cycle cancelled: zone was closed elsewhere." CR, tags::actuatores);
#endif
      runningProgram = -1;
      runningZone = 0;
      return;
    }
    if ((long)(millis() - zoneEndsAt) >= 0)
      advance();
    return;
  }

  if (!enabled)
    return;
  // No clock, no schedule. A device that has just booted without NTP must not
  // guess: watering at the wrong hour wastes water and, with a timer valve, can
  // run all night.
  if (!clockSynced())
    return;

  const int minute = clockMinuteOfDay();
  const int weekday = clockWeekday();
  if (minute < 0 || weekday < 0)
    return;

  for (size_t i = 0; i < programs.size(); i++)
  {
    IrrigationProgram &p = programs[i];
    if (!p.enabled || p.zones.empty() || p.startMinute != (uint16_t)minute || !p.runsOn(weekday))
      continue;
    // One start per due minute, or a cycle that ends inside its own start minute
    // would begin again immediately.
    if (p.lastRunDay == weekday && p.lastRunMinute == minute)
      continue;
    p.lastRunDay = weekday;
    p.lastRunMinute = minute;
    if (skipOnRain && raining())
    {
#ifdef DEBUG_ONOFRE
      Log.notice("%s Irrigation skipped: raining." CR, tags::actuatores);
#endif
      continue;
    }
    runningProgram = (int)i;
    runningZone = 0;
    openZone();
    return;
  }
}
