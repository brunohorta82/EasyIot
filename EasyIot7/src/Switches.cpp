#include "Switches.h"
#include "Discovery.h"
#include "WebServer.h"
#include "constants.h"
#include "Config.h"
#include "Mqtt.h"
#include "WebRequests.h"
#include <esp-knx-ip.h>
static const String statesPool[] = {constanstsSwitch::payloadOff, constanstsSwitch::payloadOn, constanstsSwitch::payloadStop, constanstsSwitch::payloadOpen, constanstsSwitch::payloadStop, constanstsSwitch::payloadClose, constanstsSwitch::payloadReleased, constanstsSwitch::payloadUnlock, constanstsSwitch::payloadLock};

struct Switches &getAtualSwitchesConfig()
{
  static Switches switches;
  return switches;
}

void save(Switches &switches)
{
  if (!SPIFFS.begin())
  {
    Log.error("%s File storage can't start" CR, tags::switches);
    return;
  }

  File file = SPIFFS.open(configFilenames::switches, "w+");
  if (!file)
  {
    Log.error("%s Failed to create file" CR, tags::switches);
    return;
  }
  switches.save(file);
  file.close();
  SPIFFS.end();
  Log.notice("%s Config stored." CR, tags::switches);
  switches.lastChange = 0ul;
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
      sdoc["mqttPositionCommandTopic"] = sw.mqttPositionCommandTopic;
      sdoc["mqttPositionStateTopic"] = sw.mqttPositionStateTopic;
      sdoc["percentageRequest"] = sw.percentageRequest;
      sdoc["lastPercentage"] = sw.lastPercentage;
      sdoc["positionControlCover"] = sw.positionControlCover;
      sdoc["secondaryGpioControl"] = sw.secondaryGpioControl;
    }
    sdoc["timeBetweenStates"] = sw.timeBetweenStates;
    sdoc["primaryGpioControl"] = sw.primaryGpioControl;

    sdoc["lastPrimaryGpioState"] = sw.lastPrimaryGpioState;
    sdoc["lastSecondaryGpioState"] = sw.lastSecondaryGpioState;

    sdoc["statePoolIdx"] = sw.statePoolIdx;
    sdoc["statePoolStart"] = sw.statePoolStart;
    sdoc["statePoolEnd"] = sw.statePoolEnd;
    sdoc["mqttPayload"] = sw.mqttPayload;
    sdoc["stateControl"] = sw.stateControl;

    sdoc["alexaSupport"] = sw.alexaSupport;
    sdoc["haSupport"] = sw.haSupport;
  }
  return serializeJson(doc, output);
}

void SwitchT::save(File &file) const
{

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
  file.write((uint8_t *)&timeBetweenStates, sizeof(timeBetweenStates));
  file.write((uint8_t *)&childLock, sizeof(childLock));

  //MQTT
  file.write((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.write((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.write((uint8_t *)mqttPositionStateTopic, sizeof(mqttPositionStateTopic));
  file.write((uint8_t *)mqttPositionCommandTopic, sizeof(mqttPositionCommandTopic));
  file.write((uint8_t *)mqttPayload, sizeof(mqttPayload));
  file.write((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  //CONTROL VARIABLES
  file.write((uint8_t *)stateControl, sizeof(stateControl));
  file.write((uint8_t *)&positionControlCover, sizeof(positionControlCover));
  file.write((uint8_t *)&lastPercentage, sizeof(lastPercentage));

  file.write((uint8_t *)&lastPrimaryGpioState, sizeof(lastPrimaryGpioState));
  file.write((uint8_t *)&lastSecondaryGpioState, sizeof(lastSecondaryGpioState));
  file.write((uint8_t *)&lastTimeChange, sizeof(lastTimeChange));
  file.write((uint8_t *)&percentageRequest, sizeof(percentageRequest));
  file.write((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
  file.write((uint8_t *)&statePoolStart, sizeof(statePoolStart));
  file.write((uint8_t *)&statePoolEnd, sizeof(statePoolEnd));

  file.write((uint8_t *)&alexaSupport, sizeof(alexaSupport));
  file.write((uint8_t *)&haSupport, sizeof(haSupport));
  file.write((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.write((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.write((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));
}
void SwitchT::load(File &file)
{
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
  file.read((uint8_t *)&timeBetweenStates, sizeof(timeBetweenStates));
  file.read((uint8_t *)&childLock, sizeof(childLock));

  //MQTT
  file.read((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.read((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.read((uint8_t *)mqttPositionStateTopic, sizeof(mqttPositionStateTopic));
  file.read((uint8_t *)mqttPositionCommandTopic, sizeof(mqttPositionCommandTopic));
  file.read((uint8_t *)mqttPayload, sizeof(mqttPayload));
  file.read((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  //CONTROL VARIABLES
  file.read((uint8_t *)stateControl, sizeof(stateControl));
  file.read((uint8_t *)&positionControlCover, sizeof(positionControlCover));
  file.read((uint8_t *)&lastPercentage, sizeof(lastPercentage));

  file.read((uint8_t *)&lastPrimaryGpioState, sizeof(lastPrimaryGpioState));
  file.read((uint8_t *)&lastSecondaryGpioState, sizeof(lastSecondaryGpioState));
  lastTimeChange = 0;
  file.read((uint8_t *)&lastTimeChange, sizeof(lastTimeChange));
  percentageRequest = -1;
  file.read((uint8_t *)&percentageRequest, sizeof(percentageRequest));
  file.read((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
  file.read((uint8_t *)&statePoolStart, sizeof(statePoolStart));
  file.read((uint8_t *)&statePoolEnd, sizeof(statePoolEnd));

  file.read((uint8_t *)&alexaSupport, sizeof(alexaSupport));
  file.read((uint8_t *)&haSupport, sizeof(haSupport));
  file.read((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.read((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.read((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));
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
    bool state = (bool)msg.data[0];
    s->knxNotifyGroup = false;
    s->changeState(state ? constanstsSwitch::payloadOn : constanstsSwitch::payloadOff);
    break;
  }
  case KNX_CT_READ:
  {
    Serial.println("Read");
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
    bool state = (bool)msg.data[0];
    for (auto &sw : getAtualSwitchesConfig().items)
    {
      sw.knxNotifyGroup = false;
      sw.changeState(state ? constanstsSwitch::payloadOn : constanstsSwitch::payloadOff);
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
    if (item.knxLevelOne > 0 && item.knxLevelTwo > 0 && item.knxLevelThree > 0)
    {
      if (!globalKnxLevelThreeAssign)
      {
        knx.callback_assign(knx.callback_register("ALL SWITCHES", allwitchesCallback), knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, 0));
        globalKnxLevelThreeAssign = true;
      }
      item.knxIdRegister = knx.callback_register(String(item.name), switchesCallback, &item);
      item.knxIdAssign = knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, item.knxLevelThree));
    }

    configPIN(item.primaryGpio, item.primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    configPIN(item.secondaryGpio, item.secondaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    configPIN(item.primaryGpioControl, OUTPUT);
    configPIN(item.secondaryGpioControl, OUTPUT);
    item.lastPrimaryGpioState = readPIN(item.primaryGpio);
    item.lastSecondaryGpioState = readPIN(item.secondaryGpio);
    if (item.alexaSupport)
    {
      addSwitchToAlexa(item.name);
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
  removeSwitchFromAlexa(match->name);
  unsubscribeOnMqtt(match->mqttCommandTopic);

  items.erase(match);

  return true;
}

void remove(Switches &switches, const char *id)
{
  if (switches.remove(id))
    save(switches);
}

void templateSwitch(SwitchT &sw, const String &name, const char *family, const SwitchMode &mode, unsigned int primaryGpio, unsigned int secondaryGpio, unsigned int primaryGpioControl, unsigned int secondaryGpioControl, bool mqttRetaint = false, unsigned long autoStateDelay = 0ul, const String &autoStateValue = "", const SwitchControlType &typecontrol = RELAY_AND_MQTT, unsigned long timeBetweenStates = 0ul, bool haSupport = true, bool alexaSupport = true, uint8_t knxLevelOne = 0, uint8_t knxLevelTwo = 0, uint8_t knxLevelThree = 0)
{
  String idStr;
  generateId(idStr, name, sizeof(sw.id));
  strlcpy(sw.id, idStr.c_str(), sizeof(sw.id));
  strlcpy(sw.name, name.c_str(), sizeof(sw.name));
  strlcpy(sw.family, family, sizeof(sw.family));
  sw.primaryGpio = primaryGpio;
  sw.secondaryGpio = secondaryGpio;
  sw.autoStateDelay = autoStateDelay;
  strlcpy(sw.autoStateValue, autoStateValue.c_str(), sizeof(sw.autoStateValue));
  sw.typeControl = typecontrol;
  sw.mode = mode;
  sw.haSupport = haSupport;
  sw.alexaSupport = alexaSupport;
  sw.pullup = true;
  sw.mqttRetain = mqttRetaint;
  sw.inverted = false;
  String mqttTopic;
  mqttTopic.reserve(sizeof(sw.mqttCommandTopic));
  mqttTopic.concat(getBaseTopic());
  mqttTopic.concat("/");
  mqttTopic.concat(sw.family);
  mqttTopic.concat("/");
  mqttTopic.concat(sw.id);
  mqttTopic.concat("/set");
  strlcpy(sw.mqttCommandTopic, mqttTopic.c_str(), sizeof(sw.mqttCommandTopic));
  mqttTopic.replace("/set", "/state");
  strlcpy(sw.mqttStateTopic, mqttTopic.c_str(), sizeof(sw.mqttStateTopic));
  if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
  {
    mqttTopic.replace("/state", "/setposition");
    strlcpy(sw.mqttPositionCommandTopic, mqttTopic.c_str(), sizeof(sw.mqttPositionCommandTopic));
    mqttTopic.replace("/setposition", "/position");
    strlcpy(sw.mqttPositionStateTopic, mqttTopic.c_str(), sizeof(sw.mqttPositionStateTopic));
    sw.statePoolStart = constanstsSwitch::coverStartIdx;
    sw.statePoolEnd = constanstsSwitch::converEndIdx;
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

  sw.timeBetweenStates = timeBetweenStates;

  sw.primaryGpioControl = primaryGpioControl;
  sw.secondaryGpioControl = secondaryGpioControl;

  configPIN(sw.primaryGpio, sw.primaryGpio == 16u ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
  configPIN(sw.secondaryGpio, sw.secondaryGpio == 16u ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
  configPIN(sw.primaryGpioControl, OUTPUT);
  configPIN(sw.secondaryGpioControl, OUTPUT);
  sw.lastPrimaryGpioState = readPIN(sw.primaryGpio);
  sw.lastSecondaryGpioState = readPIN(sw.secondaryGpio);
  strlcpy(sw.stateControl, constanstsSwitch::payloadOff, sizeof(sw.mqttPayload));
  strlcpy(sw.mqttPayload, sw.stateControl, sizeof(sw.mqttPayload));
  sw.lastTimeChange = 0ul;
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
  templateSwitch(*this, doc["name"], doc["family"], static_cast<SwitchMode>(doc["mode"] | static_cast<int>(SWITCH)), doc["primaryGpio"] | constantsConfig::noGPIO, doc["secondaryGpio"] | constantsConfig::noGPIO, doc["primaryGpioControl"] | constantsConfig::noGPIO, doc["secondaryGpioControl"] | constantsConfig::noGPIO, doc["mqttRetain"] | false, doc["autoStateDelay"] | 0ul, doc["autoStateValue"] | "", static_cast<SwitchControlType>(doc["typeControl"] | static_cast<int>(SwitchControlType::MQTT)), doc["timeBetweenStates"] | 0ul, doc["haSupport"] | true, doc["alexaSupport"] | true, doc["knxLevelOne"] | 0, doc["knxLevelTwo"] | 0, doc["knxLevelThree"] | 0);
  doc["id"] = id;
  doc["stateControl"] = stateControl;
  doc["mqttCommandTopic"] = mqttCommandTopic;
  doc["mqttStateTopic"] = mqttStateTopic;
  if (strcmp(family, constanstsSwitch::familyCover) == 0)
  {
    doc["mqttPositionCommandTopic"] = mqttPositionCommandTopic;
    doc["mqttPositionStateTopic"] = mqttPositionStateTopic;
  }
}
void saveAndRefreshServices(Switches &switches, const SwitchT &sw)
{
  save(switches);
  removeFromHaDiscovery(sw);
  removeSwitchFromAlexa(sw.name);
  delay(10);
  if (sw.alexaSupport)
  {
    addSwitchToAlexa(sw.name);
  }
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
    Log.error("%s File storage can't start" CR, tags::sensors);
    return;
  }

  if (!SPIFFS.exists(configFilenames::switches))
  {
    Log.notice("%s Default config loaded." CR, tags::switches);
#if defined SINGLE_SWITCH
    SwitchT one;
    templateSwitch(one, "Interruptor", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    switches.items.push_back(one);
#elif defined DUAL_LIGHT
    SwitchT one;
    SwitchT two;
    templateSwitch(one, "Interruptor 1", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO);
    one.knxLevelOne = 2;
    one.knxLevelTwo = 1;
    one.knxLevelThree = 1;
    switches.items.push_back(one);
    templateSwitch(two, "Interruptor 2", constanstsSwitch::familyLight, SWITCH, 13u, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO);
    two.knxLevelOne = 2;
    two.knxLevelTwo = 1;
    two.knxLevelThree = 2;
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
  Log.notice("%s Stored values was loaded." CR, tags::switches);
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
void knkGroupNotifyState(const SwitchT &sw, const char *state)
{
}
void SwitchT::changeState(const char *state)
{
  if (knxNotifyGroup)
  {
    knx.write_1bit(knx.GA_to_address(knxLevelOne, knxLevelTwo, knxLevelThree), strcmp(state, constanstsSwitch::payloadOn) == 0);
  }
  knxNotifyGroup = true;
  bool dirty = strcmp(state, stateControl);
  Log.notice("%s Name:      %s" CR, tags::switches, name);
  Log.notice("%s State:     %s" CR, tags::switches, state);
  Log.notice("%s State IDX: %d" CR, tags::switches, statePoolIdx);

  if (strcmp(constanstsSwitch::payloadOpen, state) == 0)
  {
    if (timeBetweenStates > 0 && positionControlCover >= 100)
    {
      return;
    }
    lastPercentage = positionControlCover;
    strlcpy(stateControl, constanstsSwitch::payloadOpen, sizeof(stateControl));
    strlcpy(mqttPayload, constanstsSwitch::payloadOpen, sizeof(mqttPayload));
    if (percentageRequest < 0)
      percentageRequest = 100;
    if (typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(primaryGpioControl, OUTPUT);
      configPIN(secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF . STOP
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(secondaryGpioControl, inverted ? LOW : HIGH); //TURN ON . OPEN REQUEST
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON . EXECUTE REQUEST
    }
    publishOnMqtt(mqttPositionStateTopic, String(percentageRequest).c_str(), mqttRetain);
  }
  else if (strcmp(constanstsSwitch::payloadStop, state) == 0)
  {
    strlcpy(stateControl, constanstsSwitch::payloadStop, sizeof(stateControl));
    strlcpy(mqttPayload, constanstsSwitch::payloadStateStop, sizeof(mqttPayload));
    percentageRequest = -1;
    lastPercentage = positionControlCover;
    if (typeControl == SwitchControlType::RELAY_AND_MQTT)
    {

      configPIN(primaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF . STOP
      delay(constanstsSwitch::delayCoverProtection);
      configPIN(secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(secondaryGpioControl, inverted ? HIGH : LOW); //TURN OFF . PROTECT RELAY LIFE
      delay(constanstsSwitch::delayCoverProtection);
    }
    if (timeBetweenStates > 0)
    {
      publishOnMqtt(mqttPositionStateTopic, String(lastPercentage).c_str(), mqttRetain);
    }
    else
    {
      publishOnMqtt(mqttPositionStateTopic, "50", mqttRetain);
    }
  }
  else if (strcmp(constanstsSwitch::payloadClose, state) == 0)
  {

    if (timeBetweenStates > 0 && positionControlCover <= 0)
    {
      return;
    }
    lastPercentage = positionControlCover;
    strlcpy(stateControl, constanstsSwitch::payloadClose, sizeof(stateControl));
    strlcpy(mqttPayload, constanstsSwitch::payloadStateClose, sizeof(mqttPayload));
    if (percentageRequest < 0)
      percentageRequest = 0;
    if (typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(primaryGpioControl, OUTPUT);
      configPIN(secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF . STOP
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(secondaryGpioControl, inverted ? HIGH : LOW); //TURN OFF . CLOSE REQUEST
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON . EXECUTE REQUEST
    }
    publishOnMqtt(mqttPositionStateTopic, String(percentageRequest).c_str(), mqttRetain);
  }
  else if (strcmp(constanstsSwitch::payloadOn, state) == 0)
  {
    strlcpy(stateControl, constanstsSwitch::payloadOn, sizeof(stateControl));
    strlcpy(mqttPayload, constanstsSwitch::payloadOn, sizeof(mqttPayload));
    if (typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(primaryGpioControl, OUTPUT);
      writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON
    }
  }
  else if (strcmp(constanstsSwitch::payloadOff, state) == 0)
  {
    strlcpy(stateControl, constanstsSwitch::payloadOff, sizeof(stateControl));
    strlcpy(mqttPayload, constanstsSwitch::payloadOff, sizeof(mqttPayload));
    if (typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(primaryGpioControl, OUTPUT);
      writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF
    }
  }
  else if (strcmp(constanstsSwitch::payloadLock, state) == 0 || strcmp(constanstsSwitch::payloadUnlock, state) == 0)
  {
    strlcpy(stateControl, state, sizeof(stateControl)); //TODO CHECK STATE FROM SENSOR
    strlcpy(mqttPayload, state, sizeof(mqttPayload));   //TODO CHECK STATE FROM SENSOR
    if (typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(primaryGpioControl, OUTPUT);
      writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON
    }
  }
  else if (strcmp(constanstsSwitch::payloadReleased, state) == 0)
  {
    strlcpy(stateControl, state, sizeof(stateControl));                       //TODO CHECK STATE FROM SENSOR
    strlcpy(mqttPayload, constanstsSwitch::payloadLock, sizeof(mqttPayload)); //TODO CHECK STATE FROM SENSOR
    if (typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(primaryGpioControl, OUTPUT);
      writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF
    }
  }
  else if (isValidNumber(state))
  {

    percentageRequest = max(0, min(100, atoi(state)));
    if (positionControlCover > percentageRequest)
    {
      statePoolIdx = constanstsSwitch::closeIdx;
      changeState(statesPool[statePoolIdx].c_str());
    }
    else
    {
      statePoolIdx = constanstsSwitch::openIdx;
      changeState(statesPool[statePoolIdx].c_str());
    }
  }

  publishOnMqtt(mqttStateTopic, mqttPayload, mqttRetain);
  String payloadSe;
  payloadSe.reserve(strlen(mqttPayload) + strlen(id) + 21);
  payloadSe.concat("{\"id\":\"");
  payloadSe.concat(id);
  payloadSe.concat("\",\"state\":\"");
  payloadSe.concat(mqttPayload);
  payloadSe.concat("\"}");
  sendToServerEvents("states", payloadSe);
  lastTimeChange = millis();
  statePoolIdx = findPoolIdx(stateControl, statePoolIdx, statePoolStart, statePoolEnd);
  if (dirty)
    getAtualSwitchesConfig().lastChange = millis();
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
void stateSwitchByName(Switches &switches, const char *name, const char *state, const char *value)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(sw.name, name) == 0)
    {
      if (strcmp(constanstsSwitch::familyCover, sw.family) == 0)
      {
        if (sw.timeBetweenStates > 0)
        {
          sw.changeState(value);
        }
        else
        {
          sw.changeState(state);
        }
      }
      else if (strcmp(constanstsSwitch::familyLight, sw.family) == 0 || strcmp(constanstsSwitch::familySwitch, sw.family) == 0)
      {

        sw.changeState(state);
      }
    }
  }
}

bool stateTimeout(const SwitchT &sw)
{
  return sw.autoStateDelay > 0 && strlen(sw.autoStateValue) > 0 && strcmp(sw.autoStateValue, sw.stateControl) != 0 && sw.lastTimeChange + sw.autoStateDelay < millis();
}
boolean positionDone(const SwitchT &sw)
{
  if (strcmp(sw.family, constanstsSwitch::familyCover) != 0)
  {
    return false;
  }
  if (strcmp(constanstsSwitch::payloadStop, sw.stateControl) == 0)
  {
    return false;
  }
  if (sw.lastTimeChange + constanstsSwitch::coverAutoStopProtection < millis())
  {
    return true;
  }
  if (sw.timeBetweenStates == 0)
  {
    return false;
  }

  if (sw.percentageRequest == sw.positionControlCover)
  {
    return true;
  }
  return false;
}

void loop(Switches &switches)
{

  if (switches.lastChange > 0 && switches.lastChange + constantsConfig::storeConfigDelay < millis())
  {
    save(switches);
  }
  for (auto &sw : switches.items)
  {
    bool primaryValue = readPIN(sw.primaryGpio);
    bool secondaryValue = readPIN(sw.secondaryGpio);
    unsigned long currentTime = millis();

    bool primaryGpioEvent = primaryValue != sw.lastPrimaryGpioState;
    bool secondaryGpioEvent = secondaryValue != sw.lastSecondaryGpioState;

    if ((primaryGpioEvent || secondaryGpioEvent))
    {
      if (currentTime - sw.lastTimeChange <= constanstsSwitch::delayDebounce)
      {
        sw.lastTimeChange = currentTime;
        return;
      }

      sw.lastPrimaryGpioState = primaryValue;
      sw.lastSecondaryGpioState = secondaryValue;

      int poolSize = sw.statePoolEnd - sw.statePoolStart + 1;

      switch (sw.mode)
      {
      case SWITCH:
        sw.statePoolIdx = ((sw.statePoolIdx - sw.statePoolStart + 1) % poolSize) + sw.statePoolStart;
        sw.changeState(statesPool[sw.statePoolIdx].c_str());
        break;
      case PUSH:
        if (!primaryValue)
        { //PUSHED
          sw.statePoolIdx = ((sw.statePoolIdx - sw.statePoolStart + 1) % poolSize) + sw.statePoolStart;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        break;
      case DUAL_SWITCH:
        if (primaryValue == true && secondaryValue == true)
        {
          sw.statePoolIdx = sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        else if (primaryGpioEvent && !primaryValue)
        {
          sw.statePoolIdx = constanstsSwitch::openIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        else if (secondaryGpioEvent && !secondaryValue)
        {
          sw.statePoolIdx = constanstsSwitch::closeIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        break;
      case DUAL_PUSH:
        if (!primaryValue)
        { //PUSHED
          sw.statePoolIdx = sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::firtStopIdx : constanstsSwitch::openIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        if (!secondaryValue)
        { //PUSHED
          sw.statePoolIdx = sw.statePoolIdx == constanstsSwitch::closeIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::closeIdx;
          sw.changeState(statesPool[sw.statePoolIdx].c_str());
        }
        break;
      default:
        break;
      }
    }

    if (sw.primaryGpioControl != constantsConfig::noGPIO && sw.timeBetweenStates != 0 && digitalRead(sw.primaryGpioControl))
    {
      unsigned long offset = ((sw.lastPercentage * sw.timeBetweenStates) / 100);
      unsigned long deltaTime = (millis() - sw.lastTimeChange);
      if (sw.secondaryGpioControl != constantsConfig::noGPIO)
      {
        if (digitalRead(sw.secondaryGpioControl))
        {
          //OPEN
          sw.positionControlCover = max(0ul, ((offset + deltaTime) * 100) / sw.timeBetweenStates);
        }
        else
        { //CLOSE
          sw.positionControlCover = min(100ul, ((offset - deltaTime) * 100) / sw.timeBetweenStates);
        }
      }
    }

    if (stateTimeout(sw))
    {
      sw.statePoolIdx = findPoolIdx(sw.autoStateValue, sw.statePoolIdx, sw.statePoolStart, sw.statePoolEnd);
      Log.notice("%s State Timeout set change switch to %s " CR, tags::switches, statesPool[sw.statePoolIdx].c_str());
      sw.changeState(statesPool[sw.statePoolIdx].c_str());
    }
    if (positionDone(sw))
    {
      sw.statePoolIdx = sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx;
      sw.percentageRequest = -1;
      Log.notice("%s Control Positon set change switch to  %s" CR, tags::switches, statesPool[sw.statePoolIdx].c_str());
      sw.changeState(statesPool[sw.statePoolIdx].c_str());
    }
  }
}
