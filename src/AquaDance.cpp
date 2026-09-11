#include "AquaDance.h"
#include "ConfigOnofre.h"
#include "DeviceLog.h"
#include "Persistence.h"
#include "Irrigation.h"
#include <LittleFS.h>
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif

extern ConfigOnofre config;
AquaDance aquadance;

namespace
{
  Actuator *findFixture(const char *uniqueId)
  {
    for (auto &a : config.actuatores)
    {
      if (a.ready && (a.isGardenValve() || a.isLight()) && strcmp(a.uniqueId, uniqueId) == 0)
        return &a;
    }
    return nullptr;
  }
}

uint8_t AquaDance::runningShowId() const
{
  const AquaShow *p = running();
  return p ? p->id : 0;
}

const AquaShow *AquaDance::running() const
{
  if (runningShow < 0 || (size_t)runningShow >= shows.size())
    return nullptr;
  return &shows[runningShow];
}

void AquaDance::clearRuntime()
{
  runningShow = -1;
  currentStep = 0;
  nextStepAt = 0;
}

void AquaDance::load()
{
  if (!LittleFS.exists(configFilenames::aquadance))
    return;
  File file = LittleFS.open(configFilenames::aquadance, "r");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error)
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s AquaDance file unreadable, starting with default." CR, tags::config);
#endif
    return;
  }
  JsonObject root = doc.as<JsonObject>();
  update(root);
}

bool AquaDance::save()
{
  JsonDocument doc;
  doc["enabled"] = enabled;
  JsonArray list = doc["shows"].to<JsonArray>();
  for (const auto &s : shows)
  {
    JsonObject so = list.add<JsonObject>();
    so["id"] = s.id;
    so["name"] = s.name;
    so["stepMs"] = s.stepMs;
    so["totalSteps"] = s.totalSteps;
    so["loop"] = s.loop;
    JsonArray ts = so["tracks"].to<JsonArray>();
    for (const auto &t : s.tracks)
    {
      JsonObject to = ts.add<JsonObject>();
      to["uniqueId"] = t.uniqueId;
      to["trackType"] = t.trackType;
      to["posX"] = t.posX;
      to["posY"] = t.posY;
      JsonArray st = to["steps"].to<JsonArray>();
      for (const auto stepVal : t.steps)
      {
        st.add(stepVal);
      }
      if (!t.rgbw.empty())
      {
        JsonArray clr = to["rgbw"].to<JsonArray>();
        for (const auto cVal : t.rgbw)
        {
          clr.add(cVal);
        }
      }
    }
  }
  return persistJsonAtomically(configFilenames::aquadance,
                               configFilenames::aquadanceTemporary, doc);
}

bool AquaDance::update(JsonObject &root)
{
  JsonVariantConst enabledVal = root["enabled"];
  JsonVariantConst showsVal = root["shows"];

  if (!enabledVal.isNull() && enabledVal.is<bool>())
  {
    enabled = enabledVal.as<bool>();
  }

  if (showsVal.isNull() || !showsVal.is<JsonArrayConst>())
    return false;

  JsonArrayConst list = showsVal.as<JsonArrayConst>();
  if (list.size() > kMaxAquaShows)
    return false;

  std::vector<AquaShow> parsedShows;
  for (JsonVariantConst showItem : list)
  {
    if (!showItem.is<JsonObjectConst>())
      return false;
    JsonObjectConst so = showItem.as<JsonObjectConst>();

    JsonVariantConst idVal = so["id"];
    JsonVariantConst nameVal = so["name"];
    JsonVariantConst stepMsVal = so["stepMs"];
    JsonVariantConst totalStepsVal = so["totalSteps"];
    JsonVariantConst loopVal = so["loop"];
    JsonVariantConst tracksVal = so["tracks"];

    if (!idVal.is<unsigned int>() || !tracksVal.is<JsonArrayConst>())
      return false;

    AquaShow s;
    s.id = static_cast<uint8_t>(idVal.as<unsigned int>());
    if (s.id == 0)
      return false;

    if (nameVal.is<const char *>())
    {
      strlcpy(s.name, nameVal.as<const char *>(), sizeof(s.name));
    }
    else
    {
      snprintf(s.name, sizeof(s.name), "AquaDance %u", s.id);
    }

    unsigned int stepMs = stepMsVal.is<unsigned int>() ? stepMsVal.as<unsigned int>() : kDefaultStepMs;
    if (stepMs < kMinStepMs)
      stepMs = kMinStepMs;
    if (stepMs > kMaxStepMs)
      stepMs = kMaxStepMs;
    s.stepMs = static_cast<uint16_t>(stepMs);

    unsigned int totalSteps = totalStepsVal.is<unsigned int>() ? totalStepsVal.as<unsigned int>() : 32;
    if (totalSteps < 1)
      totalSteps = 1;
    if (totalSteps > kMaxAquaSteps)
      totalSteps = kMaxAquaSteps;
    s.totalSteps = static_cast<uint16_t>(totalSteps);

    s.loop = loopVal.is<bool>() ? loopVal.as<bool>() : false;

    JsonArrayConst tracksList = tracksVal.as<JsonArrayConst>();
    for (JsonVariantConst trackItem : tracksList)
    {
      if (!trackItem.is<JsonObjectConst>())
        continue;
      JsonObjectConst to = trackItem.as<JsonObjectConst>();
      const char *uid = to["uniqueId"] | "";
      if (!uid || !uid[0])
        continue;

      AquaTrack t;
      strlcpy(t.uniqueId, uid, sizeof(t.uniqueId));
      t.trackType = to["trackType"].is<unsigned int>() ? static_cast<uint8_t>(to["trackType"].as<unsigned int>()) : TRACK_VALVE;
      t.posX = to["posX"].is<float>() ? to["posX"].as<float>() : 50.0f;
      t.posY = to["posY"].is<float>() ? to["posY"].as<float>() : 50.0f;
      t.steps.assign(s.totalSteps, 0);

      JsonArrayConst stepsList = to["steps"].as<JsonArrayConst>();
      size_t stepIdx = 0;
      for (JsonVariantConst stVal : stepsList)
      {
        if (stepIdx >= s.totalSteps)
          break;
        t.steps[stepIdx] = static_cast<uint8_t>(stVal.as<int>());
        stepIdx++;
      }

      if (to["rgbw"].is<JsonArrayConst>())
      {
        t.rgbw.assign(s.totalSteps, 0x00FFFFFF);
        JsonArrayConst rgbwList = to["rgbw"].as<JsonArrayConst>();
        size_t cIdx = 0;
        for (JsonVariantConst cVal : rgbwList)
        {
          if (cIdx >= s.totalSteps)
            break;
          t.rgbw[cIdx] = cVal.as<uint32_t>();
          cIdx++;
        }
      }

      s.tracks.push_back(t);
    }
    parsedShows.push_back(s);
  }

  if (isRunning())
    stop();

  shows = parsedShows;
  return true;
}

void AquaDance::json(JsonVariant &root)
{
  JsonVariant aquadanceRoot = root["aquadance"].to<JsonObject>();
  jsonBody(aquadanceRoot);
}

void AquaDance::jsonBody(JsonVariant &root)
{
  root["enabled"] = enabled;
  const AquaShow *p = running();
  if (p)
  {
    JsonObject run = root["running"].to<JsonObject>();
    run["showId"] = p->id;
    run["step"] = currentStep;
    run["totalSteps"] = p->totalSteps;
    run["stepMs"] = p->stepMs;
    run["loop"] = p->loop;
  }
  else
  {
    root["running"] = nullptr;
  }

  JsonArray list = root["shows"].to<JsonArray>();
  for (const auto &s : shows)
  {
    JsonObject so = list.add<JsonObject>();
    so["id"] = s.id;
    so["name"] = s.name;
    so["stepMs"] = s.stepMs;
    so["totalSteps"] = s.totalSteps;
    so["loop"] = s.loop;
    JsonArray ts = so["tracks"].to<JsonArray>();
    for (const auto &t : s.tracks)
    {
      JsonObject to = ts.add<JsonObject>();
      to["uniqueId"] = t.uniqueId;
      to["trackType"] = t.trackType;
      to["posX"] = t.posX;
      to["posY"] = t.posY;
      JsonArray st = to["steps"].to<JsonArray>();
      for (const auto stepVal : t.steps)
      {
        st.add(stepVal);
      }
      if (!t.rgbw.empty())
      {
        JsonArray clr = to["rgbw"].to<JsonArray>();
        for (const auto cVal : t.rgbw)
        {
          clr.add(cVal);
        }
      }
    }
  }
}

void AquaDance::statusJson(JsonVariant &root)
{
  jsonBody(root);
}

void AquaDance::closeAllShowValves()
{
  const AquaShow *p = running();
  if (!p)
    return;
  for (const auto &t : p->tracks)
  {
    Actuator *fixture = findFixture(t.uniqueId);
    if (fixture && fixture->state != ActuatorState::OFF_OPEN)
    {
      fixture->changeState(StateOrigin::INTERNAL, ActuatorState::OFF_OPEN);
    }
  }
}

void AquaDance::applyCurrentStep()
{
  const AquaShow *p = running();
  if (!p)
    return;

  for (const auto &t : p->tracks)
  {
    Actuator *fixture = findFixture(t.uniqueId);
    if (!fixture)
      continue;

    uint8_t stepVal = 0;
    if (currentStep < t.steps.size())
    {
      stepVal = t.steps[currentStep];
    }

    if (t.trackType == TRACK_VALVE)
    {
      bool shouldOpen = (stepVal > 0);
      if (shouldOpen && fixture->state != ActuatorState::ON_CLOSE)
      {
        fixture->changeState(StateOrigin::INTERNAL, ActuatorState::ON_CLOSE);
      }
      else if (!shouldOpen && fixture->state == ActuatorState::ON_CLOSE)
      {
        fixture->changeState(StateOrigin::INTERNAL, ActuatorState::OFF_OPEN);
      }
    }
    else // TRACK_LIGHT_DIMMER or TRACK_LIGHT_RGBW
    {
      int targetState = stepVal > 0 ? (stepVal > 1 ? stepVal : 100) : 0;
      if (fixture->state != targetState)
      {
        fixture->changeState(StateOrigin::INTERNAL, targetState);
      }
    }
  }
}

bool AquaDance::play(uint8_t showId)
{
  for (size_t i = 0; i < shows.size(); i++)
  {
    if (shows[i].id != showId)
      continue;

    if (irrigation.isRunning())
      irrigation.stop();

    if (isRunning())
      stop();

    runningShow = (int)i;
    currentStep = 0;
    nextStepAt = millis() + shows[i].stepMs;

#ifdef DEBUG_ONOFRE
    Log.notice("%s AquaDance started show: %s" CR, tags::actuatores, shows[i].name);
#endif
    deviceLog("aquadance inicio %s", shows[i].name);

    applyCurrentStep();
    return true;
  }
  return false;
}

void AquaDance::stop()
{
  if (!isRunning())
    return;
#ifdef DEBUG_ONOFRE
  Log.notice("%s AquaDance stopped." CR, tags::actuatores);
#endif
  deviceLog("aquadance parado");
  closeAllShowValves();
  clearRuntime();
}

void AquaDance::loop()
{
  if (!isRunning())
    return;

  const AquaShow *p = running();
  if (!p)
  {
    clearRuntime();
    return;
  }

  if ((long)(millis() - nextStepAt) >= 0)
  {
    currentStep++;
    if (currentStep >= p->totalSteps)
    {
      if (p->loop)
      {
        currentStep = 0;
      }
      else
      {
        stop();
        return;
      }
    }
    nextStepAt = millis() + p->stepMs;
    applyCurrentStep();
  }
}

bool AquaDance::command(const char *payload)
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
    const long showId = atol(payload + 4);
    if (showId <= 0 || showId > 255)
      return false;
    return play((uint8_t)showId);
  }
  return false;
}
