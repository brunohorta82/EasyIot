#include "Switches.h"
#include "Discovery.h"
#include "WebServer.h"
#include "constants.h"
#include "Config.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include <Bounce2.h>
#include "CloudIO.h"
#include <Shutters.h>
extern Config config;
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
  if (SwitchMode::GATE_SWITCH == mode)
  {
    strlcpy(family, constanstsSwitch::familyGate, sizeof(family));
  }
  else
  {
    strlcpy(family, doc["family"], sizeof(family));
  }

  primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  secondaryGpio = doc["secondaryGpio"] | constantsConfig::noGPIO;
  autoStateDelay = doc["autoStateDelay"] | 0ul;
  strlcpy(autoStateValue, doc["autoStateValue"] | "", sizeof(autoStateValue));
  typeControl = static_cast<SwitchControlType>(doc["typeControl"] | static_cast<int>(SwitchControlType::NONE));

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
  knxLevelTwo = doc["knxLevelTwo"] | 0;
  knxLevelThree = doc["knxLevelThree"] | 0;
  knxSupport = knxLevelOne > 0 && knxLevelTwo >= 0 && knxLevelThree >= 0;
  primaryStateGpio = doc["primaryStateGpio"] | constantsConfig::noGPIO;
  secondaryStateGpio = doc["secondaryStateGpio"] | constantsConfig::noGPIO;
  thirdGpioControl = doc["thirdGpioControl"] | constantsConfig::noGPIO;

  reloadMqttTopics();
  if (statePoolIdx < 0)
  {
    statePoolIdx = findPoolIdx("", statePoolIdx, family);
  }

  primaryGpioControl = doc["primaryGpioControl"] | constantsConfig::noGPIO;
  secondaryGpioControl = doc["secondaryGpioControl"] | constantsConfig::noGPIO;

  configPins();

  // UPDATE JSON TO RESPONSE
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
  getAtualSwitchesConfig().lastChange = 0ul;
}
void Switches::toJson(JsonVariant &root)
{
  for (const auto &sw : items)
  {
    sw.toJson(root);
  }
}
void SwitchT::toJson(JsonVariant &root) const
{

  JsonVariant sdoc = root.createNestedObject();
  sdoc["id"] = id;
  sdoc["name"] = name;
  sdoc["family"] = family;
  sdoc["primaryGpio"] = primaryGpio;
  sdoc["secondaryGpio"] = secondaryGpio;
  sdoc["downCourseTime"] = downCourseTime;
  sdoc["upCourseTime"] = upCourseTime;
  sdoc["calibrationRatio"] = calibrationRatio;
  sdoc["autoStateValue"] = autoStateValue;
  sdoc["autoStateDelay"] = autoStateDelay;
  sdoc["childLock"] = childLock;
  sdoc["typeControl"] = static_cast<int>(typeControl);
  sdoc["mode"] = static_cast<int>(mode);
  sdoc["primaryGpioControl"] = primaryGpioControl;
  sdoc["secondaryGpioControl"] = secondaryGpioControl;
  sdoc["stateControl"] = getCurrentState();
  sdoc["cloudIOSupport"] = cloudIOSupport;
  sdoc["haSupport"] = haSupport;
  sdoc["knxSupport"] = knxSupport;
  sdoc["mqttSupport"] = mqttSupport;
  sdoc["knxLevelOne"] = knxLevelOne;
  sdoc["knxLevelTwo"] = knxLevelTwo;
  sdoc["knxLevelThree"] = knxLevelThree;
  sdoc["primaryStateGpio"] = primaryStateGpio;
  sdoc["secondaryStateGpio"] = secondaryStateGpio;
  sdoc["thirdGpioControl"] = thirdGpioControl;
}
const String SwitchT::getCurrentState() const
{
  if (isCover)
  {
    return String(lastPercentage);
  }

  return STATES_POLL[statePoolIdx];
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

  // MQTT
  file.write((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.write((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.write((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  // CONTROL VARIABLES
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

  file.write((uint8_t *)&primaryStateGpio, sizeof(primaryStateGpio));
  file.write((uint8_t *)&secondaryStateGpio, sizeof(secondaryStateGpio));
  file.write((uint8_t *)&thirdGpioControl, sizeof(thirdGpioControl));
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
    publishOnMqtt(shutters->getSwitchT()->mqttStateTopic, shutters->getSwitchT()->getCurrentState().c_str(), false);
  }
  if (shutters->getSwitchT()->cloudIOSupport)
  {
    if (shutters->getSwitchT()->getCurrentState().equalsIgnoreCase(constanstsSwitch::payloadStop))
    {
      notifyStateToCloudIO(shutters->getSwitchT()->mqttCloudStateTopic, shutters->getSwitchT()->getCurrentState().c_str(), strlen(shutters->getSwitchT()->getCurrentState().c_str()));
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
#ifdef ESP32
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW);
      delay(1);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH);
#else
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH); // TURN ON . OPEN REQUEST
#endif
    }
    break;
  case ShuttersOperation::DOWN:
    if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {

#ifdef ESP32
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH);
      delay(1);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW);
#else
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? LOW : HIGH);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW); // TURN OFF . CLOSE REQUEST
#endif
    }

    break;
  case ShuttersOperation::HALT:
    if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {

#ifdef ESP32
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW);
      delay(1);
      writeToPIN(s->getSwitchT()->secondaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW);
#else
      writeToPIN(s->getSwitchT()->primaryGpioControl, s->getSwitchT()->inverted ? HIGH : LOW);   // TURN OFF . STOP
#endif
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
  configPIN(thirdGpioControl, OUTPUT);
  configPIN(secondaryStateGpio, INPUT_PULLUP);
  configPIN(primaryStateGpio, INPUT_PULLUP);
  if (primaryGpio != constantsConfig::noGPIO)
  {
    debouncerPrimary = new Bounce();
    debouncerPrimary->attach(primaryGpio, INPUT_PULLUP);
    debouncerPrimary->interval(5);
  }

  if (secondaryGpio != constantsConfig::noGPIO)
  {
    debouncerSecondary = new Bounce();
    debouncerSecondary->attach(secondaryGpio, INPUT_PULLUP);
    debouncerSecondary->interval(5);
  }
#ifdef DEBUG_ONOFRE
  Log.notice("%s lastPrimaryGpioState %d" CR, tags::switches, lastPrimaryGpioState);
  Log.notice("%s lastSecondaryGpioState %d" CR, tags::switches, lastSecondaryGpioState);
#endif
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
  file.read((uint8_t *)&childLock, sizeof(childLock));
  // MQTT
  file.read((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.read((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.read((uint8_t *)&mqttRetain, sizeof(mqttRetain));

  // CONTROL VARIABLES
  file.read((uint8_t *)&lastPercentage, sizeof(lastPercentage));

  file.read((uint8_t *)&lastPrimaryGpioState, sizeof(lastPrimaryGpioState));
  file.read((uint8_t *)&lastSecondaryGpioState, sizeof(lastSecondaryGpioState));
  file.read((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
  file.read((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  file.read((uint8_t *)&haSupport, sizeof(haSupport));
  file.read((uint8_t *)&knxSupport, sizeof(knxSupport));
  file.read((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.read((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.read((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));

  file.read((uint8_t *)&mqttSupport, sizeof(mqttSupport));
  file.read((uint8_t *)&upCourseTime, sizeof(upCourseTime));
  file.read((uint8_t *)&downCourseTime, sizeof(downCourseTime));
  file.read((uint8_t *)&calibrationRatio, sizeof(calibrationRatio));
  file.read((uint8_t *)shutterState, sizeof(shutterState));
  file.read((uint8_t *)&primaryStateGpio, sizeof(primaryStateGpio));
  file.read((uint8_t *)&secondaryStateGpio, sizeof(secondaryStateGpio));
  file.read((uint8_t *)&thirdGpioControl, sizeof(thirdGpioControl));
  knxSupport = knxLevelOne > 0 && knxLevelTwo >= 0 && knxLevelThree >= 0;
  firmware = VERSION;
  configPins();
  bool isLight = strncmp(family, constanstsSwitch::familyLight, sizeof(constanstsSwitch::familyLight)) == 0;
  if (isLight)
    changeState(getCurrentState().c_str(), "LOAD");
}

void Switches::save()
{
  if (!LittleFS.begin())
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s File storage can't start" CR, tags::switches);
#endif
    return;
  }
  File file = LittleFS.open(configFilenames::switches, "w+");
  if (!file)
    return;
  this->save(file);
  file.close();
#ifdef DEBUG_ONOFRE
  Log.error("%s Request CloudIO Sync" CR, tags::switches);
#endif
  config.requestCloudIOSync();
}

void switchesCallback(message_t const &msg, void *arg)
{
  auto s = static_cast<SwitchT *>(arg);
  switch (msg.ct)
  {
  case KNX_CT_ADC_ANSWER:
  case KNX_CT_ADC_READ:
  case KNX_CT_ANSWER:
  case KNX_CT_ESCAPE:
  case KNX_CT_INDIVIDUAL_ADDR_REQUEST:
  case KNX_CT_INDIVIDUAL_ADDR_RESPONSE:
  case KNX_CT_INDIVIDUAL_ADDR_WRITE:
  case KNX_CT_MASK_VERSION_READ:
  case KNX_CT_MASK_VERSION_RESPONSE:
  case KNX_CT_MEM_ANSWER:
  case KNX_CT_MEM_READ:
  case KNX_CT_MEM_WRITE:
  case KNX_CT_READ:
  case KNX_CT_RESTART:
    break;

  case KNX_CT_WRITE:
  {
    uint16_t stateIdx = msg.data[1] | (msg.data[0] << 8);
    s->changeState(STATES_POLL[stateIdx].c_str(), "KNX");
    break;
  }
  };
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
    if (item.knxSupport)
    {
      knx.callback_unassign(item.knxIdAssign);
      knx.callback_deregister(item.knxIdRegister);
      item.knxIdRegister = knx.callback_register(String(item.name), switchesCallback, &item);
      item.knxIdAssign = knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, item.knxLevelThree));
      knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, 0));
      knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, 0, 0));
    }
  }
  knx.reload();
}

Switches &Switches::remove(const char *id)
{
  auto match = std::find_if(items.begin(), items.end(), [id](const SwitchT &item)
                            { return strcmp(id, item.id) == 0; });

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
  for (auto &item : switches.items)
  {
    if (item.knxSupport)
    {
      item.knxIdRegister = knx.callback_register(String(item.name), switchesCallback, &item);
      item.knxIdAssign = knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, item.knxLevelThree));
      knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, item.knxLevelTwo, 0));
      knx.callback_assign(item.knxIdRegister, knx.GA_to_address(item.knxLevelOne, 0, 0));
    }
  }
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

  if (!LittleFS.exists(configFilenames::switches))
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s No Switches config found!." CR, tags::switches);
#endif
    return;
  }
  File file = LittleFS.open(configFilenames::switches, "r+");
  switches.load(file);
  file.close();

#ifdef DEBUG_ONOFRE
  Log.notice("%s Stored values was loaded." CR, tags::switches);
#endif
}

int findPoolIdx(const char *state, int currentIdx, const char *family)
{
  int start, end;

  if (strcmp(family, constanstsSwitch::familyCover) == 0 || strcmp(family, constanstsSwitch::familyGate) == 0)
  {
    start = constanstsSwitch::coverStartIdx;
    end = constanstsSwitch::converEndIdx;
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
const void SwitchT::notifyState(bool dirty, const char *origin)
{
  if (strcmp("INTERNAL", origin) == 0)
  {
    return;
  }
  const String currentStateToSend = getCurrentState();
#ifdef DEBUG_ONOFRE
  Log.notice("%s %s current state: %s" CR, tags::switches, name, currentStateToSend.c_str());
#endif
  if (mqttSupport)
  {
    publishOnMqtt(mqttStateTopic, currentStateToSend.c_str(), true);
  }
  if (cloudIOSupport)
  {
    notifyStateToCloudIO(mqttCloudStateTopic, currentStateToSend.c_str(), currentStateToSend.length());
  }
  sendToServerEvents(id, currentStateToSend.c_str());
  if (dirty)
    getAtualSwitchesConfig().lastChange = millis();
  if (strcmp("KNX", origin) != 0 && knxSupport)
  {
    knx.write_1byte_int(knx.GA_to_address(knxLevelOne, knxLevelTwo, knxLevelThree), statePoolIdx);
    delay(5);
    knx.write_1byte_int(knx.GA_to_address(knxLevelOne, knxLevelTwo, knxLevelThree), statePoolIdx);
  }
  bool knxGroup = knxLevelOne > 0 && knxLevelTwo >= 0 && knxLevelThree == 0;
  if (knxGroup)
  {
    for (auto &sw : getAtualSwitchesConfig().items)
    {

      if (strcmp(sw.id, id) != 0)
      {
        sw.changeState(currentStateToSend.c_str(), "INTERNAL");
      }
    }
  }
}
const char *SwitchT::rotateState()
{
  int idx = findPoolIdx("", statePoolIdx, family);
  if (idx < 0)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s Error on rotate" CR, tags::switches);
#endif
    return "ERROR";
  }

  const char *state = STATES_POLL[idx].c_str();
  changeState(state, "ROTATE");
  return state;
}
void knkGroupNotifyState(const SwitchT &sw, const char *state)
{
}
void timedOn(unsigned int gpio, bool inverted, unsigned onTime)
{
  Serial.println("PORTAO4");
  configPIN(gpio, OUTPUT);
  writeToPIN(gpio, inverted ? LOW : HIGH); // TURN ON
  unsigned long pressOn = millis();
  while (pressOn + onTime > millis())
  {
  }
  writeToPIN(gpio, inverted ? HIGH : LOW); // TURN OFF
}
const String SwitchT::changeState(const char *state, const char *origin)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Name:      %s" CR, tags::switches, name);
  Log.notice("%s State:     %s" CR, tags::switches, state);
  Log.notice("%s State IDX: %d" CR, tags::switches, statePoolIdx);
  Log.notice("%s From : %s" CR, tags::switches, origin);
  Log.notice("%s Family : %s" CR, tags::switches, family);
#endif
  bool dirty = strcmp(state, getCurrentState().c_str()) != 0;
  bool isCover = strcmp(family, constanstsSwitch::familyCover) == 0;
  bool isFromKnx = strcmp("KNX", origin) == 0;
  bool isGate = strncmp(family, constanstsSwitch::familyGate, sizeof(constanstsSwitch::familyGate)) == 0;
  if (isCover)
  {
    if (typeControl == SwitchControlType::GPIO_OUTPUT)
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
      if (strcmp(constanstsSwitch::payloadOpen, state) == 0)
      {
        statePoolIdx = constanstsSwitch::openIdx;
      }
      else if (strcmp(constanstsSwitch::payloadStop, state) == 0)
      {
        statePoolIdx = statePoolIdx == 3 ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx;
      }
      else if (strcmp(constanstsSwitch::payloadClose, state) == 0)
      {
        statePoolIdx = constanstsSwitch::closeIdx;
      }
      notifyState(dirty, origin);
    }
  }
  else if (isGate)
  {
    statePoolIdx = findPoolIdx(state, statePoolIdx, family);
    if (statePoolIdx < 0)
    {
      return "ERROR";
    }
    if (typeControl == SwitchControlType::GPIO_OUTPUT)
    {
      if (primaryGpioControl != constantsConfig::noGPIO && secondaryGpioControl == constantsConfig::noGPIO && thirdGpioControl == constantsConfig::noGPIO)
      {
        if (statePoolIdx == constanstsSwitch::closeIdx || statePoolIdx == constanstsSwitch::openIdx || statePoolIdx == constanstsSwitch::firtStopIdx || statePoolIdx == constanstsSwitch::secondStopIdx)
        {
          timedOn(primaryGpioControl, inverted, 1000);
        }
      }
      else if (primaryGpioControl != constantsConfig::noGPIO && secondaryGpioControl != constantsConfig::noGPIO && thirdGpioControl == constantsConfig::noGPIO)
      {
        if (statePoolIdx == constanstsSwitch::closeIdx)
        {
          timedOn(primaryGpioControl, inverted, 1000);
        }
        else if (statePoolIdx == constanstsSwitch::openIdx)
        {
          timedOn(secondaryGpioControl, inverted, 1000);
        }
      }
      else if (primaryGpioControl != constantsConfig::noGPIO && secondaryGpioControl != constantsConfig::noGPIO && thirdGpioControl != constantsConfig::noGPIO)
      {
        if (statePoolIdx == constanstsSwitch::closeIdx)
        {
          timedOn(primaryGpio, inverted, 1000);
        }
        else if (statePoolIdx == constanstsSwitch::openIdx)
        {
          timedOn(secondaryGpioControl, inverted, 1000);
        }
        else
        {
          timedOn(thirdGpioControl, inverted, 1000);
        }
      }
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
        writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); // TURN ON
      }
    }
    else if (statePoolIdx == constanstsSwitch::offIdx)
    {
      if (typeControl == SwitchControlType::GPIO_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); // TURN OFF
      }
    }
    else if (statePoolIdx == constanstsSwitch::closeIdx || statePoolIdx == constanstsSwitch::openIdx)
    {
      if (typeControl == SwitchControlType::GPIO_OUTPUT)
      {
        configPIN(primaryGpioControl, OUTPUT);
        writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); // TURN ON
      }
    }

    notifyState(dirty, origin);
  }

  return getCurrentState();
}

const String Switches::stateSwitchById(const char *id, const char *state)
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
  return sw.autoStateDelay > 0 && strlen(sw.autoStateValue) > 0 && strcmp(sw.autoStateValue, sw.getCurrentState().c_str()) != 0 && sw.lastChangeState + sw.autoStateDelay < millis();
}
void loop(Switches &switches)
{
  if (switches.lastChange > 0 && switches.lastChange + constantsConfig::storeConfigDelay < millis())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s AUTO SAVE" CR, tags::switches);
#endif
    getAtualSwitchesConfig().save();
  }
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
#ifdef DEBUG_ONOFRE
        Log.notice("%s 1 lastPrimaryGpioState %d primaryGpioEvent %d" CR, tags::switches, sw.lastPrimaryGpioState, primaryGpioEvent);
#endif
        sw.rotateState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s 2 lastPrimaryGpioState %d primaryGpioEvent %d" CR, tags::switches, sw.lastPrimaryGpioState, primaryGpioEvent);
#endif
      }
      break;
    case PUSH:
      if (sw.primaryGpio != constantsConfig::noGPIO && !primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
      {
        sw.rotateState();
      }

      break;
    case GATE_SWITCH:
    {
      bool primaryStateGpioEvent = true;
      bool secondaryStateGpioEvent = true;
      if (sw.primaryStateGpio != constantsConfig::noGPIO)
      {
        primaryStateGpioEvent = readPIN(sw.primaryStateGpio);
        if (sw.lastPrimaryStateGpioState != primaryStateGpioEvent)
        {
          sw.lastPrimaryStateGpioState = primaryStateGpioEvent;
          if (primaryStateGpioEvent)
          {
            sw.statePoolIdx = constanstsSwitch::closeIdx;
          }
          else
          {
            sw.statePoolIdx = constanstsSwitch::openIdx;
          }
          sw.notifyState(true, "LOOP");
        }
      }
      if (sw.secondaryStateGpio != constantsConfig::noGPIO)
      {
        secondaryStateGpioEvent = readPIN(sw.secondaryStateGpio);
        if (sw.lastSecondaryStateGpioState != secondaryStateGpioEvent)
        {
          sw.lastSecondaryStateGpioState = secondaryStateGpioEvent;
          if (secondaryStateGpioEvent)
          {
            sw.statePoolIdx = constanstsSwitch::openIdx;
          }
          else
          {
            sw.statePoolIdx = constanstsSwitch::closeIdx;
          }
          sw.notifyState(true, "LOOP");
        }
      }
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
        { // PUSHED
          sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::firtStopIdx : constanstsSwitch::openIdx].c_str(), "DUAL_PUSH");
        }
        if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
        { // PUSHED
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

#ifdef DEBUG_ONOFRE
      Log.notice("%s State Timeout set change switch to %s " CR, tags::switches, STATES_POLL[sw.statePoolIdx].c_str());
#endif
      sw.changeState(STATES_POLL[findPoolIdx(sw.autoStateValue, sw.statePoolIdx, sw.family)].c_str(), "AUTO_TIMEOUT");
    }
  }
}
