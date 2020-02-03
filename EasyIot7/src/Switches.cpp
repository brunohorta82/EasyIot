#include "Switches.h"
#include "Discovery.h"
#include "WebServer.h"
#include "constants.h"
#include "Config.h"
#include "Mqtt.h"
#include "WebRequests.h"
#include <esp-knx-ip.h>
#include <Bounce2.h>
#include "CloudIO.h"
#include <Shutters.h>

void shuttersOperationHandler(Shutters *s, ShuttersOperation operation)
{
  switch (operation)
  {
  case ShuttersOperation::UP:
    strlcpy(s->getSwitchT()->stateControl, constanstsSwitch::payloadOpen, sizeof(s->getSwitchT()->stateControl));
    strlcpy(s->getSwitchT()->mqttPayload, constanstsSwitch::payloadOpen, sizeof(s->getSwitchT()->mqttPayload));
    if (s->getSwitchT()->typeControl == SwitchControlType::PIN_OUTPUT)
    {
      configPIN(s->getSwitchT()->primaryGpioControl, OUTPUT);
      configPIN(s->getSwitchT()->secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW); //TURN OFF . STOP
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH); //TURN ON . OPEN REQUEST
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH); //TURN ON . EXECUTE REQUEST
    }
    break;
  case ShuttersOperation::DOWN:
    strlcpy(s->getSwitchT()->stateControl, constanstsSwitch::payloadClose, sizeof(s->getSwitchT()->stateControl));
    strlcpy(s->getSwitchT()->mqttPayload, constanstsSwitch::payloadStateClose, sizeof(s->getSwitchT()->mqttPayload));
    if (s->getSwitchT()->typeControl == SwitchControlType::PIN_OUTPUT)
    {
      configPIN(s->getSwitchT()->primaryGpioControl, OUTPUT);
      configPIN(s->getSwitchT()->secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW); //TURN OFF . STOP
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW); //TURN OFF . CLOSE REQUEST
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH); //TURN ON . EXECUTE REQUEST
    }

    break;
  case ShuttersOperation::HALT:
    strlcpy(s->getSwitchT()->stateControl, constanstsSwitch::payloadStop, sizeof(s->getSwitchT()->stateControl));
    strlcpy(s->getSwitchT()->mqttPayload, constanstsSwitch::payloadStateStop, sizeof(s->getSwitchT()->mqttPayload));
    if (s->getSwitchT()->typeControl == SwitchControlType::PIN_OUTPUT)
    {
      configPIN(s->getSwitchT()->primaryGpioControl, OUTPUT);
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW); //TURN OFF . STOP
    }
    break;
  }
}

void onShuttersLevelReached(Shutters *shutters, uint8_t level)
{
  shutters->getSwitchT()->lastPercentage = level;
  publishOnMqtt(shutters->getSwitchT()->mqttStateTopic, String(level).c_str(), shutters->getSwitchT()->mqttRetain);
  static unsigned long measurement_timestamp = millis( );
  if( millis( ) - measurement_timestamp > 2000ul )
  {
      measurement_timestamp = millis( );
      sendToServerEvents("shutters", String(level).c_str());

  }
  
}

static const String statesPool[] = {constanstsSwitch::payloadOff, constanstsSwitch::payloadOn, constanstsSwitch::payloadStop, constanstsSwitch::payloadOpen, constanstsSwitch::payloadStop, constanstsSwitch::payloadClose, constanstsSwitch::payloadReleased, constanstsSwitch::payloadUnlock, constanstsSwitch::payloadLock};

struct Switches &getAtualSwitchesConfig()
{
  static Switches switches;
  return switches;
}

void save(Switches &switches)
{
  switches.lastChange = 0ul;
  if (!SPIFFS.begin())
  {
#ifdef DEBUG
    Log.error("%s File storage can't start" CR, tags::switches);
#endif
    return;
  }

  File file = SPIFFS.open(configFilenames::switches, "w+");
  if (!file)
  {
#ifdef DEBUG
    Log.error("%s Failed to create file" CR, tags::switches);
#endif
    return;
  }
  switches.save(file);
  file.close();
  SPIFFS.end();
#ifdef DEBUG
  Log.notice("%s Config stored." CR, tags::switches);
#endif
}
size_t Switches::serializeToJson(Print &output)
{
  if (items.empty())
    return output.write("[]");

  const size_t CAPACITY = JSON_ARRAY_SIZE(items.size()) + items.size() * (JSON_OBJECT_SIZE(30) + sizeof(SwitchT));
  DynamicJsonDocument doc(CAPACITY);

  for (const auto &sw : items)
  {
    JsonObject sdoc = doc.createNestedObject();
    sdoc["id"] = sw.id;
    sdoc["name"] = sw.name;
    sdoc["firmware"] = sw.firmware;
    sdoc["family"] = sw.family;
    sdoc["primaryGpio"] = sw.primaryGpio;
    sdoc["secondaryGpio"] = sw.secondaryGpio;
    sdoc["autoStateValue"] = sw.autoStateValue;
    sdoc["autoStateDelay"] = sw.autoStateDelay;
    sdoc["typeControl"] = static_cast<int>(sw.typeControl);
    sdoc["mode"] = static_cast<int>(sw.mode);
    sdoc["pullup"] = sw.pullup;
    sdoc["mqttRetain"] = sw.mqttRetain;
    sdoc["inverted"] = sw.inverted;
    sdoc["mqttCommandTopic"] = sw.mqttCommandTopic;
    sdoc["mqttStateTopic"] = sw.mqttStateTopic;
    sdoc["knxLevelOne"] = sw.knxLevelOne;
    sdoc["knxLevelTwo"] = sw.knxLevelTwo;
    sdoc["knxLevelThree"] = sw.knxLevelThree;
    if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
    {
      sdoc["secondaryGpioControl"] = sw.secondaryGpioControl;
      sdoc["upCourseTime"] = sw.upCourseTime;
      sdoc["lastPercentage"] = sw.lastPercentage;
      sdoc["downCourseTime"] = sw.downCourseTime;
      sdoc["calibrationRatio"] = sw.calibrationRatio;
    }

    sdoc["primaryGpioControl"] = sw.primaryGpioControl;
    sdoc["mqttPayload"] = sw.mqttPayload;
    sdoc["stateControl"] = sw.stateControl;

    sdoc["cloudIOSupport"] = sw.cloudIOSupport;
    sdoc["haSupport"] = sw.haSupport;
    sdoc["knxSupport"] = sw.knxSupport;
    sdoc["mqttSupport"] = sw.mqttSupport;
  }

  return serializeJson(doc, output);
}
void readLastShutterState(char *dest, byte length, char *value)
{
  for (byte i = 0; i < length; i++)
  {
    dest[i] = value[i];
  }
}
void shuttersWriteStateHandler(Shutters *shutters, const char *state, byte length)
{
  for (byte i = 0; i < length; i++)
  {
    shutters->getSwitchT()->shutterState[i] = state[i];
  }
  getAtualSwitchesConfig().lastChange = millis();
  sendToServerEvents("shutters", String(shutters->getSwitchT()->lastPercentage).c_str());
}

void SwitchT::save(File &file) const
{
  file.write((uint8_t *)&firmware, sizeof(firmware));
  file.write((uint8_t *)id, sizeof(id));
  file.write((uint8_t *)name, sizeof(name));
  file.write((uint8_t *)family, sizeof(family));
  file.write((uint8_t *)&mode, sizeof(mode));
  file.write((uint8_t *)&typeControl, sizeof(typeControl));
  file.write((uint8_t *)&primaryGpio, sizeof(primaryGpio));
  file.write((uint8_t *)&secondaryGpio, sizeof(secondaryGpio));
  file.write((uint8_t *)&pullup, sizeof(pullup));
  file.write((uint8_t *)&primaryGpioControl, sizeof(primaryGpioControl));
  file.write((uint8_t *)&secondaryGpioControl, sizeof(secondaryGpioControl));
  file.write((uint8_t *)&inverted, sizeof(inverted));
  file.write((uint8_t *)&autoStateDelay, sizeof(autoStateDelay));
  file.write((uint8_t *)autoStateValue, sizeof(autoStateValue));
  file.write((uint8_t *)&automationTimeA, sizeof(automationTimeA));
  file.write((uint8_t *)&childLock, sizeof(childLock));

  //MQTT
  file.write((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.write((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.write((uint8_t *)infoA, sizeof(infoA));
  file.write((uint8_t *)infoB, sizeof(infoB));
  file.write((uint8_t *)mqttPayload, sizeof(mqttPayload));
  file.write((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  //CONTROL VARIABLES
  file.write((uint8_t *)stateControl, sizeof(stateControl));
  file.write((uint8_t *)&positionControlCover, sizeof(positionControlCover));
  file.write((uint8_t *)&lastPercentage, sizeof(lastPercentage));

  file.write((uint8_t *)&lastPrimaryGpioState, sizeof(lastPrimaryGpioState));
  file.write((uint8_t *)&lastSecondaryGpioState, sizeof(lastSecondaryGpioState));
  file.write((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
  file.write((uint8_t *)&statePoolStart, sizeof(statePoolStart));
  file.write((uint8_t *)&statePoolEnd, sizeof(statePoolEnd));

  file.write((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  file.write((uint8_t *)&haSupport, sizeof(haSupport));
  file.write((uint8_t *)&knxSupport, sizeof(knxSupport));
  file.write((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.write((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.write((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));

  file.write((uint8_t *)&automationTimeB, sizeof(automationTimeB));
  file.write((uint8_t *)&calibrationRatio, sizeof(calibrationRatio));
  file.write((uint8_t *)&upCourseTime, sizeof(upCourseTime));
  file.write((uint8_t *)&downCourseTime, sizeof(downCourseTime));
  file.write((uint8_t *)&mqttSupport, sizeof(mqttSupport));
  file.write((uint8_t *)&shutterState, sizeof(shutterState));
}
void SwitchT::load(File &file)
{
  file.read((uint8_t *)&firmware, sizeof(firmware));
  file.read((uint8_t *)id, sizeof(id));
  file.read((uint8_t *)name, sizeof(name));
  file.read((uint8_t *)family, sizeof(family));
  file.read((uint8_t *)&mode, sizeof(mode));
  file.read((uint8_t *)&typeControl, sizeof(typeControl));
  file.read((uint8_t *)&primaryGpio, sizeof(primaryGpio));
  file.read((uint8_t *)&secondaryGpio, sizeof(secondaryGpio));
  file.read((uint8_t *)&pullup, sizeof(pullup));
  file.read((uint8_t *)&primaryGpioControl, sizeof(primaryGpioControl));
  file.read((uint8_t *)&secondaryGpioControl, sizeof(secondaryGpioControl));
  file.read((uint8_t *)&inverted, sizeof(inverted));
  file.read((uint8_t *)&autoStateDelay, sizeof(autoStateDelay));
  file.read((uint8_t *)autoStateValue, sizeof(autoStateValue));
  file.read((uint8_t *)&automationTimeA, sizeof(automationTimeA));
  file.read((uint8_t *)&childLock, sizeof(childLock));

  //MQTT
  file.read((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.read((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.read((uint8_t *)infoA, sizeof(infoA));
  file.read((uint8_t *)infoB, sizeof(infoB));
  file.read((uint8_t *)mqttPayload, sizeof(mqttPayload));
  file.read((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  //CONTROL VARIABLES
  file.read((uint8_t *)stateControl, sizeof(stateControl));
  file.read((uint8_t *)&positionControlCover, sizeof(positionControlCover));
  file.read((uint8_t *)&lastPercentage, sizeof(lastPercentage));

  file.read((uint8_t *)&lastPrimaryGpioState, sizeof(lastPrimaryGpioState));
  file.read((uint8_t *)&lastSecondaryGpioState, sizeof(lastSecondaryGpioState));
  file.read((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
  file.read((uint8_t *)&statePoolStart, sizeof(statePoolStart));
  file.read((uint8_t *)&statePoolEnd, sizeof(statePoolEnd));

  file.read((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  file.read((uint8_t *)&haSupport, sizeof(haSupport));
  file.read((uint8_t *)&knxSupport, sizeof(knxSupport));
  file.read((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.read((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.read((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));
  if (firmware >= 7.851)
  {
    file.read((uint8_t *)&automationTimeB, sizeof(automationTimeB));
    file.read((uint8_t *)&calibrationRatio, sizeof(calibrationRatio));
    file.read((uint8_t *)&upCourseTime, sizeof(upCourseTime));
    file.read((uint8_t *)&downCourseTime, sizeof(downCourseTime));
    file.read((uint8_t *)&mqttSupport, sizeof(mqttSupport));
    file.read((uint8_t *)&shutterState, sizeof(shutterState));
  }
  else
  {
    automationTimeA = 0;
    automationTimeB = 0;
    calibrationRatio = 0.1;
    upCourseTime = 45 * 1000;
    downCourseTime = 45 * 1000;
    mqttSupport = true;
  }
  shutter = new Shutters(this);
  char storedShuttersState[shutter->getStateLength()];
  readLastShutterState(storedShuttersState, shutter->getStateLength(), shutterState);
  shutter->setOperationHandler(shuttersOperationHandler)
      .setWriteStateHandler(shuttersWriteStateHandler)
      .restoreState(storedShuttersState)
      .setCourseTime(upCourseTime, downCourseTime)
      .onLevelReached(onShuttersLevelReached)
      .begin();
  if (primaryGpio != constantsConfig::noGPIO)
  {
    debouncerPrimary = new Bounce();
    debouncerPrimary->attach(primaryGpio, primaryGpio == 16u ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    debouncerPrimary->interval(5);
  }

  if (secondaryGpio != constantsConfig::noGPIO)
  {
    debouncerSecondary = new Bounce();
    debouncerSecondary->attach(secondaryGpio, secondaryGpio == 16u ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    debouncerSecondary->interval(5);
  }
  if (firmware < VERSION)
  {
#ifdef DEBUG
    Log.notice("%s Migrate Firmware from %F to %F" CR, tags::switches, firmware, VERSION);
#endif
    firmware = VERSION;
  }
}

void Switches::save(File &file) const
{
  auto n_items = items.size();
  file.write((uint8_t *)&n_items, sizeof(n_items));
  for (const auto &item : items)
  {
    item.save(file);
  }
}
void switchesCallback(message_t const &msg, void *arg)
{
  auto s = static_cast<SwitchT *>(arg);
  switch (msg.ct)
  {
  case KNX_CT_WRITE:
  {
    int stateIdx = (int)msg.data[1];
    s->knxNotifyGroup = false;

    s->changeState(statesPool[stateIdx].c_str());

    break;
  }
  case KNX_CT_READ:
  {
    break;
  }
  }
}
void allwitchesCallback(message_t const &msg, void *arg)
{
  switch (msg.ct)
  {
  case KNX_CT_WRITE:
  {
    int stateIdx = (int)msg.data[0];
    for (auto &sw : getAtualSwitchesConfig().items)
    {
      sw.knxNotifyGroup = false;
      sw.changeState(statesPool[stateIdx].c_str());
    }
    break;
  }
  case KNX_CT_READ:
  {

    break;
  }
  }
}
void Switches::load(File &file)
{
  knx.start(nullptr);
  auto n_items = items.size();
  file.read((uint8_t *)&n_items, sizeof(n_items));
  items.clear();
  items.resize(n_items);
  bool globalKnxLevelThreeAssign = false;
  for (auto &item : items)
  {
    item.load(file);
    configPIN(item.primaryGpio, item.primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    configPIN(item.secondaryGpio, item.secondaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    configPIN(item.primaryGpioControl, OUTPUT);
    configPIN(item.secondaryGpioControl, OUTPUT);
    //KNX
    if (item.knxSupport && item.knxLevelOne > 0 && item.knxLevelTwo > 0 && item.knxLevelThree > 0)
    {
      if (!globalKnxLevelThreeAssign)
      {
        knx.callback_assign(knx.callback_register("ALL SWITCHES", allwitchesCallback), knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, 0));
        globalKnxLevelThreeAssign = true;
      }
      item.knxIdRegister = knx.callback_register(String(item.name), switchesCallback, &item);
      item.knxIdAssign = knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, item.knxLevelThree));
    }
  }
}
bool Switches::remove(const char *id)
{

  auto match = std::find_if(items.begin(), items.end(), [id](const SwitchT &item) {
    return strcmp(id, item.id) == 0;
  });

  if (match == items.end())
  {
    return false;
  }

  publishOnMqtt(match->mqttCommandTopic, "", true);
  removeFromHaDiscovery(*match);

  unsubscribeOnMqtt(match->mqttCommandTopic);

  items.erase(match);

  return true;
}

void remove(Switches &switches, const char *id)
{
  if (switches.remove(id))
    save(switches);
}
void SwitchT::reloadMqttTopics()
{
  String mqttTopic;
  mqttTopic.reserve(sizeof(mqttCommandTopic));
  mqttTopic.concat(getBaseTopic());
  mqttTopic.concat("/");
  mqttTopic.concat(family);
  mqttTopic.concat("/");
  mqttTopic.concat(id);
  mqttTopic.concat("/set");
  strlcpy(mqttCommandTopic, mqttTopic.c_str(), sizeof(mqttCommandTopic));
  mqttTopic.replace("/set", "/state");
  strlcpy(mqttStateTopic, mqttTopic.c_str(), sizeof(mqttStateTopic));
}
void reloadSwitches()
{
  for (auto &sw : getAtualSwitchesConfig().items)
  {
    sw.reloadMqttTopics();
  }
  save(getAtualSwitchesConfig());
}
void templateSwitch(SwitchT &sw, const String &name, const char *family, const SwitchMode &mode, unsigned int primaryGpio, unsigned int secondaryGpio, unsigned int primaryGpioControl, unsigned int secondaryGpioControl, bool mqttRetaint = false, unsigned long autoStateDelay = 0ul, const String &autoStateValue = "", const SwitchControlType &typecontrol = PIN_OUTPUT, unsigned long automationTimeA = 0ul, bool haSupport = false, bool cloudIOSupport = true, uint8_t knxLevelOne = 0, uint8_t knxLevelTwo = 0, uint8_t knxLevelThree = 0, bool knxSupport = false, unsigned long automationTimeB = 0ul, bool mqttSupport = true,unsigned long upCourseTime = 45,unsigned long downCourseTime = 45)
{
  String idStr;
  generateId(idStr, name, sizeof(sw.id));
  strlcpy(sw.id, idStr.c_str(), sizeof(sw.id));
  strlcpy(sw.name, name.c_str(), sizeof(sw.name));
  strlcpy(sw.family, family, sizeof(sw.family));
  sw.primaryGpio = primaryGpio;
  sw.secondaryGpio = secondaryGpio;
  sw.autoStateDelay = autoStateDelay * 1000;
  strlcpy(sw.autoStateValue, autoStateValue.c_str(), sizeof(sw.autoStateValue));
  sw.typeControl = typecontrol;
  sw.mode = mode;
  sw.knxSupport = knxSupport;
  sw.haSupport = haSupport;
  sw.upCourseTime = upCourseTime * 1000;
  sw.downCourseTime = downCourseTime * 1000;
  sw.cloudIOSupport = cloudIOSupport;
  sw.pullup = true;
  sw.mqttRetain = mqttRetaint;
  sw.inverted = false;
  sw.reloadMqttTopics();
  if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
  {
    sw.statePoolStart = constanstsSwitch::coverStartIdx;
    sw.statePoolEnd = constanstsSwitch::converEndIdx;
    sw.shutter = new Shutters(&sw);
    char storedShuttersState[sw.shutter->getStateLength()];
    readLastShutterState(storedShuttersState, sw.shutter->getStateLength(), sw.shutterState);
    sw.shutter->setOperationHandler(shuttersOperationHandler)
        .setWriteStateHandler(shuttersWriteStateHandler)
        .restoreState(storedShuttersState)
        .setCourseTime(sw.upCourseTime, sw.downCourseTime)
        .onLevelReached(onShuttersLevelReached)
        .begin();
  }
  else if (strcmp(sw.family, constanstsSwitch::familySwitch) == 0 || strcmp(sw.family, constanstsSwitch::familyLight) == 0)
  {
    sw.statePoolStart = constanstsSwitch::switchStartIdx;
    sw.statePoolEnd = constanstsSwitch::switchEndIdx;
  }
  else if (strcmp(sw.family, constanstsSwitch::familyLock) == 0)
  {
    sw.statePoolStart = constanstsSwitch::lockStartIdx;
    sw.statePoolEnd = constanstsSwitch::lockEndIdx;
  }
  sw.statePoolIdx = sw.statePoolStart;

  sw.automationTimeA = automationTimeA;
  sw.automationTimeB = automationTimeB;

  sw.primaryGpioControl = primaryGpioControl;
  sw.secondaryGpioControl = secondaryGpioControl;

  if (sw.primaryGpio != constantsConfig::noGPIO)
  {
    sw.debouncerPrimary = new Bounce();
    sw.debouncerPrimary->attach(sw.primaryGpio, sw.primaryGpio == 16u ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    sw.debouncerPrimary->interval(5);
  }

  if (sw.secondaryGpio != constantsConfig::noGPIO)
  {
    sw.debouncerSecondary = new Bounce();
    sw.debouncerSecondary->attach(sw.secondaryGpio, sw.secondaryGpio == 16u ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    sw.debouncerSecondary->interval(5);
  }

  configPIN(sw.primaryGpioControl, OUTPUT);
  configPIN(sw.secondaryGpioControl, OUTPUT);
  sw.lastPrimaryGpioState = true;
  sw.lastSecondaryGpioState = true;
  strlcpy(sw.stateControl, constanstsSwitch::payloadOff, sizeof(sw.mqttPayload));
  strlcpy(sw.mqttPayload, sw.stateControl, sizeof(sw.mqttPayload));
  sw.mqttSupport = mqttSupport;
  sw.knxLevelOne = knxLevelOne;
  sw.knxLevelTwo = knxLevelTwo;
  sw.knxLevelThree = knxLevelThree;
  knx.callback_unassign(sw.knxIdAssign);
  knx.callback_deregister(sw.knxIdRegister);
  sw.knxIdRegister = knx.callback_register(String(sw.name), switchesCallback, &sw);
  sw.knxIdAssign = knx.callback_assign(sw.knxIdRegister, knx.GA_to_address(sw.knxLevelOne, sw.knxLevelTwo, sw.knxLevelThree));
}
void SwitchT::updateFromJson(JsonObject doc)
{
  templateSwitch(*this,
                 doc["name"], doc["family"],
                 static_cast<SwitchMode>(doc["mode"] | static_cast<int>(SWITCH)),
                 doc["primaryGpio"] | constantsConfig::noGPIO,
                 doc["secondaryGpio"] | constantsConfig::noGPIO,
                 doc["primaryGpioControl"] | constantsConfig::noGPIO,
                 doc["secondaryGpioControl"] | constantsConfig::noGPIO,
                 doc["mqttRetain"] | false, doc["autoStateDelay"] | 0ul,
                 doc["autoStateValue"] | "",
                 static_cast<SwitchControlType>(doc["typeControl"] | static_cast<int>(SwitchControlType::MQTT)),
                 doc["automationTimeA"] | 0ul,
                 doc["haSupport"] | false,
                 doc["cloudIOSupport"] | true,
                 doc["knxLevelOne"] | 0,
                 doc["knxLevelTwo"] | 0,
                 doc["knxLevelThree"] | 0,
                 doc["knxSupport"],
                 doc["automationTimeB"] | 0ul,
                 doc["mqttSupport"] | true,
                 doc["upCourseTime"] | 45ul,
                 doc["downCourseTime"] | 45ul);
  doc["id"] = id;
  doc["stateControl"] = stateControl;
  doc["lastPercentage"] = lastPercentage;
  doc["upCourseTime"] = upCourseTime;
  doc["downCourseTime"] = downCourseTime;
  if (!doc["mqttCommandTopic"].isNull())
  {
    strlcpy(mqttCommandTopic, doc["mqttCommandTopic"], sizeof(mqttCommandTopic));
  }
  if (!doc["mqttStateTopic"].isNull())
  {
    strlcpy(mqttStateTopic, doc["mqttStateTopic"], sizeof(mqttCommandTopic));
  }
  doc["mqttCommandTopic"] = mqttCommandTopic;
  doc["mqttStateTopic"] = mqttStateTopic;
}
void saveAndRefreshServices(Switches &switches, const SwitchT &sw)
{
  save(switches);
  removeFromHaDiscovery(sw);
  if (sw.haSupport)
  {
    addToHaDiscovery(sw);
  }
}
void update(Switches &switches, const String &id, JsonObject doc)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(id.c_str(), sw.id) == 0)
    {
      sw.updateFromJson(doc);
      saveAndRefreshServices(switches, sw);
      return;
    }
  }
  SwitchT newSw;
  newSw.updateFromJson(doc);
  switches.items.push_back(newSw);
  saveAndRefreshServices(switches, newSw);
}

void load(Switches &switches)
{
  if (!SPIFFS.begin())
  {
#ifdef DEBUG
    Log.error("%s File storage can't start" CR, tags::sensors);
#endif
    return;
  }

  if (!SPIFFS.exists(configFilenames::switches))
  {
#ifdef DEBUG
    Log.notice("%s Default config loaded." CR, tags::switches);
#endif
#if defined SINGLE_SWITCH
    SwitchT one;
    templateSwitch(one, "Interruptor", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    switches.items.push_back(one);
#elif defined DUAL_LIGHT
    SwitchT one;
    SwitchT two;
    templateSwitch(one, "Interruptor 1", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    one.knxLevelOne = 0;
    one.knxLevelTwo = 0;
    one.knxLevelThree = 0;
    switches.items.push_back(one);
    templateSwitch(two, "Interruptor 2", constanstsSwitch::familyLight, SWITCH, 13u, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);
    two.knxLevelOne = 0;
    two.knxLevelTwo = 0;
    two.knxLevelThree = 0;
    switches.items.push_back(two);

#elif defined FOUR_LOCK
    SwitchT one;
    SwitchT two;
    SwitchT three;
    SwitchT four;
    templateSwitch(one, "Porta 1", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 14u, constantsConfig::noGPIO);
    templateSwitch(two, "Porta 2", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    templateSwitch(three, "Porta 3", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 12u, constantsConfig::noGPIO);
    templateSwitch(four, "Porta 4", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);

    one.autoStateDelay = 1000; //1 second
    strlcpy(one.autoStateValue, constanstsSwitch::payloadOff, sizeof(one.autoStateValue));
    two.autoStateDelay = 1000; //1 second
    strlcpy(two.autoStateValue, constanstsSwitch::payloadOff, sizeof(two.autoStateValue));
    three.autoStateDelay = 1000; //1 second
    strlcpy(three.autoStateValue, constanstsSwitch::payloadOff, sizeof(three.autoStateValue));
    four.autoStateDelay = 1000; //1 second
    strlcpy(four.autoStateValue, constanstsSwitch::payloadOff, sizeof(four.autoStateValue));
    switches.items.push_back(one);
    switches.items.push_back(two);
    switches.items.push_back(three);
    switches.items.push_back(four);
#elif defined VMC
    SwitchT one;
    SwitchT two;

    templateSwitch(one, "VMC", constanstsSwitch::familySwitch, PUSH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    one.autoStateDelay = 45 * 60 * 1000; //45 minutes

    strlcpy(one.autoStateValue, constanstsSwitch::payloadOff, sizeof(one.autoStateValue));

    templateSwitch(two, "BOMBA", constanstsSwitch::familySwitch, PUSH, 12u, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);
    two.autoStateDelay = 3 * 60 * 1000; //45 minutes

    strlcpy(two.autoStateValue, constanstsSwitch::payloadOff, sizeof(two.autoStateValue));

    switches.items.push_back(one);
    switches.items.push_back(two);

#elif defined COVER
    SwitchT one;
    templateSwitch(one, "Estore", constanstsSwitch::familyCover, DUAL_SWITCH, 12u, 13u, 4u, 5u);
    switches.items.push_back(one);
#elif defined LOCK
    SwitchT one;
    templateSwitch(one, "Portão", constanstsSwitch::familyLock, PUSH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    switches.items.push_back(one);
#elif defined GATE
    SwitchT one;
    templateSwitch(one, "Portão", constanstsSwitch::familyLock, PUSH, constantsConfig::noGPIO, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);
    one.autoStateDelay = 1000; //1 second
    strlcpy(one.autoStateValue, constanstsSwitch::payloadReleased, sizeof(one.autoStateValue));
    switches.items.push_back(one);
#endif
    SPIFFS.end();
    return;
  }

  File file = SPIFFS.open(configFilenames::switches, "r+");
  switches.load(file);
  file.close();
  SPIFFS.end();
#ifdef DEBUG
  Log.notice("%s Stored values was loaded." CR, tags::switches);
#endif
}

int findPoolIdx(const char *state, unsigned int currentIdx, unsigned int start, unsigned int end)
{
  int max = (end - start) * 2;
  unsigned int p = currentIdx;
  while (max > 0)
  {
    if (strcmp(state, statesPool[p].c_str()) == 0)
    {
      return p;
    }
    p = ((p - start + 1) % (end - start + 1)) + start;
    max--;
  }
  return start;
}

void mqttSwitchControl(Switches &switches, const char *topic, const char *payload)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(sw.mqttCommandTopic, topic) == 0)
    {

      for (unsigned int p = sw.statePoolStart; p <= sw.statePoolEnd; p++)
      {
        if (strcmp(payload, statesPool[p].c_str()) == 0)
        {
          sw.statePoolIdx = p;
          sw.changeState(statesPool[p].c_str());
          return;
        }
      }
      sw.changeState(payload);
    }
  }
}

void mqttCloudSwitchControl(Switches &switches, const char *topic, const char *payload)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(sw.mqttCloudCommandTopic, topic) == 0)
    {

      for (unsigned int p = sw.statePoolStart; p <= sw.statePoolEnd; p++)
      {
        if (strcmp(payload, statesPool[p].c_str()) == 0)
        {
          sw.statePoolIdx = p;
          sw.changeState(statesPool[p].c_str());
          return;
        }
      }
      sw.changeState(payload);
    }
  }
}

void SwitchT::changeState(const char *state)
{

#ifdef DEBUG
  Log.notice("%s Name:      %s" CR, tags::switches, name);
  Log.notice("%s State:     %s" CR, tags::switches, state);
  Log.notice("%s State IDX: %d" CR, tags::switches, statePoolIdx);
#endif
  bool isCover = strcmp(family, constanstsSwitch::familyCover) == 0;
  if (isCover)
  {
    if (strcmp(constanstsSwitch::payloadOpen, state) == 0)
    {
      shutter->setLevel(0);
    }
    else if (strcmp(constanstsSwitch::payloadStop, state) == 0)
    {
      shutter->stop();
    }
    else if (strcmp(constanstsSwitch::payloadClose, state) == 0)
    {
      shutter->setLevel(100);
    }
    else if (isValidNumber(state))
    {
      shutter->setLevel(max(0, min(100, atoi(state))));
    }
  }
  else
  {
    lastChangeState = millis();
    bool dirty = strcmp(state, stateControl);
    if (strcmp(constanstsSwitch::payloadOn, state) == 0)
    {
      strlcpy(stateControl, constanstsSwitch::payloadOn, sizeof(stateControl));
      strlcpy(mqttPayload, constanstsSwitch::payloadOn, sizeof(mqttPayload));
      if (typeControl == SwitchControlType::PIN_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON
      }
    }
    else if (strcmp(constanstsSwitch::payloadOff, state) == 0)
    {
      strlcpy(stateControl, constanstsSwitch::payloadOff, sizeof(stateControl));
      strlcpy(mqttPayload, constanstsSwitch::payloadOff, sizeof(mqttPayload));
      if (typeControl == SwitchControlType::PIN_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF
      }
    }
    else if (strcmp(constanstsSwitch::payloadLock, state) == 0 || strcmp(constanstsSwitch::payloadUnlock, state) == 0)
    {
      strlcpy(stateControl, state, sizeof(stateControl)); //TODO CHECK STATE FROM SENSOR
      strlcpy(mqttPayload, state, sizeof(mqttPayload));   //TODO CHECK STATE FROM SENSOR
      if (typeControl == SwitchControlType::PIN_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON
      }
    }
    else if (strcmp(constanstsSwitch::payloadReleased, state) == 0)
    {
      strlcpy(stateControl, state, sizeof(stateControl));                       //TODO CHECK STATE FROM SENSOR
      strlcpy(mqttPayload, constanstsSwitch::payloadLock, sizeof(mqttPayload)); //TODO CHECK STATE FROM SENSOR
      if (typeControl == SwitchControlType::PIN_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF
      }
    }
    notifyStateToCloudIO(mqttCloudStateTopic, mqttPayload);
    publishOnMqtt(mqttStateTopic, mqttPayload, true);
    if (dirty)
    {
      getAtualSwitchesConfig().lastChange = millis();
    }
  }

  String payloadSe;
  payloadSe.reserve(strlen(mqttPayload) + strlen(id) + 21);
  payloadSe.concat("{\"id\":\"");
  payloadSe.concat(id);
  payloadSe.concat("\",\"state\":\"");
  payloadSe.concat(mqttPayload);
  payloadSe.concat("\"}");
  sendToServerEvents("states", payloadSe.c_str());
  statePoolIdx = findPoolIdx(stateControl, statePoolIdx, statePoolStart, statePoolEnd);
  if (knxNotifyGroup && knxSupport)
  {
    knx.write_1byte_int(knx.GA_to_address(knxLevelOne, knxLevelTwo, knxLevelThree), statePoolIdx);
  }
  knxNotifyGroup = true;
}
void stateSwitchById(Switches &switches, const char *id, const char *state)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(id, sw.id) == 0)
    {
      sw.changeState(state);
    }
  }
}

bool stateTimeout(const SwitchT &sw)
{
  return sw.autoStateDelay > 0 && strlen(sw.autoStateValue) > 0 && strcmp(sw.autoStateValue, sw.stateControl) != 0 && sw.lastChangeState + sw.autoStateDelay < millis();
}
void loop(Switches &switches)
{

  if (switches.lastChange > 0ul && switches.lastChange + constantsConfig::storeConfigDelay < millis())
  {
    save(switches);
  }
  for (auto &sw : switches.items)
  {
    bool isCover = strcmp(sw.family, constanstsSwitch::familyCover) == 0;
    if (isCover)
    {
      sw.shutter->loop();
    }
    bool primaryGpioEvent = true;
    bool secondaryGpioEvent = true;
    if (sw.primaryGpio != constantsConfig::noGPIO)
    {
      sw.debouncerPrimary->update();
      primaryGpioEvent = sw.debouncerPrimary->read();
    }
    if (sw.secondaryGpio != constantsConfig::noGPIO)
    {
      sw.debouncerSecondary->update();
      secondaryGpioEvent = sw.debouncerSecondary->read();
    }

    int poolSize = sw.statePoolEnd - sw.statePoolStart + 1;
    switch (sw.mode)
    {
    case SWITCH:
      if (sw.primaryGpio != constantsConfig::noGPIO && sw.lastPrimaryGpioState != primaryGpioEvent)
      {
        sw.statePoolIdx = ((sw.statePoolIdx - sw.statePoolStart + 1) % poolSize) + sw.statePoolStart;
        sw.changeState(statesPool[sw.statePoolIdx].c_str());
      }
      break;
    case PUSH:
      if (sw.primaryGpio != constantsConfig::noGPIO && !primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
      { //PUSHED
        sw.statePoolIdx = ((sw.statePoolIdx - sw.statePoolStart + 1) % poolSize) + sw.statePoolStart;
        sw.changeState(statesPool[sw.statePoolIdx].c_str());
      }

      break;
    case DUAL_SWITCH:
      if (isCover && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
      {

        if (primaryGpioEvent && secondaryGpioEvent && (sw.lastPrimaryGpioState != primaryGpioEvent || sw.lastSecondaryGpioState != secondaryGpioEvent))
        {

          sw.statePoolIdx = sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        else if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
        {

          sw.statePoolIdx = constanstsSwitch::openIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        else if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
        {

          sw.statePoolIdx = constanstsSwitch::closeIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
      }
      break;
    case DUAL_PUSH:
      if (isCover && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
      {
        if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
        { //PUSHED

          sw.statePoolIdx = sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::firtStopIdx : constanstsSwitch::openIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
        { //PUSHED

          sw.statePoolIdx = sw.statePoolIdx == constanstsSwitch::closeIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::closeIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
      }
      break;
    default:
      break;
    }
    sw.lastPrimaryGpioState = primaryGpioEvent;
    sw.lastSecondaryGpioState = secondaryGpioEvent;
    if (stateTimeout(sw))
    {
      sw.statePoolIdx = findPoolIdx(sw.autoStateValue, sw.statePoolIdx, sw.statePoolStart, sw.statePoolEnd);
#ifdef DEBUG
      Log.notice("%s State Timeout set change switch to %s " CR, tags::switches, statesPool[sw.statePoolIdx].c_str());
#endif
      sw.changeState(statesPool[sw.statePoolIdx].c_str());
    }
  }
}
