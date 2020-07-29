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
#include "Templates.h"

struct Switches &getAtualSwitchesConfig()
{
  static Switches switches;
  return switches;
}
void SwitchT::updateFromJson(JsonObject doc)
{
  firmware = VERSION;
  strlcpy(name, doc["name"], sizeof(name));
  String idStr;
  mode = static_cast<SwitchMode>(doc["mode"] | static_cast<int>(SWITCH));
  generateId(idStr, name, static_cast<int>(mode), sizeof(id));
  strlcpy(id, idStr.c_str(), sizeof(id));
  strlcpy(family, doc["family"], sizeof(family));
  primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  secondaryGpio = doc["secondaryGpio"] | constantsConfig::noGPIO;
  autoStateDelay = doc["autoStateDelay"] | 0ul;
  strlcpy(autoStateValue, doc["autoStateValue"] | "", sizeof(autoStateValue));
  typeControl = static_cast<SwitchControlType>(doc["typeControl"] | static_cast<int>(SwitchControlType::NONE));
  knxSupport = doc["knxSupport"] | false;
  haSupport = doc["haSupport"] | false;
  mqttSupport = doc["mqttSupport"] | false;
  cloudIOSupport = doc["cloudIOSupport"] | true;
  childLock = doc["childLock"] | false;
  upCourseTime = doc["upCourseTime"] | upCourseTime;
  downCourseTime = doc["downCourseTime"] | downCourseTime;
  calibrationRatio = doc["calibrationRatio"] | calibrationRatio;
  pullup = doc["pullup"] | pullup;
  mqttRetain = doc["mqttRetain"] | mqttRetain;
  inverted = doc["inverted"] | inverted;
    
   knxLevelOne = doc["knxLevelOne"] | 0;
   knxLevelTwo =doc["knxLevelTwo"]  |0;
   knxLevelThree = doc["knxLevelThree"]|0;

  reloadMqttTopics();
  if (statePoolIdx < 0)
  {
    statePoolIdx = findPoolIdx("", statePoolIdx, family);
  }

  primaryGpioControl = doc["primaryGpioControl"] | constantsConfig::noGPIO;
  secondaryGpioControl = doc["secondaryGpioControl"] | constantsConfig::noGPIO;

  configPins();

  //UPDATE JSON TO RESPONSE
  doc["id"] = id;
  doc["stateControl"] = STATES_POLL[statePoolIdx];
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
void Switches::save(File &file) const
{
  auto n_items = items.size();
  file.write((uint8_t *)&n_items, sizeof(n_items));
  for (const auto &item : items)
    item.save(file);
}
void Switches::toJson(JsonVariant &root)
{
  DynamicJsonDocument doc(1024);
  JsonArray a = root.to<JsonArray>();
  for (const auto &sw : items)
  {
    JsonVariant sdoc = doc.to<JsonVariant>();
    sdoc["id"] = sw.id;
    sdoc["name"] = sw.name;
    sdoc["family"] = sw.family;
    sdoc["primaryGpio"] = sw.primaryGpio;
    sdoc["secondaryGpio"] = sw.secondaryGpio;
    sdoc["downCourseTime"] = sw.downCourseTime;
    sdoc["upCourseTime"] = sw.upCourseTime;
    sdoc["calibrationRatio"] = sw.calibrationRatio;
    sdoc["autoStateValue"] = sw.autoStateValue;
    sdoc["autoStateDelay"] = sw.autoStateDelay;
    sdoc["childLock"] = sw.childLock;
    sdoc["typeControl"] = static_cast<int>(sw.typeControl);
    sdoc["mode"] = static_cast<int>(sw.mode);
    sdoc["primaryGpioControl"] = sw.primaryGpioControl;
    sdoc["secondaryGpioControl"] = sw.secondaryGpioControl;
    sdoc["stateControl"] = sw.getCurrentState();
    sdoc["lastPercentage"] = sw.lastPercentage;
    sdoc["cloudIOSupport"] = sw.cloudIOSupport;
    sdoc["haSupport"] = sw.haSupport;
    sdoc["knxSupport"] = sw.knxSupport;
    sdoc["mqttSupport"] = sw.mqttSupport;
    sdoc["knxLevelOne"] = sw.knxLevelOne;
    sdoc["knxLevelTwo"] = sw.knxLevelTwo;
    sdoc["knxLevelThree"] = sw.knxLevelThree;
    a.add(sdoc);
  }
}
const char *SwitchT::getCurrentState() const
{
  return STATES_POLL[statePoolIdx].c_str();
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
  file.write((uint8_t *)&childLock, sizeof(childLock));

  //MQTT
  file.write((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.write((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.write((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  //CONTROL VARIABLES
  file.write((uint8_t *)&lastPercentage, sizeof(lastPercentage));

  file.write((uint8_t *)&lastPrimaryGpioState, sizeof(lastPrimaryGpioState));
  file.write((uint8_t *)&lastSecondaryGpioState, sizeof(lastSecondaryGpioState));
  file.write((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));

  file.write((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  file.write((uint8_t *)&haSupport, sizeof(haSupport));
  file.write((uint8_t *)&knxSupport, sizeof(knxSupport));
  file.write((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.write((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.write((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));
  file.write((uint8_t *)&mqttSupport, sizeof(mqttSupport));
  file.write((uint8_t *)&upCourseTime, sizeof(upCourseTime));
  file.write((uint8_t *)&downCourseTime, sizeof(downCourseTime));
  file.write((uint8_t *)&calibrationRatio, sizeof(calibrationRatio));
  file.write((uint8_t *)shutterState, sizeof(shutterState));
}

void shuttersWriteStateHandler(Shutters *shutters, const char *state, byte length)
{
  for (byte i = 0; i < length; i++)
  {
    shutters->getSwitchT()->shutterState[i] = state[i];
  }

  getAtualSwitchesConfig().save();
  if (shutters->getSwitchT()->mqttSupport)
  {
    publishOnMqtt(shutters->getSwitchT()->mqttStateTopic, shutters->getSwitchT()->getCurrentState(), false);
  }
  if (shutters->getSwitchT()->cloudIOSupport)
  {
    if (strcmp(shutters->getSwitchT()->getCurrentState(), constanstsSwitch::payloadStop) == 0)
    {
      notifyStateToCloudIO(shutters->getSwitchT()->mqttCloudStateTopic, shutters->getSwitchT()->getCurrentState(), strlen(shutters->getSwitchT()->getCurrentState()));
    }
    else
    {
      char dump[4] = {0};
      int l = sprintf(dump, "%d", shutters->getSwitchT()->lastPercentage);
      notifyStateToCloudIO(shutters->getSwitchT()->mqttCloudStateTopic, dump, l);
    }
  }
}
void shuttersOperationHandler(Shutters *s, ShuttersOperation operation)
{
  switch (operation)
  {
  case ShuttersOperation::UP:
    if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH); //TURN ON . OPEN REQUEST
    }
    break;
  case ShuttersOperation::DOWN:
    if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW); //TURN OFF . CLOSE REQUEST
    }

    break;
  case ShuttersOperation::HALT:
    if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW); //TURN OFF . STOP
    }
    break;
  }
}
void readLastShutterState(char *dest, byte length, char *value)
{
  for (byte i = 0; i < length; i++)
  {
    dest[i] = value[i];
  }
}

void onShuttersLevelReached(Shutters *shutters, uint8_t level)
{
  shutters->getSwitchT()->lastPercentage = level;

  char dump[4] = {0};
  int l = sprintf(dump, "%d", level);
  if (shutters->getSwitchT()->mqttSupport)
  {
    publishOnMqtt(shutters->getSwitchT()->mqttStateTopic, dump, false);
  }
  if (shutters->getSwitchT()->cloudIOSupport)
  {
    notifyStateToCloudIO(shutters->getSwitchT()->mqttCloudStateTopic, dump, l);
  }
}
void SwitchT::configPins()
{
  isCover = strcmp(family, constanstsSwitch::familyCover) == 0;
  if (isCover)
  {
    shutter = new Shutters(this);
    char storedShuttersState[shutter->getStateLength()];
    readLastShutterState(storedShuttersState, shutter->getStateLength(), shutter->getSwitchT()->shutterState);
    shutter->setOperationHandler(shuttersOperationHandler)
        .setWriteStateHandler(shuttersWriteStateHandler)
        .restoreState(storedShuttersState)
        .setCourseTime(upCourseTime, downCourseTime)
        .onLevelReached(onShuttersLevelReached)
        .begin();
  }
  configPIN(primaryGpioControl, OUTPUT);
  configPIN(secondaryGpioControl, OUTPUT);
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
  lastPrimaryGpioState = pullup;
  lastSecondaryGpioState = pullup;

}
void SwitchT::load(File &file)
{
  file.read((uint8_t *)&firmware, sizeof(firmware));
  double configFirmware = (double)firmware;
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
  if (configFirmware < 7.94){
    file.seek(sizeof(unsigned long), SeekCur); // remove timeBetweenStates
  }
  file.read((uint8_t *)&childLock, sizeof(childLock));

  //MQTT
  file.read((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.read((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  if (configFirmware < 7.94)
  {
    file.seek(sizeof(char[128]), SeekCur); // remove mqttPositionStateTopic
    file.seek(sizeof(char[128]), SeekCur); // remove mqttPositionCommandTopic
    file.seek(sizeof(char[10]), SeekCur);  // remove mqttPayload
  }
  file.read((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  //CONTROL VARIABLES
   if (configFirmware < 7.94)
  {
    file.seek(sizeof(char[10]), SeekCur); // remove stateControl
    file.seek(sizeof(int), SeekCur);      // remove positionControlCover
  }
  file.read((uint8_t *)&lastPercentage, sizeof(lastPercentage));

  file.read((uint8_t *)&lastPrimaryGpioState, sizeof(lastPrimaryGpioState));
  file.read((uint8_t *)&lastSecondaryGpioState, sizeof(lastSecondaryGpioState));
  file.read((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
     if (configFirmware < 7.94)
  {
    file.seek(sizeof(unsigned int), SeekCur); // remove statePoolStart
    file.seek(sizeof(unsigned int), SeekCur); // remove statePoolEnd
  }

  file.read((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  file.read((uint8_t *)&haSupport, sizeof(haSupport));
  file.read((uint8_t *)&knxSupport, sizeof(knxSupport));
  file.read((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.read((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.read((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));
  if (configFirmware >= 7.94)
  {
    file.read((uint8_t *)&mqttSupport, sizeof(mqttSupport));
    file.read((uint8_t *)&upCourseTime, sizeof(upCourseTime));
    file.read((uint8_t *)&downCourseTime, sizeof(downCourseTime));
    file.read((uint8_t *)&calibrationRatio, sizeof(calibrationRatio));
    file.read((uint8_t *)shutterState, sizeof(shutterState));
  }

  firmware = VERSION;
  configPins();
  changeState(getCurrentState(), "LOAD");
}


void Switches::save()
{
  if (!SPIFFS.begin())
  {
#ifdef DEBUG
    Log.error("%s File storage can't start" CR, tags::switches);
#endif
    return;
  }
  File file = SPIFFS.open(configFilenames::switches, "w+");
  if (!file)
    return;
  this->save(file);
  file.close();

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

    s->changeState(STATES_POLL[stateIdx].c_str(),"KNX");

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
    int stateIdx = (int)msg.data[0];
    for (auto &sw : getAtualSwitchesConfig().items)
    {
      sw.knxNotifyGroup = false;
      sw.changeState(STATES_POLL[stateIdx].c_str(),"KNX_GROUP");
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
  for (auto &item : items){
      item.load(file);
        //KNX
      bool globalKnxLevelThreeAssign = false;
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

Switches &Switches::remove(const char *id)
{
  auto match = std::find_if(items.begin(), items.end(), [id](const SwitchT &item) {
    return strcmp(id, item.id) == 0;
  });

  if (match == items.end())
  {
    return *this;
  }
  unsubscribeOnMqtt(match->mqttCommandTopic);
  removeFromHaDiscovery(*match);
  items.erase(match);
  save();
  return *this;
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
    sw.reloadMqttTopics();
  getAtualSwitchesConfig().save();
}
void saveAndRefreshServices(Switches &switches, const SwitchT &sw)
{
  getAtualSwitchesConfig().save();
  removeFromHaDiscovery(sw);
  if (sw.haSupport)
    addToHaDiscovery(sw);
}
Switches &Switches::updateFromJson(const String &id, JsonObject &doc)
{
  for (auto &sw : items)
  {
    if (strcmp(id.c_str(), sw.id) == 0)
    {
      sw.updateFromJson(doc);
      saveAndRefreshServices(*this, sw);
      return *this;
    }
  }
  SwitchT newSw;
  newSw.updateFromJson(doc);
  items.push_back(newSw);
  saveAndRefreshServices(*this, newSw);
  return *this;
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

  if (!SPIFFS.exists(configFilenames::switches))
  {
#ifdef DEBUG
    Log.notice("%s Default config loaded." CR, tags::switches);
#endif
    loadSwitchDefaults();
    getAtualSwitchesConfig().save();
  }
  File file = SPIFFS.open(configFilenames::switches, "r+");
  switches.load(file);
  file.close();

#ifdef DEBUG
  Log.notice("%s Stored values was loaded." CR, tags::switches);
#endif
}

int findPoolIdx(const char *state, int currentIdx, const char *family)
{
  int start, end;

  if (strcmp(family, constanstsSwitch::familyCover) == 0)
  {
    start = constanstsSwitch::coverStartIdx;
    end = constanstsSwitch::converEndIdx;
  }
  else if (strcmp(family, constanstsSwitch::familyLock) == 0)
  {
    start = constanstsSwitch::lockStartIdx;
    end = constanstsSwitch::lockEndIdx;
  }
  else
  {
    start = constanstsSwitch::switchStartIdx;
    end = constanstsSwitch::switchEndIdx;
  }

  if (currentIdx < 0)
    return start; // INIT POOL STATE IDX

  if (strlen(state) < 0 || strcmp(state, "") == 0)
  { // RETURN NEXT POOL STATE
    int poolSize = end - start + 1;
    return ((currentIdx - start + 1) % poolSize) + start;
  }

  int max = (end - start) * 2;
  unsigned int p = currentIdx;
  while (max > 0)
  {
    if (strcmp(state, STATES_POLL[p].c_str()) == 0)
    {
      return p;
    }
    p = ((p - start + 1) % (end - start + 1)) + start;
    max--;
  }
  return -1;
}


void mqttSwitchControl(Switches &switches, const char *topic, const char *payload)
{
  for (auto &sw : switches.items)
  {
    if (strcmp(sw.mqttCommandTopic, topic) == 0 || strcmp(sw.mqttCloudCommandTopic, topic) == 0)
    {
      if (strcmp(payload, "DISABLED") == 0)
      {
        sw.childLock = true;
        continue;
      }
      else if (strcmp(payload, "ENABLED") == 0)
      {
        sw.childLock = false;
        continue;
      }
      sw.changeState(payload, "MQTT");
    }
  }
}
const char *Switches::rotate(const char *id)
{
  for (auto &sw : items)
  {
    if (strcmp(id, sw.id) == 0)
    {
      return sw.rotateState();
    }
  }
  return "ERROR";
}
const char *SwitchT::rotateState()
{
  int idx = findPoolIdx("", statePoolIdx, family);
  if (idx < 0)
    return "ERROR";
  const char *state = STATES_POLL[idx].c_str();
  changeState(state, "ROTATE");
  return state;
}
void knkGroupNotifyState(const SwitchT &sw, const char *state)
{
}
const char *SwitchT::changeState(const char *state, const char *origin)
{
#ifdef DEBUG
  Log.notice("%s Name:      %s" CR, tags::switches, name);
  Log.notice("%s State:     %s" CR, tags::switches, state);
  Log.notice("%s State IDX: %d" CR, tags::switches, statePoolIdx);
  Log.notice("%s From : %s" CR, tags::switches, origin);
#endif

  bool isCover = strcmp(family, constanstsSwitch::familyCover) == 0;
  if (isCover)
  {
    if (strcmp(constanstsSwitch::payloadOpen, state) == 0)
    {
      shutter->setLevel(0);
      statePoolIdx = constanstsSwitch::openIdx;
    }
    else if (strcmp(constanstsSwitch::payloadStop, state) == 0)
    {
      shutter->stop();
      statePoolIdx = statePoolIdx == 3 ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx;
    }
    else if (strcmp(constanstsSwitch::payloadClose, state) == 0)
    {
      shutter->setLevel(100);
      statePoolIdx = constanstsSwitch::closeIdx;
    }
    else if (isValidNumber(state))
    {
      shutter->setLevel(max(0, min(100, atoi(state))));
    }
  }
  else
  {

    statePoolIdx = findPoolIdx(state, statePoolIdx, family);
    if (statePoolIdx < 0)
      return "ERROR";
    lastChangeState = millis();

    if (statePoolIdx == constanstsSwitch::onIdx)
    {
      if (typeControl == SwitchControlType::GPIO_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON
      }
    }
    else if (statePoolIdx == constanstsSwitch::offIdx)
    {
      if (typeControl == SwitchControlType::GPIO_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF
      }
    }
    else if (statePoolIdx == constanstsSwitch::lockIdx || statePoolIdx == constanstsSwitch::unlockIdx)
    {
      if (typeControl == SwitchControlType::GPIO_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON
      }
    }
    else if (strcmp(constanstsSwitch::payloadReleased, state) == 0)
    {
      if (typeControl == SwitchControlType::GPIO_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF
      }
    }
    const char *currentStateToSend = getCurrentState();
    if (mqttSupport)
    {
      publishOnMqtt(mqttStateTopic, currentStateToSend, true);
    }
    if (cloudIOSupport)
    {
      notifyStateToCloudIO(mqttCloudStateTopic, currentStateToSend, strlen(currentStateToSend));
    }
    sendToServerEvents(id, currentStateToSend);
    if(strcmp("LOAD",origin) != 0)
      getAtualSwitchesConfig().save();
  }
    if (knxNotifyGroup && knxSupport)
  {
    knx.write_1byte_int(knx.GA_to_address(knxLevelOne, knxLevelTwo, knxLevelThree), statePoolIdx);
  }
    knxNotifyGroup = true;
  return getCurrentState();
}

const char *Switches::stateSwitchById(const char *id, const char *state)
{
  for (auto &sw : getAtualSwitchesConfig().items)
  {
    if (strcmp(id, sw.id) == 0)
    {
      return sw.changeState(state, "BY_ID");
    }
  }
  return "ERROR";
}
bool stateTimeout(SwitchT &sw)
{
  return sw.autoStateDelay > 0 && strlen(sw.autoStateValue) > 0 && strcmp(sw.autoStateValue, sw.getCurrentState()) != 0 && sw.lastChangeState + sw.autoStateDelay < millis();
}
void loop(Switches &switches)
{

  for (auto &sw : switches.items)
  {
    if (sw.isCover)
      sw.shutter->loop();
    if (sw.childLock)
      continue;
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

    switch (sw.mode)
    {
    case SWITCH:
      if (sw.primaryGpio != constantsConfig::noGPIO && sw.lastPrimaryGpioState != primaryGpioEvent)
      {
        sw.rotateState();
      }
      break;
    case PUSH:
      if (sw.primaryGpio != constantsConfig::noGPIO && !primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
      {
        sw.rotateState();
      }

      break;
    case DUAL_SWITCH:
      if (strcmp(sw.family, constanstsSwitch::familyCover) == 0 && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
      {
        if (primaryGpioEvent && secondaryGpioEvent && (sw.lastPrimaryGpioState != primaryGpioEvent || sw.lastSecondaryGpioState != secondaryGpioEvent))
        {
          sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx].c_str(), "DUAL_SWITCH_STOP");
        }
        else if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
        {
          sw.changeState(STATES_POLL[constanstsSwitch::openIdx].c_str(), "DUAL_SWITCH_OPEN");
        }
        else if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
        {
          sw.changeState(STATES_POLL[constanstsSwitch::closeIdx].c_str(), "DUAL_SWITCH_CLOSE");
        }
      }
      break;
    case DUAL_PUSH:
      if (strcmp(sw.family, constanstsSwitch::familyCover) == 0 && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
      {
        if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
        { //PUSHED
          sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::firtStopIdx : constanstsSwitch::openIdx].c_str(), "DUAL_PUSH");
        }
        if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
        { //PUSHED
          sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::closeIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::closeIdx].c_str(), "DUAL_PUSH");
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

#ifdef DEBUG
      Log.notice("%s State Timeout set change switch to %s " CR, tags::switches, STATES_POLL[sw.statePoolIdx].c_str());
#endif
      sw.changeState(STATES_POLL[findPoolIdx(sw.autoStateValue, sw.statePoolIdx, sw.family)].c_str(), "AUTO_TIMEOUT");
    }
  }
}
