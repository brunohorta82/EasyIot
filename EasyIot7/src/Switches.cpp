#include "Switches.h"
#include "Discovery.h"
//#include "WebServer.h"
#include "constants.h"
#include "Config.h"
#include "Mqtt.h"
#include "WebRequests.h"
#include <Bounce2.h>
#include "CloudIO.h"


struct Switches &getAtualSwitchesConfig()
{
  static Switches switches;
  return switches;
}

void save(Switches &switches)
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
  switches.lastChange = 0ul;
 
}

size_t Switches::serializeToJson(Print &output)
{
  if (items.empty())
    return output.write("[]");

  const size_t CAPACITY = JSON_ARRAY_SIZE(items.size()) + items.size() * (JSON_OBJECT_SIZE(21) + sizeof(SwitchT));
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
    sdoc["mqttCommandTopic"] = sw.mqttCommandTopic;
    sdoc["mqttStateTopic"] = sw.mqttStateTopic;
    sdoc["primaryGpioControl"] = sw.primaryGpioControl;
    sdoc["secondaryGpioControl"] = sw.secondaryGpioControl;
    sdoc["stateControl"] = sw.currentState;
    sdoc["cloudIOSupport"] = sw.cloudIOSupport;
    sdoc["haSupport"] = sw.haSupport;
    sdoc["knxSupport"] = sw.knxSupport;
    sdoc["mqttSupport"] = sw.mqttSupport;
  }
  return serializeJson(doc, output);
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
  
  //CONTROL  INIT STATE VARIABLES
  file.write((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
  file.write((uint8_t *)&lastPercentage, sizeof(lastPercentage));
  
  file.write((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  file.write((uint8_t *)&haSupport, sizeof(haSupport));
  file.write((uint8_t *)&knxSupport, sizeof(knxSupport));
  file.write((uint8_t *)&mqttSupport, sizeof(mqttSupport));
  
}
void SwitchT::load(File &file)
{
  file.read((uint8_t *)&firmware, sizeof(firmware));
  if (firmware < VERSION)
  {
    if(firmware < 8){
      //TODO MIGRATION
    }
#ifdef DEBUG
    Log.notice("%s Migrate Firmware from %F to %F" CR, tags::switches, firmware, VERSION);
#endif
    firmware = VERSION;
  }
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

  //MQTT
  file.read((uint8_t *)mqttCommandTopic, sizeof(mqttCommandTopic));
  file.read((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  
  //CONTROL  INIT STATE VARIABLES
  file.read((uint8_t *)&statePoolIdx, sizeof(statePoolIdx));
  file.read((uint8_t *)&lastPercentage, sizeof(lastPercentage));
  
  file.read((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  file.read((uint8_t *)&haSupport, sizeof(haSupport));
  file.read((uint8_t *)&knxSupport, sizeof(knxSupport));
  
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

bool Switches::remove(const char *id)
{

  auto match = std::find_if(items.begin(), items.end(), [id](const SwitchT &item) {
    return strcmp(id, item.id) == 0;
  });

  if (match == items.end())
  {
    return false;
  }
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

void SwitchT::updateFromJson(JsonObject doc)
{              
  
  strlcpy(name, doc["name"], sizeof(name));
  String idStr;
  generateId(idStr, name, sizeof(id));
  
  strlcpy(id, idStr.c_str(), sizeof(id));
  
  strlcpy(family, doc["family"], sizeof(family));
  primaryGpio =  doc["primaryGpio"] | constantsConfig::noGPIO;
  secondaryGpio =  doc["secondaryGpio"] | constantsConfig::noGPIO;
  autoStateDelay = doc["autoStateDelay"] | 0ul;
  strlcpy(autoStateValue,  doc["autoStateValue"] | "", sizeof(autoStateValue));
  typeControl = static_cast<SwitchControlType>(doc["typeControl"] | static_cast<int>(SwitchControlType::MQTT));
  mode =  static_cast<SwitchMode>(doc["mode"] | static_cast<int>(SWITCH));
  knxSupport = doc["knxSupport"];
  haSupport = doc["haSupport"] | true;
  mqttSupport = doc["mqttSupport"] | false;
  cloudIOSupport = doc["cloudIOSupport"] | true;
  pullup = true;
  mqttRetain =  doc["mqttRetain"] | false;
  inverted = false;
  reloadMqttTopics();
  //TODO INIT POOL STATE
  statePoolIdx = findPoolIdx("",-1,family);
  primaryGpioControl = doc["primaryGpioControl"] | constantsConfig::noGPIO;
  secondaryGpioControl =  doc["secondaryGpioControl"] | constantsConfig::noGPIO;

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

  configPIN(primaryGpioControl, OUTPUT);
  configPIN(secondaryGpioControl, OUTPUT);
  lastPrimaryGpioState = true;
  lastSecondaryGpioState = true;
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

int findPoolIdx(const char *state, unsigned int currentIdx,  const char *family)
{ unsigned int start, end;

  if(strcmp(family, constanstsSwitch::familyCover) == 0){
    start = constanstsSwitch::coverStartIdx;
    end = constanstsSwitch::converEndIdx;
  }else if(strcmp(family, constanstsSwitch::familyLock) == 0){
    start = constanstsSwitch::lockStartIdx;
    end = constanstsSwitch::lockEndIdx;
  }else {
     start = constanstsSwitch::switchStartIdx;
    end = constanstsSwitch::switchEndIdx;
  }

  if(currentIdx < 0)return start; // INIT POOL STATE IDX

  if(strlen(state) <0){ // RETURN NEXT POOL STATE
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
    if (strcmp(sw.mqttCommandTopic, topic) == 0 || strcmp(sw.mqttCloudCommandTopic, topic) == 0){
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

  if (isValidNumber(state))
  {
    int percentageRequest = max(0, min(100, atoi(state)));
  }else
  {

  statePoolIdx = findPoolIdx(state, statePoolIdx,family);
  if(statePoolIdx < 0) return;
  lastChangeState = millis();
  if (typeControl == SwitchControlType::GPIO_OUTPUT)
    {
  configPIN(primaryGpioControl, OUTPUT);
  configPIN(secondaryGpioControl, OUTPUT);
    }
  if (statePoolIdx == constanstsSwitch::openIdx)
  {
    if (typeControl == SwitchControlType::GPIO_OUTPUT)
    {
      
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(primaryGpioControl, inverted ? HIGH : LOW); //TURN OFF . STOP
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(secondaryGpioControl, inverted ? LOW : HIGH); //TURN ON . OPEN REQUEST
      delay(constanstsSwitch::delayCoverProtection);
      writeToPIN(primaryGpioControl, inverted ? LOW : HIGH); //TURN ON . EXECUTE REQUEST
    }
  }
  else if (statePoolIdx == constanstsSwitch::firtStopIdx || statePoolIdx == constanstsSwitch::secondStopIdx)
  {
    if (typeControl == SwitchControlType::GPIO_OUTPUT)
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
    
  }
  else if (statePoolIdx == constanstsSwitch::closeIdx)
  {
    if (typeControl == SwitchControlType::GPIO_OUTPUT)
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
    
  }
  else if (statePoolIdx == constanstsSwitch::onIdx)
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
  const char* currentStateToSend = currentState.c_str();
  notifyStateToCloudIO(mqttCloudStateTopic,currentStateToSend);
  publishOnMqtt(mqttStateTopic, currentStateToSend, true);
  String payloadSe;
  payloadSe.reserve(10 + strlen(id) + 21);
  payloadSe.concat("{\"id\":\"");
  payloadSe.concat(id);
  payloadSe.concat("\",\"state\":\"");
  payloadSe.concat(currentState);
  payloadSe.concat("\"}");
  sendToServerEvents("states", payloadSe.c_str());
  
  getAtualSwitchesConfig().lastChange = millis();
  }
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
  return sw.autoStateDelay > 0 && strlen(sw.autoStateValue) > 0 && strcmp(sw.autoStateValue, sw.currentState.c_str()) != 0 && sw.lastChangeState + sw.autoStateDelay < millis();
}
void loop(Switches &switches)
{

  if (switches.lastChange > 0 && switches.lastChange + constantsConfig::storeConfigDelay < millis())
  {
    save(switches);
  }
  for (auto &sw : switches.items)
  {
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
        sw.changeState(STATES_POLL[findPoolIdx("",sw.statePoolIdx,sw.family)].c_str());
      }
      break;
    case PUSH:
      if (sw.primaryGpio != constantsConfig::noGPIO && !primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
      { //PUSHED
        sw.changeState(STATES_POLL[findPoolIdx("",sw.statePoolIdx,sw.family)].c_str());
      }

      break;
    case DUAL_SWITCH:
      if (strcmp(sw.family, constanstsSwitch::familyCover) == 0 && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
      {
        if (primaryGpioEvent && secondaryGpioEvent && (sw.lastPrimaryGpioState != primaryGpioEvent || sw.lastSecondaryGpioState != secondaryGpioEvent))
        {
          sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx].c_str());
        }
        else if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
        {
          sw.changeState(STATES_POLL[constanstsSwitch::openIdx].c_str());
        }
        else if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
        {
          sw.changeState(STATES_POLL[ constanstsSwitch::closeIdx].c_str());
        }
      }
      break;
    case DUAL_PUSH:
      if (strcmp(sw.family, constanstsSwitch::familyCover) == 0 && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
      {
        if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
        { //PUSHED
          sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::firtStopIdx : constanstsSwitch::openIdx].c_str());
        }
        if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
        { //PUSHED

          sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::closeIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::closeIdx].c_str());
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
      Log.notice("%s State Timeout set change switch to %s " CR, tags::switches, statesPool[sw.statePoolIdx].c_str());
#endif
      sw.changeState(STATES_POLL[findPoolIdx(sw.autoStateValue, sw.statePoolIdx, sw.family)].c_str());
    }
  }
}
