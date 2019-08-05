#include "Switches.h"
#include "Discovery.h"
#include "constants.h"

static Switches switchs;
static const String statesPool[] = {constanstsSwitch::payloadOff, constanstsSwitch::payloadOn, constanstsSwitch::payloadStateStop, constanstsSwitch::payloadOpen, constanstsSwitch::payloadStateStop, constanstsSwitch::payloadClose, constanstsSwitch::payloadReleased, constanstsSwitch::payloadUnlock, constanstsSwitch::payloadLock};
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
  }
}
void removeSwitch(const char *id)
{
  Log.notice("%s Remove id %s " CR, tags::switches, id);

  unsigned int del = constantsConfig::noGPIO;
  for (unsigned int i = 0; i < switchs.items.size(); i++)
  {
    SwitchT swStored = switchs.items[i];
    if (strcmp(id, swStored.id) == 0)
    {
      publishOnMqtt(swStored.mqttCommandTopic, "", true);
      removeFromHaDiscovery(&swStored);
      removeFromAlexaDiscovery(&swStored);
      unsubscribeOnMqtt(swStored.mqttCommandTopic);
      del = i;
    }
  }
  if (del != constantsConfig::noGPIO)
  {
    switchs.items.erase(switchs.items.begin() + del);
  }
 
}
void initSwitchesHaDiscovery()
{
  for (unsigned int i = 0; i < switchs.items.size(); i++)
  {
    addToHaDiscovery(&switchs.items[i]);
    publishOnMqtt(switchs.items[i].mqttStateTopic, switchs.items[i].mqttPayload, switchs.items[i].mqttRetain);
  }
}
void SwitchT::update(JsonObject doc, bool persist)
{

  Log.notice("%s Update Environment" CR, tags::switches);
  String n_name = doc["name"];
  normalize(n_name);
  String newId = doc.getMember("id").as<String>().equals(constantsConfig::newID) ? String(String(ESP.getChipId()) + n_name) : doc.getMember("id").as<String>();
  if (persist)
    removeSwitch(doc["id"], false);
  SwitchT sw;
  strlcpy(sw.id, String(String(ESP.getChipId()) + n_name).c_str(), sizeof(sw.id));
  strlcpy(sw.name, doc.getMember("name").as<String>().c_str(), sizeof(sw.name));
  strlcpy(sw.family, doc.getMember("family").as<String>().c_str(), sizeof(sw.family));
  sw.primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  int switchMode = doc["mode"] | 0;
  sw.mode = static_cast<SwitchMode>(switchMode);
  if (sw.mode == SwitchMode::PUSH || sw.mode == SwitchMode::SWITCH)
  {
    sw.secondaryGpio = constantsConfig::noGPIO;
    sw.lastSecondaryGpioState = true;
  }
  else if (sw.mode == SwitchMode::DUAL_PUSH || sw.mode == SwitchMode::DUAL_SWITCH)
  {
    sw.secondaryGpio = doc["secondaryGpio"] | constantsConfig::noGPIO;
    sw.lastSecondaryGpioState = doc["lastSecondaryGpioState"] | true;
  }
  sw.timeBetweenStates = doc["timeBetweenStates"] | 0;
  sw.autoState = (doc["autoStateDelay"] | 0) > 0 && strlen(doc["autoStateValue"] | "") > 0;
  sw.autoStateDelay = doc["autoStateDelay"] | 0;
  strlcpy(sw.autoStateValue, doc.getMember("autoStateValue").as<String>().c_str(), sizeof(sw.autoStateValue));
  sw.typeControl = static_cast<SwitchControlType>(doc["typeControl"] | static_cast<int>(SwitchControlType::MQTT));
  sw.pullup = doc["pullup"] | true;
  sw.mqttRetain = doc["mqttRetain"] | false;
  sw.inverted = doc["inverted"] | false;
  String baseTopic = getBaseTopic() + "/" + String(sw.family) + "/" + String(sw.id);

  doc["mqttCommandTopic"] = String(baseTopic + "/set");
  doc["mqttStateTopic"] = String(baseTopic + "/state");
  strlcpy(sw.mqttCommandTopic, String(baseTopic + "/set").c_str(), sizeof(sw.mqttCommandTopic));
  strlcpy(sw.mqttStateTopic, String(baseTopic + "/state").c_str(), sizeof(sw.mqttStateTopic));
  sw.primaryGpioControl = doc["primaryGpioControl"] | constantsConfig::noGPIO;

  if (sw.pullup)
  {
    if (sw.primaryGpio != constantsConfig::noGPIO)
    {
      configPIN(sw.primaryGpio, sw.primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    }
    if (sw.secondaryGpio != constantsConfig::noGPIO)
    {
      configPIN(sw.secondaryGpio, sw.secondaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    }
  }
  if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
  {
    sw.secondaryGpioControl = doc["secondaryGpioControl"] | constantsConfig::noGPIO;
    sw.statePoolStart = doc["statePoolStart"] | constanstsSwitch::coverStartIdx;
    sw.statePoolEnd = doc["statePoolEnd"] | constanstsSwitch::converEndIdx;
    sw.positionControlCover = doc["positionControlCover"] | 0;
    sw.lastPercentage = doc["lastPercentage"] | 0;
    strlcpy(sw.mqttPositionCommandTopic, String(baseTopic + "/setposition").c_str(), sizeof(sw.mqttPositionCommandTopic));
    strlcpy(sw.mqttPositionStateTopic, String(baseTopic + "/position").c_str(), sizeof(sw.mqttPositionStateTopic));
    doc["mqttPositionCommandTopic"] = String(baseTopic + "/setposition");
    doc["mqttPositionStateTopic"] = String(baseTopic + "/position");
  }
  else if (strcmp(sw.family, constanstsSwitch::familyLock) == 0)
  {
    sw.secondaryGpioControl = constantsConfig::noGPIO;
    sw.statePoolStart = doc["statePoolStart"] | constanstsSwitch::lockStartIdx;
    sw.statePoolEnd = doc["statePoolEnd"] | constanstsSwitch::lockEndIdx;
  }
  else
  {
    sw.secondaryGpioControl = constantsConfig::noGPIO;
    sw.statePoolStart = doc["statePoolStart"] | constanstsSwitch::switchStartIdx;
    sw.statePoolEnd = doc["statePoolEnd"] | constanstsSwitch::switchEndIdx;
  }
  sw.statePoolIdx = doc["statePoolIdx"] | sw.statePoolStart;

  strlcpy(sw.mqttPayload, statesPool[sw.statePoolIdx].c_str(), sizeof(sw.mqttPayload));
  strlcpy(sw.stateControl, statesPool[sw.statePoolIdx].c_str(), sizeof(sw.stateControl));
  sw.lastPrimaryGpioState = doc["lastPrimaryGpioState"] | true;

  sw.lastTimeChange = 0;
  sw.percentageRequest = -1;
  addToAlexaDiscovery(&sw);
  addToHaDiscovery(&sw);
  doc["id"] = sw.id;
  doc["stateControl"] = sw.stateControl;
  switchs.items.push_back(sw);
  stateSwitch(&sw, sw.stateControl);
}
void updateSwitch(const String &id,JsonObject doc){

}
void loadStoredSwitches()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(configFilenames::switches, "r+");
    int switchesSize = 3; //TODO GET THIS VALUE FROM FILE
    const size_t CAPACITY = JSON_ARRAY_SIZE(switchesSize + 1) + switchesSize * JSON_OBJECT_SIZE(36) + 3000;
    DynamicJsonDocument doc(CAPACITY);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      file.close();
      file = SPIFFS.open(configFilenames::switches, "w+");
      Log.warning("%s Default values was loaded." CR, tags::switches);
#if defined SINGLE_SWITCH
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor\",\"name\":\"Novo Interruptor\",\"family\":\"light\",\"primaryGpio\":12,\"secondaryGpio\":99,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":1,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":4,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":0,\"statePoolStart\":0,\"statePoolEnd\":1,\"mqttPayload\":\"OFF\",\"stateControl\":\"OFF\"}]").c_str());
#elif defined DUAL_SWITCH
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor1\",\"name\":\"Novo Interruptor 1\",\"family\":\"light\",\"primaryGpio\":12,\"secondaryGpio\":99,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":1,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor1/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor1/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":4,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":0,\"statePoolStart\":0,\"statePoolEnd\":1,\"mqttPayload\":\"OFF\",\"stateControl\":\"OFF\"},{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor2\",\"name\":\"Novo Interruptor 2\",\"family\":\"light\",\"primaryGpio\":13,\"secondaryGpio\":99,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":1,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor2/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor2/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":5,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":0,\"statePoolStart\":0,\"statePoolEnd\":1,\"mqttPayload\":\"OFF\",\"stateControl\":\"OFF\"}]").c_str());
#elif defined COVER
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor\",\"name\":\"Novo Interruptor\",\"family\":\"cover\",\"primaryGpio\":12,\"secondaryGpio\":13,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":4,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/state\",\"mqttPositionCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/setposition\",\"mqttPositionStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/position\",\"percentageRequest\":-1,\"lastPercentage\":0,\"positionControlCover\":0,\"secondaryGpioControl\":5,\"timeBetweenStates\":0,\"primaryGpioControl\":4,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":2,\"statePoolStart\":" + String(constanstsSwitch::coverStartIdx) + ",\"statePoolEnd\":" + String(constanstsSwitch::converEndIdx) + ",\"mqttPayload\":\"STOP\",\"stateControl\":\"STOP\"}]").c_str());
#elif defined LOCK
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "fechadura1\",\"name\":\"Fechadura 1\",\"family\":\"lock\",\"primaryGpio\":99,\"secondaryGpio\":99,\"autoStateValue\":\"RELEASED\",\"autoState\":true,\"autoStateDelay\":1000,\"typeControl\":1,\"mode\":2,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/lock/" + String(ESP.getChipId()) + "fechadura1/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/lock/" + String(ESP.getChipId()) + "fechadura1/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":5,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":6,\"statePoolStart\":6,\"statePoolEnd\":8,\"mqttPayload\":\"\",\"stateControl\":\"RELEASED\"}]"));
#else
      file.print(String("[]").c_str());
#endif
      file.close();
      SPIFFS.end();
      loadStoredSwitches();
    }
    else
    {
      Log.notice("%s Stored values was loaded." CR, tags::switches);
    }
    file.close();
    JsonArray ar = doc.as<JsonArray>();
    for (JsonVariant sw : ar)
    {
      updateSwitch("",sw.as<JsonObject>());
    }
  }
  SPIFFS.end();
}

void saveSwitchs()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(configFilenames::switches, "w+");
    if (!file)
    {
      Log.warning("%s Open Switches config file Error!." CR, tags::switches);
    }
    else
    {
      const size_t CAPACITY = JSON_ARRAY_SIZE(switchs.items.size() + 1) + switchs.items.size() * JSON_OBJECT_SIZE(36) + 2200;
      ;
      DynamicJsonDocument doc(CAPACITY);
      for (unsigned int i = 0; i < switchs.items.size(); i++)
      {
        JsonObject sdoc = doc.createNestedObject();
        SwitchT sw = switchs.items[i];
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
          sdoc["positionControlCover"] = sw.positionControlCover; //COVER PERCENTAGE

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

      if (serializeJson(doc.as<JsonArray>(), file) == 0)
      {
        Log.error("%s Failed to write Switches Config into file" CR, tags::switches);
      }
      else
      {
        Log.notice("%s Switches Config stored." CR, tags::switches);
      }
    }
    file.close();
  }
  SPIFFS.end();
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
  for (unsigned int i = 0; i < switchs.items.size(); i++)
  {
    if (strcmp(switchs.items[i].mqttCommandTopic, topic) == 0)
    {

      for (unsigned int p = switchs.items[i].statePoolStart; p <= switchs.items[i].statePoolEnd; p++)
      {
        if (strcmp(payload, statesPool[p].c_str()) == 0)
        {
          switchs.items[i].statePoolIdx = p;
          stateSwitch(&switchs.items[i], statesPool[p].c_str());
          return;
        }
      }
      stateSwitch(&switchs.items[i], payload);
    }
  }
}

void stateSwitch(SwitchT *switchT, const char *state)
{
  Log.notice("%s Name:      %s" CR, tags::switches, switchT->name);
  Log.notice("%s State:     %s" CR, tags::switches, state);
  Log.notice("%s State IDX: %d" CR, tags::switches, switchT->statePoolIdx);

  if (strcmp(constanstsSwitch::payloadOpen, state) == 0)
  {
    if (switchT->timeBetweenStates > 0 && switchT->positionControlCover >= 100)
    {
      return;
    }
    switchT->lastPercentage = switchT->positionControlCover;
    strlcpy(switchT->stateControl, constanstsSwitch::payloadOpen, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, constanstsSwitch::payloadOpen, sizeof(switchT->mqttPayload));
    if (switchT->percentageRequest < 0)
      switchT->percentageRequest = 100;
    if (switchT->typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      configPIN(switchT->secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->secondaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON -> OPEN REQUEST
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON -> EXECUTE REQUEST
    }

    publishOnMqtt(switchT->mqttPositionStateTopic, String(switchT->percentageRequest).c_str(), switchT->mqttRetain);
  }
  else if (strcmp(constanstsSwitch::payloadStop, state) == 0)
  {
    strlcpy(switchT->stateControl, constanstsSwitch::payloadStop, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, constanstsSwitch::payloadStateStop, sizeof(switchT->mqttPayload));
    switchT->percentageRequest = -1;
    switchT->lastPercentage = switchT->positionControlCover;
    if (switchT->typeControl == SwitchControlType::RELAY_AND_MQTT)
    {

      configPIN(switchT->primaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(constanstsSwitch::delayCoverProtection);
      configPIN(switchT->secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->secondaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> PROTECT RELAY LIFE
      delay(constanstsSwitch::delayCoverProtection);
    }
    if (switchT->timeBetweenStates > 0)
    {
      publishOnMqtt(switchT->mqttPositionStateTopic, String(switchT->lastPercentage).c_str(), switchT->mqttRetain);
    }
    else
    {
      publishOnMqtt(switchT->mqttPositionStateTopic, "50", switchT->mqttRetain);
    }
  }
  else if (strcmp(constanstsSwitch::payloadClose, state) == 0)
  {

    if (switchT->timeBetweenStates > 0 && switchT->positionControlCover <= 0)
    {
      return;
    }
    switchT->lastPercentage = switchT->positionControlCover;
    strlcpy(switchT->stateControl, constanstsSwitch::payloadClose, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, constanstsSwitch::payloadStateClose, sizeof(switchT->mqttPayload));
    if (switchT->percentageRequest < 0)
      switchT->percentageRequest = 0;
    if (switchT->typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      configPIN(switchT->secondaryGpioControl, OUTPUT);
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->secondaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> CLOSE REQUEST
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON -> EXECUTE REQUEST
    }

    publishOnMqtt(switchT->mqttPositionStateTopic, String(switchT->percentageRequest).c_str(), switchT->mqttRetain);
  }
  else if (strcmp(constanstsSwitch::payloadOn, state) == 0)
  {
    strlcpy(switchT->stateControl, constanstsSwitch::payloadOn, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, constanstsSwitch::payloadOn, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON
    }
  }
  else if (strcmp(constanstsSwitch::payloadOff, state) == 0)
  {
    strlcpy(switchT->stateControl, constanstsSwitch::payloadOff, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, constanstsSwitch::payloadOff, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF
    }
  }
  else if (strcmp(constanstsSwitch::payloadLock, state) == 0 || strcmp(constanstsSwitch::payloadUnlock, state) == 0)
  {
    strlcpy(switchT->stateControl, state, sizeof(switchT->stateControl)); //TODO CHECK STATE FROM SENSOR
    strlcpy(switchT->mqttPayload, state, sizeof(switchT->mqttPayload));   //TODO CHECK STATE FROM SENSOR
    if (switchT->typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON
    }
  }
  else if (strcmp(constanstsSwitch::payloadReleased, state) == 0)
  {
    strlcpy(switchT->stateControl, state, sizeof(switchT->stateControl));                       //TODO CHECK STATE FROM SENSOR
    strlcpy(switchT->mqttPayload, constanstsSwitch::payloadLock, sizeof(switchT->mqttPayload)); //TODO CHECK STATE FROM SENSOR
    if (switchT->typeControl == SwitchControlType::RELAY_AND_MQTT)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF
    }
  }
  else if (isValidNumber(state))
  {

    switchT->percentageRequest = max(0, min(100, atoi(state)));
    if (switchT->positionControlCover > switchT->percentageRequest)
    {
      switchT->statePoolIdx = constanstsSwitch::closeIdx;
      stateSwitch(switchT, statesPool[switchT->statePoolIdx].c_str());
    }
    else
    {
      switchT->statePoolIdx = constanstsSwitch::openIdx;
      stateSwitch(switchT, statesPool[switchT->statePoolIdx].c_str());
    }
  }
  publishOnMqtt(switchT->mqttStateTopic, switchT->mqttPayload, switchT->mqttRetain);

  sendToServerEvents("states", String("{\"id\":\"") + String(switchT->id) + String("\",\"state\":\"") + String(switchT->mqttPayload) + String("\"}"));
  switchT->lastTimeChange = millis();
  switchT->statePoolIdx = findPoolIdx(switchT->stateControl, switchT->statePoolIdx, switchT->statePoolStart, switchT->statePoolEnd);
  saveSwitchs();
}
void stateSwitchById(const char *id, const char *state)
{
  for (unsigned int i = 0; i < switchs.items.size(); i++)
  {
    if (strcmp(id, switchs.items[i].id) == 0)
    {
      stateSwitch(&switchs.items[i], state);
    }
  }
}
void stateSwitchByName(const char *name, const char *state, const char *value)
{
  for (unsigned int i = 0; i < switchs.items.size(); i++)
  {
    if (strcmp(switchs.items[i].name, name) == 0)
    {
      if (strcmp(constanstsSwitch::familyCover, switchs.items[i].family) == 0)
      {
        if (switchs.items[i].timeBetweenStates > 0)
        {
          stateSwitch(&switchs.items[i], value);
        }
        else
        {
          stateSwitch(&switchs.items[i], state);
        }
      }
      else if (strcmp(constanstsSwitch::familyLight, switchs.items[i].family) == 0 || strcmp(constanstsSwitch::familySwitch, switchs.items[i].family) == 0)
      {

        stateSwitch(&switchs.items[i], state);
      }
    }
  }
}

bool stateTimeout(SwitchT *sw)
{
  return (sw->autoState && strcmp(sw->autoStateValue, sw->stateControl) != 0 && (sw->lastTimeChange + sw->autoStateDelay) < millis());
}
boolean positionDone(SwitchT *sw)
{
  if (strcmp(sw->family, constanstsSwitch::familyCover) != 0)
  {
    return false;
  }
  if (strcmp(constanstsSwitch::payloadStop, sw->stateControl) == 0)
  {
    return false;
  }
  if (sw->lastTimeChange + constanstsSwitch::coverAutoStopProtection < millis())
  {
    return true;
  }
  if (sw->timeBetweenStates == 0)
  {
    return false;
  }

  if (sw->percentageRequest == sw->positionControlCover)
  {
    return true;
  }
  return false;
}
void loopSwitches()
{
  for (unsigned int i = 0; i < switchs.items.size(); i++)
  {

    SwitchT *sw = &switchs.items[i];

    bool primaryValue = readPIN(sw->primaryGpio);
    bool secondaryValue = readPIN(sw->secondaryGpio);
    unsigned long currentTime = millis();

    bool primaryGpioEvent = primaryValue != sw->lastPrimaryGpioState;
    bool secondaryGpioEvent = secondaryValue != sw->lastSecondaryGpioState;

    if ((primaryGpioEvent || secondaryGpioEvent) && currentTime - sw->lastTimeChange >= constanstsSwitch::delayDebounce)
    {
      sw->lastPrimaryGpioState = primaryValue;
      sw->lastSecondaryGpioState = secondaryValue;
      sw->lastTimeChange = currentTime;
      int poolSize = sw->statePoolEnd - sw->statePoolStart + 1;

      switch (sw->mode)
      {
      case SWITCH:
        sw->statePoolIdx = ((sw->statePoolIdx - sw->statePoolStart + 1) % poolSize) + sw->statePoolStart;
        stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
        break;
      case PUSH:
        if (!primaryValue)
        { //PUSHED
          sw->statePoolIdx = ((sw->statePoolIdx - sw->statePoolStart + 1) % poolSize) + sw->statePoolStart;
          stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
        }
        break;
      case DUAL_SWITCH:
        if (primaryValue == true && secondaryValue == true)
        {
          sw->statePoolIdx = sw->statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx;
          stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
        }
        else if (primaryGpioEvent && !primaryValue)
        {
          sw->statePoolIdx = constanstsSwitch::openIdx;
          stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
        }
        else if (secondaryGpioEvent && !secondaryValue)
        {
          sw->statePoolIdx = constanstsSwitch::closeIdx;
          stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
        }
        break;
      case DUAL_PUSH:
        if (!primaryValue)
        { //PUSHED
          sw->statePoolIdx = sw->statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::firtStopIdx : constanstsSwitch::openIdx;
          stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
        }
        if (!secondaryValue)
        { //PUSHED
          sw->statePoolIdx = sw->statePoolIdx == constanstsSwitch::closeIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::closeIdx;
          stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
        }
        break;
      default:
        break;
      }
    }

    if (sw->primaryGpioControl != constantsConfig::noGPIO && sw->timeBetweenStates != 0 && digitalRead(sw->primaryGpioControl))
    {
      unsigned long offset = ((sw->lastPercentage * sw->timeBetweenStates) / 100);
      unsigned long deltaTime = (millis() - sw->lastTimeChange);
      if (sw->secondaryGpioControl != constantsConfig::noGPIO)
      {
        if (digitalRead(sw->secondaryGpioControl))
        {
          //OPEN
          sw->positionControlCover = max(0ul, ((offset + deltaTime) * 100) / sw->timeBetweenStates);
        }
        else
        { //CLOSE
          sw->positionControlCover = min(100ul, ((offset - deltaTime) * 100) / sw->timeBetweenStates);
        }
      }
    }

    if (stateTimeout(sw))
    {
      sw->statePoolIdx = findPoolIdx(sw->autoStateValue, sw->statePoolIdx, sw->statePoolStart, sw->statePoolEnd);
      Log.notice("%s State Timeout set change switch to %s " CR, statesPool[sw->statePoolIdx].c_str());
      stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
    }
    if (positionDone(sw))
    {
      sw->statePoolIdx = sw->statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx;
      sw->percentageRequest = -1;
      Log.notice("%s Control Positon set change switch to  %s" CR, tags::switches, statesPool[sw->statePoolIdx].c_str());
      stateSwitch(sw, statesPool[sw->statePoolIdx].c_str());
    }
  }
}
