#include "Switches.h"
#include "Mqtt.h"
#include "Discovery.h"

std::vector<SwitchT> switchs;

void removeSwitch(String id, bool persist)
{
  logger(SWITCHES_TAG, "Remove switch " + id);

  unsigned int del = NO_GPIO;
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
    SwitchT swStored = switchs[i];
    if (strcmp(id.c_str(), swStored.id) == 0)
    {
      publishOnMqtt(swStored.mqttCommandTopic, "", true);
      removeFromHaDiscovery(&swStored);
      removeFromAlexaDiscovery(&swStored);
      unsubscribeOnMqtt(String(swStored.mqttCommandTopic));
      del = i;
    }
  }
  if (del != NO_GPIO)
  {
    switchs.erase(switchs.begin() + del);
  }
  if (persist)
  {
    saveSwitchs();
  }
}
void initSwitchesHaDiscovery()
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
    addToHaDiscovery(&switchs[i]);
    publishOnMqtt(switchs[i].mqttStateTopic, switchs[i].mqttPayload, switchs[i].mqttRetain);
  }
}
JsonObject updateSwitches(JsonObject doc, bool persist)
{

  logger(SWITCHES_TAG, "Update Environment Switches");
  String newId = doc.getMember("id").as<String>().equals(NEW_ID) ? String(String(ESP.getChipId()) + normalize(doc.getMember("name").as<String>())) : doc.getMember("id").as<String>();
  if (persist)
    removeSwitch(doc.getMember("id").as<String>(), false);
  SwitchT sw;
  strlcpy(sw.id, String(String(ESP.getChipId()) + normalize(doc.getMember("name").as<String>())).c_str(), sizeof(sw.id));
  strlcpy(sw.name, doc.getMember("name").as<String>().c_str(), sizeof(sw.name));
  strlcpy(sw.family, doc.getMember("family").as<String>().c_str(), sizeof(sw.family));
  sw.primaryGpio = doc["primaryGpio"] | NO_GPIO;
  sw.mode = doc["mode"] | MODE_SWITCH;
  if (sw.mode == MODE_PUSH || sw.mode == MODE_SWITCH)
  {
    sw.secondaryGpio = NO_GPIO;
    sw.lastSecondaryGpioState = true;
  }
  else if (sw.mode == MODE_DUAL_PUSH || sw.mode == MODE_DUAL_SWITCH)
  {
    sw.secondaryGpio = doc["secondaryGpio"] | NO_GPIO;
    sw.lastSecondaryGpioState = doc["lastSecondaryGpioState"] | true;
  }
  sw.timeBetweenStates = doc["timeBetweenStates"] | 0;
  sw.autoState = (doc["autoStateDelay"] | 0) > 0 && strlen(doc["autoStateValue"] | "") > 0;
  sw.autoStateDelay = doc["autoStateDelay"] | 0;
  strlcpy(sw.autoStateValue, doc.getMember("autoStateValue").as<String>().c_str(), sizeof(sw.autoStateValue));
  sw.typeControl = doc["typeControl"] | TYPE_MQTT;
  sw.pullup = doc["pullup"] | true;
  sw.mqttRetain = doc["mqttRetain"] | false;
  sw.inverted = doc["inverted"] | false;
  String baseTopic = getBaseTopic() + "/" + String(sw.family) + "/" + String(sw.id);

  doc["mqttCommandTopic"] = String(baseTopic + "/set");
  doc["mqttStateTopic"] = String(baseTopic + "/state");
  strlcpy(sw.mqttCommandTopic, String(baseTopic + "/set").c_str(), sizeof(sw.mqttCommandTopic));
  strlcpy(sw.mqttStateTopic, String(baseTopic + "/state").c_str(), sizeof(sw.mqttStateTopic));
  sw.primaryGpioControl = doc["primaryGpioControl"] | NO_GPIO;

  if (sw.pullup)
  {
    if (sw.primaryGpio != NO_GPIO)
    {
      configPIN(sw.primaryGpio, sw.primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    }
    if (sw.secondaryGpio != NO_GPIO)
    {
      configPIN(sw.secondaryGpio, sw.secondaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    }
  }
  if (String(sw.family).equals(String(FAMILY_COVER)))
  {
    sw.secondaryGpioControl = doc["secondaryGpioControl"] | NO_GPIO;
    sw.statePoolStart = doc["statePoolStart"] | COVER_START_IDX;
    sw.statePoolEnd = doc["statePoolEnd"] | COVER_END_IDX;
    sw.positionControlCover = doc["positionControlCover"] | 0;
    sw.lastPercentage = doc["lastPercentage"] | 0;
    strlcpy(sw.mqttPositionCommandTopic, String(baseTopic + "/setposition").c_str(), sizeof(sw.mqttPositionCommandTopic));
    strlcpy(sw.mqttPositionStateTopic, String(baseTopic + "/position").c_str(), sizeof(sw.mqttPositionStateTopic));
    doc["mqttPositionCommandTopic"] = String(baseTopic + "/setposition");
    doc["mqttPositionStateTopic"] = String(baseTopic + "/position");
  }
  else if (String(sw.family).equals(String(FAMILY_LOCK)))
  {
    sw.secondaryGpioControl = NO_GPIO;
    sw.statePoolStart = doc["statePoolStart"] | LOCK_START_IDX;
    sw.statePoolEnd = doc["statePoolEnd"] | LOCK_END_IDX;
  }
  else
  {
    sw.secondaryGpioControl = NO_GPIO;
    sw.statePoolStart = doc["statePoolStart"] | SWITCH_START_IDX;
    sw.statePoolEnd = doc["statePoolEnd"] | SWITCH_END_IDX;
  }
  sw.statePoolIdx = doc["statePoolIdx"] | sw.statePoolStart;

  strlcpy(sw.mqttPayload, statesPool[sw.statePoolIdx].c_str(), sizeof(sw.mqttPayload));
  strlcpy(sw.stateControl, statesPool[sw.statePoolIdx].c_str(), sizeof(sw.stateControl));
  sw.lastPrimaryGpioState = doc["lastPrimaryGpioState"] | true;

  sw.lastTimeChange = doc["lastTimeChange"] | 0;
  sw.percentageRequest = doc["percentageRequest"] | -1;
  addToAlexaDiscovery(&sw);
  doc["id"] = String(sw.id);
  doc["stateControl"] = String(sw.stateControl);
  switchs.push_back(sw);
  stateSwitch(&sw, sw.stateControl);
  return doc;
}

void loadStoredSwitchs()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(SWITCHES_CONFIG_FILENAME, "r+");
    const size_t CAPACITY = JSON_ARRAY_SIZE(switchs.size() + 1) + switchs.size() * JSON_OBJECT_SIZE(36) + 2200;
    DynamicJsonDocument doc(CAPACITY);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {

      file.close();
      file = SPIFFS.open(SWITCHES_CONFIG_FILENAME, "w+");
      logger(SWITCHES_TAG, "Default switches loaded.");
#if defined SINGLE_SWITCH
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor\",\"name\":\"Novo Interruptor\",\"family\":\"light\",\"primaryGpio\":12,\"secondaryGpio\":99,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":1,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":4,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":0,\"statePoolStart\":0,\"statePoolEnd\":1,\"mqttPayload\":\"OFF\",\"stateControl\":\"OFF\"}]").c_str());
#elif defined DUAL_SWITCH
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor1\",\"name\":\"Novo Interruptor 1\",\"family\":\"light\",\"primaryGpio\":12,\"secondaryGpio\":99,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":1,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor1/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor1/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":4,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":0,\"statePoolStart\":0,\"statePoolEnd\":1,\"mqttPayload\":\"OFF\",\"stateControl\":\"OFF\"},{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor2\",\"name\":\"Novo Interruptor 2\",\"family\":\"light\",\"primaryGpio\":13,\"secondaryGpio\":99,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":1,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor2/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/light/" + String(ESP.getChipId()) + "novointerruptor2/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":5,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":0,\"statePoolStart\":0,\"statePoolEnd\":1,\"mqttPayload\":\"OFF\",\"stateControl\":\"OFF\"}]").c_str());
#elif defined COVER
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "novointerruptor\",\"name\":\"Novo Interruptor\",\"family\":\"cover\",\"primaryGpio\":12,\"secondaryGpio\":13,\"autoStateValue\":\"\",\"autoState\":false,\"autoStateDelay\":0,\"typeControl\":1,\"mode\":4,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/state\",\"mqttPositionCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/setposition\",\"mqttPositionStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/cover/" + String(ESP.getChipId()) + "novointerruptor/position\",\"percentageRequest\":-1,\"lastPercentage\":0,\"positionControlCover\":0,\"secondaryGpioControl\":5,\"timeBetweenStates\":0,\"primaryGpioControl\":4,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":2,\"statePoolStart\":" + String(COVER_START_IDX) + ",\"statePoolEnd\":" + String(COVER_END_IDX) + ",\"mqttPayload\":\"STOP\",\"stateControl\":\"STOP\"}]").c_str());
#elif defined LOCK
      file.print(String("[{\"id\":\"" + String(ESP.getChipId()) + "fechadura1\",\"name\":\"Fechadura 1\",\"family\":\"lock\",\"primaryGpio\":99,\"secondaryGpio\":99,\"autoStateValue\":\"RELEASED\",\"autoState\":true,\"autoStateDelay\":1000,\"typeControl\":1,\"mode\":2,\"pullup\":true,\"mqttRetain\":false,\"inverted\":false,\"mqttCommandTopic\":\"easyiot/" + String(ESP.getChipId()) + "/lock/" + String(ESP.getChipId()) + "fechadura1/set\",\"mqttStateTopic\":\"easyiot/" + String(ESP.getChipId()) + "/lock/" + String(ESP.getChipId()) + "fechadura1/state\",\"timeBetweenStates\":0,\"primaryGpioControl\":5,\"lastPrimaryGpioState\":true,\"lastSecondaryGpioState\":true,\"lastTimeChange\":0,\"statePoolIdx\":6,\"statePoolStart\":6,\"statePoolEnd\":8,\"mqttPayload\":\"\",\"stateControl\":\"RELEASED\"}]"));
#else
      file.print(String("[]").c_str());
#endif
      file.close();
      SPIFFS.end();
      loadStoredSwitchs();
    }
    else
    {
      logger(SWITCHES_TAG, "Stored switches loaded.");
    }
    file.close();
    JsonArray ar = doc.as<JsonArray>();
    for (JsonVariant sw : ar)
    {
      updateSwitches(sw.as<JsonObject>(), error);
    }
  }
  SPIFFS.end();
}

void saveSwitchs()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(SWITCHES_CONFIG_FILENAME, "w+");
    if (!file)
    {
      logger(SWITCHES_TAG, "Open Switches config file Error!");
    }
    else
    {
      const size_t CAPACITY = JSON_ARRAY_SIZE(switchs.size() + 1) + switchs.size() * JSON_OBJECT_SIZE(36) + 2200;
      ;
      DynamicJsonDocument doc(CAPACITY);
      for (unsigned int i = 0; i < switchs.size(); i++)
      {
        JsonObject sdoc = doc.createNestedObject();
        SwitchT sw = switchs[i];
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
        if (String(sw.family).equals("cover"))
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

        sdoc["lastTimeChange"] = sw.lastTimeChange;

        sdoc["statePoolIdx"] = sw.statePoolIdx;
        sdoc["statePoolStart"] = sw.statePoolStart;
        sdoc["statePoolEnd"] = sw.statePoolEnd;
        sdoc["mqttPayload"] = sw.mqttPayload;
        sdoc["stateControl"] = sw.stateControl;
      }

      if (serializeJson(doc.as<JsonArray>(), file) == 0)
      {
        logger(SWITCHES_TAG, "Failed to write Switches Config into file");
      }
      else
      {
        logger(SWITCHES_TAG, "Switches Config stored.");
      }
    }
    file.close();
  }
  SPIFFS.end();
}

String getSwitchesConfigStatus()
{
  String object = "";
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(SWITCHES_CONFIG_FILENAME, "r+");

    if (!file)
    {
      return "[]";
    }
    while (file.available())
    {
      object += (char)file.read();
    }
    file.close();
  }
  SPIFFS.end();
  if (object.equals("") || object.equals("null"))
    return "[]";
  return object;
}
int findPoolIdx(String state, unsigned int currentIdx, unsigned int start, unsigned int end)
{
  int max = (end - start)*2 ;
  unsigned int p = currentIdx;
  while (max > 0)
  {
    if (strcmp(state.c_str(), statesPool[p].c_str()) == 0)
    {
      return p;
    }
    p = ((p - start + 1) % (end - start + 1)) + start;
    max--;
  }
  return start;
}

void mqttSwitchControl(String topic, String payload)
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
    if (strcmp(switchs[i].mqttCommandTopic, topic.c_str()) == 0)
    {

      for (unsigned int p = switchs[i].statePoolStart; p <= switchs[i].statePoolEnd; p++)
      {
        if (strcmp(payload.c_str(), statesPool[p].c_str()) == 0)
        {
          switchs[i].statePoolIdx = p;
          stateSwitch(&switchs[i], statesPool[p]);
          return;
        }
      }
      stateSwitch(&switchs[i], payload);
    }
  }
}
void configPIN(uint8_t pin, uint8_t mode)
{
  if (pin == NO_GPIO)
  {
    return;
  }
  pinMode(pin, mode);
}
void writeToPIN(uint8_t pin, uint8_t val)
{
  if (pin == NO_GPIO)
  {
    return;
  }
  digitalWrite(pin, val);
}
bool readPIN(uint8_t pin)
{
  if (pin == NO_GPIO)
  {
    return true;
  }
  return digitalRead(pin);
}
void stateSwitch(SwitchT *switchT, String state)
{
  logger(SWITCHES_TAG, "Name:      " + String(switchT->name));
  logger(SWITCHES_TAG, "State:     " + String(state));
  logger(SWITCHES_TAG, "State IDX: " + String(switchT->statePoolIdx));

  if (String(PAYLOAD_OPEN).equals(state))
  {
    if (switchT->timeBetweenStates > 0 && switchT->positionControlCover >= 100)
    {
      return;
    }
    switchT->lastPercentage = switchT->positionControlCover;
    strlcpy(switchT->stateControl, PAYLOAD_OPEN, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_OPEN, sizeof(switchT->mqttPayload));
    if (switchT->percentageRequest < 0)
      switchT->percentageRequest = 100;
    if (switchT->typeControl == TYPE_RELAY)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      configPIN(switchT->secondaryGpioControl, OUTPUT);
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->secondaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON -> OPEN REQUEST
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON -> EXECUTE REQUEST
    }

    publishOnMqtt(switchT->mqttPositionStateTopic, String(switchT->percentageRequest), switchT->mqttRetain);
  }
  else if (String(PAYLOAD_STOP).equals(state))
  {
    strlcpy(switchT->stateControl, PAYLOAD_STOP, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_STOP, sizeof(switchT->mqttPayload));
    switchT->percentageRequest = -1;
    switchT->lastPercentage = switchT->positionControlCover;
    if (switchT->typeControl == TYPE_RELAY)
    {
      
      configPIN(switchT->primaryGpioControl, OUTPUT);
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(DELAY_COVER_PROTECTION);
      configPIN(switchT->secondaryGpioControl, OUTPUT);
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->secondaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> PROTECT RELAY LIFE
      delay(DELAY_COVER_PROTECTION);
    }
    if (switchT->timeBetweenStates > 0)
    {
      publishOnMqtt(switchT->mqttPositionStateTopic, String(switchT->lastPercentage), switchT->mqttRetain);
    }
    else
    {
      publishOnMqtt(switchT->mqttPositionStateTopic, "50", switchT->mqttRetain);
    }
  }
  else if (String(PAYLOAD_CLOSE).equals(state))
  {

    if (switchT->timeBetweenStates > 0 && switchT->positionControlCover <= 0)
    {
      return;
    }
    switchT->lastPercentage = switchT->positionControlCover;
    strlcpy(switchT->stateControl, PAYLOAD_CLOSE, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_CLOSE, sizeof(switchT->mqttPayload));
    if (switchT->percentageRequest < 0)
      switchT->percentageRequest = 0;
    if (switchT->typeControl == TYPE_RELAY)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      configPIN(switchT->secondaryGpioControl, OUTPUT);
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->secondaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> CLOSE REQUEST
      delay(DELAY_COVER_PROTECTION);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON -> EXECUTE REQUEST
    }

    publishOnMqtt(switchT->mqttPositionStateTopic, String(switchT->percentageRequest), switchT->mqttRetain);
  }
  else if (String(PAYLOAD_ON).equals(state))
  {
    strlcpy(switchT->stateControl, PAYLOAD_ON, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_ON, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON
    }
  }
  else if (String(PAYLOAD_OFF).equals(state))
  {
    strlcpy(switchT->stateControl, PAYLOAD_OFF, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_OFF, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF
    }
  }
  else if (String(PAYLOAD_LOCK).equals(state) || String(PAYLOAD_UNLOCK).equals(state))
  {
    strlcpy(switchT->stateControl, state.c_str(), sizeof(switchT->stateControl));//TODO CHECK STATE FROM SENSOR
    strlcpy(switchT->mqttPayload, state.c_str(), sizeof(switchT->mqttPayload));//TODO CHECK STATE FROM SENSOR
    if (switchT->typeControl == TYPE_RELAY)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? LOW : HIGH); //TURN ON
    }
  }else if (String(PAYLOAD_RELEASED).equals(state) )
  {
    strlcpy(switchT->stateControl, state.c_str(), sizeof(switchT->stateControl));//TODO CHECK STATE FROM SENSOR
    strlcpy(switchT->mqttPayload,  PAYLOAD_LOCK,sizeof(switchT->mqttPayload));//TODO CHECK STATE FROM SENSOR
    if (switchT->typeControl == TYPE_RELAY)
    {
      configPIN(switchT->primaryGpioControl, OUTPUT);
      writeToPIN(switchT->primaryGpioControl, switchT->inverted ? HIGH : LOW); //TURN OFF
    }
  }
  else if (isValidNumber(state))
  {

    switchT->percentageRequest = max(0l, min(100l, state.toInt()));
    if (switchT->positionControlCover > switchT->percentageRequest)
    {
      switchT->statePoolIdx = CLOSE_IDX;
      stateSwitch(switchT, statesPool[switchT->statePoolIdx]);
    }
    else
    {
      switchT->statePoolIdx = OPEN_IDX;
      stateSwitch(switchT, statesPool[switchT->statePoolIdx]);
    }
  }
  publishOnMqtt(switchT->mqttStateTopic, switchT->mqttPayload, switchT->mqttRetain);

  sendToServerEvents("states", String("{\"id\":\"") + String(switchT->id) + String("\",\"state\":\"") + String(switchT->mqttPayload) + String("\"}"));
  switchT->lastTimeChange = millis();
  switchT->statePoolIdx = findPoolIdx(switchT->stateControl, switchT->statePoolIdx, switchT->statePoolStart, switchT->statePoolEnd);
  saveSwitchs();
}
void stateSwitchById(String id, String state)
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
    if (strcmp(id.c_str(), switchs[i].id) == 0)
    {
      stateSwitch(&switchs[i], state);
    }
  }
}
void stateSwitchByName(const char *name, String state, String value)
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
    if (strcmp(switchs[i].name, name) == 0)
    {
      if (strcmp(FAMILY_COVER, switchs[i].family) == 0)
      {
        if (switchs[i].timeBetweenStates > 0)
        {
          stateSwitch(&switchs[i], value);
        }
        else
        {
          stateSwitch(&switchs[i], state);
        }
      }
      else if (strcmp(FAMILY_LIGHT, switchs[i].family) == 0 || strcmp(FAMILY_SWITCH, switchs[i].family) == 0)
      {

        stateSwitch(&switchs[i], state);
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
  if (strcmp(sw->family, FAMILY_COVER) != 0)
  {
    return false;
  }
  if (strcmp(PAYLOAD_STOP, sw->stateControl) == 0)
  {
    return false;
  }
  if(sw->lastTimeChange + COVER_AUTO_STOP_PROTECTION < millis()){
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

  for (unsigned int i = 0; i < switchs.size(); i++)
  {

    SwitchT *sw = &switchs[i];

    bool primaryValue = readPIN(sw->primaryGpio);
    bool secondaryValue = readPIN(sw->secondaryGpio);
    unsigned long currentTime = millis();

    bool primaryGpioEvent = primaryValue != sw->lastPrimaryGpioState;
    bool secondaryGpioEvent = secondaryValue != sw->lastSecondaryGpioState;

    if ((primaryGpioEvent || secondaryGpioEvent) && currentTime - sw->lastTimeChange >= 5)
    {
      sw->lastPrimaryGpioState = primaryValue;
      sw->lastSecondaryGpioState = secondaryValue;
      sw->lastTimeChange = currentTime;
      int poolSize = sw->statePoolEnd - sw->statePoolStart + 1;

      switch (sw->mode)
      {
      case MODE_SWITCH:
        sw->statePoolIdx = ((sw->statePoolIdx - sw->statePoolStart + 1) % poolSize) + sw->statePoolStart;
        stateSwitch(sw, statesPool[sw->statePoolIdx]);
        break;
      case MODE_PUSH:
        if (!primaryValue)
        { //PUSHED
          sw->statePoolIdx = ((sw->statePoolIdx - sw->statePoolStart + 1) % poolSize) + sw->statePoolStart;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        break;
      case MODE_DUAL_SWITCH:
        if (primaryValue == true && secondaryValue == true)
        {
          sw->statePoolIdx = sw->statePoolIdx == OPEN_IDX ? STOP_2_IDX : STOP_1_IDX;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        else if (primaryGpioEvent && !primaryValue)
        {
          sw->statePoolIdx = OPEN_IDX;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        else if (secondaryGpioEvent && !secondaryValue)
        {
          sw->statePoolIdx = CLOSE_IDX;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        break;
      case MODE_DUAL_PUSH:
        if (!primaryValue)
        { //PUSHED
          sw->statePoolIdx = sw->statePoolIdx == OPEN_IDX ? STOP_1_IDX : OPEN_IDX;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        if (!secondaryValue)
        { //PUSHED
          sw->statePoolIdx = sw->statePoolIdx == CLOSE_IDX ? STOP_2_IDX : CLOSE_IDX;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        break;
      default:
        break;
      }
    }

    if (sw->primaryGpioControl != NO_GPIO && sw->timeBetweenStates != 0 && digitalRead(sw->primaryGpioControl))
    {
      unsigned long offset = ((sw->lastPercentage * sw->timeBetweenStates) / 100);
      unsigned long deltaTime = (millis() - sw->lastTimeChange);
      if (sw->secondaryGpioControl != NO_GPIO)
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
      logger(SWITCHES_TAG, "AUTO STATE MODE set change switch to -> " + String(statesPool[sw->statePoolIdx]));
      stateSwitch(sw, statesPool[sw->statePoolIdx]);
    }
    if (positionDone(sw))
    {

      sw->statePoolIdx = sw->statePoolIdx == OPEN_IDX ? STOP_2_IDX : STOP_1_IDX;
      sw->percentageRequest = -1;
      logger(SWITCHES_TAG, "AUTO STATE MODE set change switch to -> " + String(statesPool[sw->statePoolIdx]));
      stateSwitch(sw, statesPool[sw->statePoolIdx]);
    }
  }
}
