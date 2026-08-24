#include "Irrigation.h"
#include "ConfigOnofre.h"
#include "DeviceClock.h"
#include "DeviceLog.h"
#include "Persistence.h"
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
      if (a.ready && a.isGardenValve() && strcmp(a.uniqueId, uniqueId) == 0)
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

bool Irrigation::save()
{
  JsonDocument doc;
  doc["enabled"] = enabled;
  doc["skipOnRain"] = skipOnRain;
  doc["maxConcurrentZones"] = maxConcurrentZones;
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
  return persistJsonAtomically(configFilenames::irrigation,
                               configFilenames::irrigationTemporary, doc);
}

bool Irrigation::update(JsonObject &root)
{
  // The payload replaces the schedule wholesale: the panel always sends every
  // program, and merging would make a removal indistinguishable from an omission.
  JsonVariantConst enabledValue = root["enabled"];
  JsonVariantConst skipOnRainValue = root["skipOnRain"];
  JsonVariantConst programsValue = root["programs"];
  if (!enabledValue.is<bool>() || !skipOnRainValue.is<bool>() ||
      !programsValue.is<JsonArrayConst>())
    return false;

  // Absent means "leave it as it is", not "back to one": the file written by a
  // firmware from before this setting existed must not silently reconfigure the
  // installation, and neither must an older panel or app that omits the field.
  JsonVariantConst maxZonesValue = root["maxConcurrentZones"];
  uint8_t parsedMaxZones = maxConcurrentZones;
  if (!maxZonesValue.isNull())
  {
    if (!maxZonesValue.is<unsigned int>())
      return false;
    const unsigned int wanted = maxZonesValue.as<unsigned int>();
    if (wanted < 1u || wanted > kMaxConcurrentZones)
      return false;
    parsedMaxZones = static_cast<uint8_t>(wanted);
  }

  JsonArrayConst list = programsValue.as<JsonArrayConst>();
  if (list.size() > maxPrograms)
    return false;

  std::vector<IrrigationProgram> parsed;
  for (JsonVariantConst programValue : list)
  {
    if (!programValue.is<JsonObjectConst>())
      return false;
    JsonObjectConst o = programValue.as<JsonObjectConst>();

    JsonVariantConst idValue = o["id"];
    JsonVariantConst programEnabledValue = o["enabled"];
    JsonVariantConst startMinuteValue = o["startMinute"];
    JsonVariantConst weekdaysValue = o["weekdays"];
    JsonVariantConst zonesValue = o["zones"];
    if (!idValue.is<unsigned int>() ||
        !programEnabledValue.is<bool>() ||
        !startMinuteValue.is<unsigned int>() ||
        !weekdaysValue.is<unsigned int>() ||
        !zonesValue.is<JsonArrayConst>())
      return false;

    const unsigned int id = idValue.as<unsigned int>();
    const unsigned int startMinute = startMinuteValue.as<unsigned int>();
    const unsigned int weekdays = weekdaysValue.as<unsigned int>();
    if (id == 0 || id > 255u || startMinute > 1439u || weekdays > 0x7Fu)
      return false;
    for (const auto &existing : parsed)
      if (existing.id == id)
        return false;

    IrrigationProgram p;
    p.id = static_cast<uint8_t>(id);
    p.enabled = programEnabledValue.as<bool>();
    p.startMinute = static_cast<uint16_t>(startMinute);
    p.weekdays = static_cast<uint8_t>(weekdays);
    JsonArrayConst zones = zonesValue.as<JsonArrayConst>();
    size_t zoneIndex = 0;
    for (JsonVariantConst zoneValue : zones)
    {
      if (!zoneValue.is<JsonObjectConst>())
        return false;
      JsonObjectConst zo = zoneValue.as<JsonObjectConst>();
      JsonVariantConst zoneIdValue = zo["uniqueId"];
      JsonVariantConst minutesValue = zo["minutes"];
      if (!zoneIdValue.is<const char *>() ||
          !minutesValue.is<unsigned int>())
        return false;

      const char *zoneId = zoneIdValue.as<const char *>();
      const unsigned int minutes = minutesValue.as<unsigned int>();
      IrrigationZone z;
      if (!zoneId || !zoneId[0] || strlen(zoneId) >= sizeof(z.uniqueId) ||
          minutes == 0 || minutes > maxZoneMinutes)
        return false;

      // Compare against the already-validated prefix of the submitted array,
      // including unknown valves that will be dropped below. Duplicate input
      // is malformed even when neither copy exists on this device.
      size_t priorIndex = 0;
      for (JsonVariantConst priorValue : zones)
      {
        if (priorIndex++ >= zoneIndex)
          break;
        const char *priorId = priorValue.as<JsonObjectConst>()["uniqueId"];
        if (strcmp(priorId, zoneId) == 0)
          return false;
      }
      zoneIndex++;

      // A zone naming a feature this device does not have is dropped rather than
      // kept as a hole in the cycle: the panel would show a program that waters
      // something invisible.
      if (!findZone(zoneId))
        continue;
      strlcpy(z.uniqueId, zoneId, sizeof(z.uniqueId));
      z.minutes = static_cast<uint16_t>(minutes);
      p.zones.push_back(z);
    }
    parsed.push_back(p);
  }
  // Resolve and close the open zones against the OLD schedule. Replacing the
  // vector first can make the running indices point at different zones (or at
  // none), losing the only reference to a valve that is still open.
  if (isRunning())
    stop();
  enabled = enabledValue.as<bool>();
  skipOnRain = skipOnRainValue.as<bool>();
  maxConcurrentZones = parsedMaxZones;
  programs = parsed;
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
  irr["maxConcurrentZones"] = maxConcurrentZones;
  const IrrigationProgram *p = running();
  if (p && !active.empty())
  {
    JsonObject run = irr["running"].to<JsonObject>();
    run["programId"] = p->id;
    // The first open zone stays at the top level as a single zone plus a single
    // countdown: panels and apps written before concurrency read exactly that,
    // and an update that blanked their status card would be a regression.
    run["zone"] = p->zones[active[0].index].uniqueId;
    run["secondsLeft"] = secondsLeft(active[0]);
    JsonArray list = run["zones"].to<JsonArray>();
    for (const auto &slot : active)
    {
      JsonObject o = list.add<JsonObject>();
      o["zone"] = p->zones[slot.index].uniqueId;
      o["secondsLeft"] = secondsLeft(slot);
    }
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

void Irrigation::statusJson(JsonVariant &root)
{
  jsonBody(root);
  JsonArray clocks = root["clocks"].to<JsonArray>();
  for (const auto &sw : config.actuatores)
  {
    unsigned long left = 0ul;
    unsigned long total = 0ul;
    if (!sw.valveClock(left, total))
      continue;
    JsonObject item = clocks.add<JsonObject>();
    item["zone"] = sw.uniqueId;
    item["secondsLeft"] = left;
    item["totalSeconds"] = total;
    // Only with a clock: without NTP the device does not know when "now" is, and
    // a made-up closing time would be worse than none.
    const String closesAt = clockIsoIn(left);
    if (closesAt.length() > 0)
      item["closesAt"] = closesAt;
  }
}

bool Irrigation::command(const char *payload)
{
  if (payload == nullptr)
    return false;
  if (strcmp(payload, "STOP") == 0)
  {
    stop();
    return true;
  }
  if (strncmp(payload, "RUN:", 4) == 0)
  {
    const long programId = atol(payload + 4);
    if (programId <= 0 || programId > 255)
      return false;
    runProgram((uint8_t)programId);
    return true;
  }
  if (strncmp(payload, "MAX:", 4) == 0)
  {
    const long wanted = atol(payload + 4);
    if (wanted < 1 || wanted > kMaxConcurrentZones)
      return false;
    // Persisted, or a reboot would undo what someone set from Home Assistant.
    maxConcurrentZones = (uint8_t)wanted;
    save();
    return true;
  }
  return false;
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

uint8_t Irrigation::openZoneLimit() const
{
  if (maxConcurrentZones < 1)
    return 1;
  return maxConcurrentZones > kMaxConcurrentZones ? kMaxConcurrentZones : maxConcurrentZones;
}

unsigned long Irrigation::secondsLeft(const ActiveZone &slot) const
{
  const long left = (long)(slot.endsAt - millis());
  return left > 0 ? (unsigned long)left / 1000ul : 0ul;
}

bool Irrigation::isRunningZone(const char *uniqueId) const
{
  const IrrigationProgram *p = running();
  if (!p)
    return false;
  for (const auto &slot : active)
  {
    if (slot.index < p->zones.size() &&
        strcmp(p->zones[slot.index].uniqueId, uniqueId) == 0)
      return true;
  }
  return false;
}

bool Irrigation::zoneCountdown(const char *uniqueId, unsigned long &left,
                               unsigned long &total) const
{
  const IrrigationProgram *p = running();
  if (!p)
    return false;
  for (const auto &slot : active)
  {
    if (slot.index >= p->zones.size() ||
        strcmp(p->zones[slot.index].uniqueId, uniqueId) != 0)
      continue;
    left = secondsLeft(slot);
    total = slot.totalSeconds;
    return true;
  }
  return false;
}

void Irrigation::clearRuntime()
{
  runningProgram = -1;
  active.clear();
  nextZone = 0;
}

void Irrigation::startPendingZones()
{
  const IrrigationProgram *p = running();
  if (!p)
    return;
  const size_t limit = openZoneLimit();
  while (active.size() < limit && nextZone < p->zones.size())
  {
    const size_t index = nextZone++;
    const IrrigationZone &z = p->zones[index];
    Actuator *valve = findZone(z.uniqueId);
    // A valve removed since the program was written is skipped, not waited for.
    if (!valve)
      continue;
    active.push_back({index, millis() + (unsigned long)z.minutes * 60000ul,
                      (unsigned long)z.minutes * 60ul});
#ifdef DEBUG_ONOFRE
    Log.notice("%s Irrigation: %s for %d min." CR, tags::actuatores, valve->name, z.minutes);
#endif
    deviceLog("rega abre %s %dmin", valve->name, z.minutes);
    valve->changeState(StateOrigin::INTERNAL, ActuatorState::ON_CLOSE);
  }
  if (active.empty())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Irrigation cycle finished." CR, tags::actuatores);
#endif
    clearRuntime();
  }
}

void Irrigation::closeActiveZones()
{
  const IrrigationProgram *p = running();
  if (!p)
    return;
  for (const auto &slot : active)
  {
    if (slot.index >= p->zones.size())
      continue;
    Actuator *valve = findZone(p->zones[slot.index].uniqueId);
    if (valve && valve->state == ActuatorState::ON_CLOSE)
      valve->changeState(StateOrigin::INTERNAL, ActuatorState::OFF_OPEN);
  }
}

void Irrigation::stop()
{
  closeActiveZones();
  clearRuntime();
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
    active.clear();
    nextZone = 0;
    programs[i].lastRunDay = clockWeekday();
    programs[i].lastRunMinute = clockMinuteOfDay();
    startPendingZones();
    return isRunning();
  }
  return false;
}

void Irrigation::loop()
{
  if (isRunning())
  {
    const IrrigationProgram *p = running();
    if (!p)
    {
      clearRuntime();
      return;
    }
    // Someone may have closed one of the open valves from a wall button, the app
    // or the cloud. That is a takeover, not a pause: the whole cycle ends rather
    // than reopening a valve under the person who just closed it, or leaving the
    // rest of the program running while they think they stopped the watering.
    for (const auto &slot : active)
    {
      Actuator *valve = slot.index < p->zones.size() ? findZone(p->zones[slot.index].uniqueId) : nullptr;
      if (!valve || valve->state != ActuatorState::ON_CLOSE)
      {
#ifdef DEBUG_ONOFRE
        Log.notice("%s Irrigation cycle cancelled: zone was closed elsewhere." CR, tags::actuatores);
#endif
        deviceLog("rega cancelada: valvula fechada por fora");
        clearRuntime();
        return;
      }
    }
    // Close whatever ran out, then refill the free slots. Iterating backwards
    // keeps the surviving indices valid while erasing.
    bool freed = false;
    for (size_t i = active.size(); i-- > 0;)
    {
      if ((long)(millis() - active[i].endsAt) < 0)
        continue;
      Actuator *valve = findZone(p->zones[active[i].index].uniqueId);
      if (valve && valve->state == ActuatorState::ON_CLOSE)
        valve->changeState(StateOrigin::INTERNAL, ActuatorState::OFF_OPEN);
      active.erase(active.begin() + i);
      freed = true;
    }
    if (freed)
      startPendingZones();
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
      // The single most confusing silence in the whole feature: a schedule that
      // does nothing because a sensor says it is raining.
      deviceLog("rega saltada: chuva");
#ifdef DEBUG_ONOFRE
      Log.notice("%s Irrigation skipped: raining." CR, tags::actuatores);
#endif
      continue;
    }
    runningProgram = (int)i;
    active.clear();
    nextZone = 0;
    startPendingZones();
    return;
  }
}
