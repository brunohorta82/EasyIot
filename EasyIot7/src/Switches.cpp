#include "Switches.h"
#include "Discovery.h"
#include "WebServer.h"
#include "constants.h"
static const String statesPool[] = {constanstsSwitch::payloadOff, constanstsSwitch::payloadOn, constanstsSwitch::payloadStateStop, constanstsSwitch::payloadOpen, constanstsSwitch::payloadStateStop, constanstsSwitch::payloadClose, constanstsSwitch::payloadReleased, constanstsSwitch::payloadUnlock, constanstsSwitch::payloadLock};
static Switches switches;
struct Switches &getAtualSwitchesConfig()
{
  return switches;
}
void saveSwitches()
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

  const size_t CAPACITY = JSON_ARRAY_SIZE(items.size()) + items.size() * JSON_OBJECT_SIZE(36) + 2200;
  DynamicJsonDocument doc(CAPACITY);
  for (const auto &sw : items)
  {
    JsonObject sdoc = doc.createNestedObject();
    sdoc["id"] = sw.id;
    sdoc["name"] = sw.name;
    sdoc["family"] = sw.family;
    sdoc["primaryGpio"] = sw.primaryGpio;
    sdoc["secondaryGpio"] = sw.secondaryGpio;
    sdoc["autoStateValue"] = String(sw.autoStateValue);
    sdoc["autoState"] = sw.autoState;
    sdoc["autoStateDelay"] = sw.autoStateDelay;
    sdoc["typeControl"] = sw.typeControl;
    sdoc["mode"] = sw.mode;
    sdoc["pullup"] = sw.pullup;
    sdoc["mqttRetain"] = sw.mqttRetain;
    sdoc["inverted"] = sw.inverted;
    sdoc["mqttCommandTopic"] = String(sw.mqttCommandTopic);
    sdoc["mqttStateTopic"] = String(sw.mqttStateTopic);
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
  file.write((uint8_t *)&autoState, sizeof(autoState));
  file.write((uint8_t *)&autoStateDelay, sizeof(autoStateDelay));
  file.write((uint8_t *)autoStateValue, sizeof(autoStateValue));
  file.write((uint8_t *)&timeBetweenStates, sizeof(timeBetweenStates));

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
  file.read((uint8_t *)&autoState, sizeof(autoState));
  file.read((uint8_t *)&autoStateDelay, sizeof(autoStateDelay));
  file.read((uint8_t *)autoStateValue, sizeof(autoStateValue));
  file.read((uint8_t *)&timeBetweenStates, sizeof(timeBetweenStates));

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
void Switches::load(File &file)
{
  auto n_items = items.size();
  file.read((uint8_t *)&n_items, sizeof(n_items));
  items.clear();
  items.resize(n_items);
  for (auto &item : items)
  {
    item.load(file);
    configPIN(item.primaryGpio, item.primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    configPIN(item.secondaryGpio, item.secondaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    configPIN(item.primaryGpioControl, OUTPUT);
    configPIN(item.secondaryGpioControl, OUTPUT);
    item.lastPrimaryGpioState = readPIN(item.primaryGpio);
    item.lastSecondaryGpioState = readPIN(item.secondaryGpio);
    if(item.alexaSupport){
      addSwitchToAlexa(item.name);
    }
  }
}
bool Switches::remove(const char *id) const
{
  int idx = 0;
  bool found = false;
  for (const auto &item : items)
  {
    if (strcmp(id, item.id) == 0)
    {
      found = true;
      publishOnMqtt(item.mqttCommandTopic, "", true);
      removeFromHaDiscovery(item);
      removeSwitchFromAlexa(item.name);
      unsubscribeOnMqtt(item.mqttCommandTopic);
      break;
    }
    idx++;
  }
  if (found)
  {
    switches.items.erase(switches.items.begin() + idx);
  }
  return found;
}

void removeSwitch(const char *id)
{
  if(switches.remove(id)) saveSwitches();
}

void initSwitchesHaDiscovery()
{
  for (const auto &sw : switches.items)
  {
    addToHaDiscovery(sw);
    publishOnMqtt(sw.mqttStateTopic, sw.mqttPayload, sw.mqttRetain);
  }
}

void SwitchT::updateFromJson(JsonObject doc)
{
  String n_name = doc["name"];
  normalize(n_name);
  String newId = doc.getMember("id").as<String>().equals(constantsConfig::newID) ? String(String(ESP.getChipId()) + n_name) : doc.getMember("id").as<String>();
  strlcpy(id, String(String(ESP.getChipId()) + n_name).c_str(), sizeof(id));
  strlcpy(name, doc.getMember("name").as<String>().c_str(), sizeof(name));
  strlcpy(family, doc.getMember("family").as<String>().c_str(), sizeof(family));
  primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  int switchMode = doc["mode"] | 0;
  mode = static_cast<SwitchMode>(switchMode);
  if (mode == SwitchMode::PUSH || mode == SwitchMode::SWITCH)
  {
    secondaryGpio = constantsConfig::noGPIO;
    lastSecondaryGpioState = true;
  }
  else if (mode == SwitchMode::DUAL_PUSH || mode == SwitchMode::DUAL_SWITCH)
  {
    secondaryGpio = doc["secondaryGpio"] | constantsConfig::noGPIO;
    lastSecondaryGpioState = doc["lastSecondaryGpioState"] | true;
  }
  timeBetweenStates = doc["timeBetweenStates"] | 0;
  autoState = (doc["autoStateDelay"] | 0) > 0 && strlen(doc["autoStateValue"] | "") > 0;
  autoStateDelay = doc["autoStateDelay"] | 0;
  strlcpy(autoStateValue, doc.getMember("autoStateValue").as<String>().c_str(), sizeof(autoStateValue));
  typeControl = static_cast<SwitchControlType>(doc["typeControl"] | static_cast<int>(SwitchControlType::MQTT));
  pullup = doc["pullup"] | true;
  mqttRetain = doc["mqttRetain"] | false;
  inverted = doc["inverted"] | false;
  String baseTopic = getBaseTopic() + "/" + String(family) + "/" + String(id);

  doc["mqttCommandTopic"] = String(baseTopic + "/set");
  doc["mqttStateTopic"] = String(baseTopic + "/state");
  strlcpy(mqttCommandTopic, String(baseTopic + "/set").c_str(), sizeof(mqttCommandTopic));
  strlcpy(mqttStateTopic, String(baseTopic + "/state").c_str(), sizeof(mqttStateTopic));
  primaryGpioControl = doc["primaryGpioControl"] | constantsConfig::noGPIO;

  if (pullup)
  {
    if (primaryGpio != constantsConfig::noGPIO)
    {
      configPIN(primaryGpio, primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    }
    if (secondaryGpio != constantsConfig::noGPIO)
    {
      configPIN(secondaryGpio, secondaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    }
  }
  if (strcmp(family, constanstsSwitch::familyCover) == 0)
  {
    secondaryGpioControl = doc["secondaryGpioControl"] | constantsConfig::noGPIO;
    statePoolStart = doc["statePoolStart"] | constanstsSwitch::coverStartIdx;
    statePoolEnd = doc["statePoolEnd"] | constanstsSwitch::converEndIdx;
    positionControlCover = doc["positionControlCover"] | 0;
    lastPercentage = doc["lastPercentage"] | 0;
    strlcpy(mqttPositionCommandTopic, String(baseTopic + "/setposition").c_str(), sizeof(mqttPositionCommandTopic));
    strlcpy(mqttPositionStateTopic, String(baseTopic + "/position").c_str(), sizeof(mqttPositionStateTopic));
    doc["mqttPositionCommandTopic"] = String(baseTopic + "/setposition");
    doc["mqttPositionStateTopic"] = String(baseTopic + "/position");
  }
  else if (strcmp(family, constanstsSwitch::familyLock) == 0)
  {
    secondaryGpioControl = constantsConfig::noGPIO;
    statePoolStart = doc["statePoolStart"] | constanstsSwitch::lockStartIdx;
    statePoolEnd = doc["statePoolEnd"] | constanstsSwitch::lockEndIdx;
  }
  else
  {
    secondaryGpioControl = constantsConfig::noGPIO;
    statePoolStart = doc["statePoolStart"] | constanstsSwitch::switchStartIdx;
    statePoolEnd = doc["statePoolEnd"] | constanstsSwitch::switchEndIdx;
  }
  statePoolIdx = doc["statePoolIdx"] | statePoolStart;

  strlcpy(mqttPayload, statesPool[statePoolIdx].c_str(), sizeof(mqttPayload));
  strlcpy(stateControl, statesPool[statePoolIdx].c_str(), sizeof(stateControl));
  lastPrimaryGpioState = doc["lastPrimaryGpioState"] | true;
  haSupport = true;
  alexaSupport = true;
  lastTimeChange = 0;
  percentageRequest = -1;
  doc["id"] = id;
  doc["stateControl"] = stateControl;
}
void saveAndRefreshServices(const SwitchT &sw)
{
  saveSwitches();
  removeFromHaDiscovery(sw);
  removeSwitchFromAlexa(sw.name);
  delay(10);
  if (sw.alexaSupport){
    addSwitchToAlexa(sw.name);
  }
  if (sw.haSupport){
    addToHaDiscovery(sw);
  }
}
void updateSwitch(const String &id, JsonObject doc)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(id.c_str(), sw.id) == 0)
    {
      sw.updateFromJson(doc);
      saveAndRefreshServices(sw);
      return;
    }
  }
  SwitchT newSw;
  newSw.updateFromJson(doc);
  switches.items.push_back(newSw);
  saveAndRefreshServices(newSw);
}

SwitchT createTemplateSwitch(const String &name, const char *family, const SwitchMode &mode, unsigned int primaryGpio, unsigned int secondaryGpio, unsigned int primaryGpioControl, unsigned int secondaryGpioControl)
{
  SwitchT sw;
  String id;
  id.reserve(sizeof(sw.id));
  id.concat(ESP.getChipId());
  id.concat(name);
  normalize(id);
  strlcpy(sw.id, id.c_str(), sizeof(sw.id));
  strlcpy(sw.name, name.c_str(), sizeof(sw.name));
  strlcpy(sw.family, family, sizeof(sw.family));
  sw.primaryGpio = primaryGpio;
  sw.secondaryGpio = secondaryGpio;
  sw.autoState = false;
  strlcpy(sw.autoStateValue, "", sizeof(sw.autoStateValue));
  sw.autoStateDelay = 0ul;
  sw.typeControl = RELAY_AND_MQTT;
  sw.mode = mode;
  sw.haSupport = true;
  sw.alexaSupport = true;
  sw.pullup = true;
  sw.mqttRetain = false;
  sw.inverted = false;
  String mqttTopic;
  mqttTopic.reserve(sizeof(sw.mqttCommandTopic));
  mqttTopic.concat("easyiot/");
  mqttTopic.concat(ESP.getChipId());
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

  sw.timeBetweenStates = 0ul;

  sw.primaryGpioControl = primaryGpioControl;
  sw.secondaryGpioControl = secondaryGpioControl;

  configPIN(sw.primaryGpio, sw.primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
  configPIN(sw.secondaryGpio, sw.secondaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
  configPIN(sw.primaryGpioControl, OUTPUT);
  configPIN(sw.secondaryGpioControl, OUTPUT);
  sw.lastPrimaryGpioState = readPIN(sw.primaryGpio);
  sw.lastSecondaryGpioState = readPIN(sw.secondaryGpio);
  strlcpy(sw.stateControl, constanstsSwitch::payloadOff, sizeof(sw.mqttPayload));
  strlcpy(sw.mqttPayload, sw.stateControl, sizeof(sw.mqttPayload));
  sw.lastTimeChange = 0ul;
  return sw;
}
void loadStoredSwitches()
{
  if (!SPIFFS.begin())
  {
    Log.error("%s File storage can't start" CR, tags::config);
    return;
  }

  if (!SPIFFS.exists(configFilenames::switches))
  {
    Log.notice("%s Default config loaded." CR, tags::switches);
#if defined SINGLE_SWITCH
    switches.items.push_back(createTemplateSwitch("Interruptor", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO));
#elif defined DUAL_LIGHT
    switches.items.push_back(createTemplateSwitch("Interruptor 1", constanstsSwitch::familyLight, SWITCH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO));
    switches.items.push_back(createTemplateSwitch("Interruptor 2", constanstsSwitch::familyLight, SWITCH, 13u, constantsConfig::noGPIO, 5u, constantsConfig::noGPIO));
#elif defined COVER
    switches.items.push_back(createTemplateSwitch("Estore", constanstsSwitch::familyCover, DUAL_SWITCH, 12u, 13u, 4u, 5u));
#elif defined LOCK
    switches.items.push_back(createTemplateSwitch("Portão", constanstsSwitch::familyLock, PUSH, 12u, constantsConfig::noGPIO, 4u, constantsConfig::noGPIO));
#endif
    SPIFFS.end();
    saveSwitches();
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

void mqttSwitchControl(const char *topic, const char *payload)
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

void SwitchT::changeState(const char *state)
{
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

  sendToServerEvents("states", String("{\"id\":\"") + String(id) + String("\",\"state\":\"") + String(mqttPayload) + String("\"}"));
  lastTimeChange = millis();
  statePoolIdx = findPoolIdx(stateControl, statePoolIdx, statePoolStart, statePoolEnd);
  if(dirty) switches.lastChange = millis();
}
void stateSwitchById(const char *id, const char *state)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(id, sw.id) == 0)
    {
      sw.changeState(state);
    }
  }
}
void stateSwitchByName(const char *name, const char *state, const char *value)
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
  return (sw.autoState && strcmp(sw.autoStateValue, sw.stateControl) != 0 && (sw.lastTimeChange + sw.autoStateDelay) < millis());
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
void loopSwitches()
{
  if(switches.lastChange > 0 && switches.lastChange + constantsConfig::storeConfigDelay < millis()){
    saveSwitches();
  }
  for (auto &sw : switches.items)
  {
    bool primaryValue = readPIN(sw.primaryGpio);
    bool secondaryValue = readPIN(sw.secondaryGpio);
    unsigned long currentTime = millis();

    bool primaryGpioEvent = primaryValue != sw.lastPrimaryGpioState;
    bool secondaryGpioEvent = secondaryValue != sw.lastSecondaryGpioState;

    if ((primaryGpioEvent || secondaryGpioEvent) && currentTime - sw.lastTimeChange >= constanstsSwitch::delayDebounce)
    {
      sw.lastPrimaryGpioState = primaryValue;
      sw.lastSecondaryGpioState = secondaryValue;
      sw.lastTimeChange = currentTime;
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
      Log.notice("%s State Timeout set change switch to %s " CR, statesPool[sw.statePoolIdx].c_str());
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
