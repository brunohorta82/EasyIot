#include "Templates.h"
#define RELAY_TYPE "relay"
#define MQTT_TYPE "mqtt"
#define SWITCH_DEVICE "switch"
#define BUTTON_SWITCH 1
#define BUTTON_PUSH 2
#define OPEN_CLOSE_SWITCH 4
#define OPEN_CLOSE_PUSH 5
#define AUTO_OFF 6
const String switchsFilename = "switchs.json";

String statesPool[] = {"OPEN", "STOP", "CLOSE", "STOP"};

JsonArray &sws = getJsonArray();

bool coverNeedsStop;
unsigned long openCloseonTime;
unsigned int _gpioStop;
unsigned int swsSize;
typedef struct
{
  Bounce *debouncer;
  bool lastState;
} switch_t;
std::vector<switch_t> _switchs;

void callback(uint8_t gpio, uint8_t event)
{
  
  for (unsigned int i = 0; i < sws.size(); i++)
  {
    JsonObject &switchJson = sws.get<JsonVariant>(i);
    if (switchJson.get<unsigned int>("gpio") == gpio){
      Serial.println(gpio);
         //stateSwitch(switchJson, switchJson.get<bool>("stateControl") ? "OFF" : "ON");
  }
  }
}

JsonObject &storeSwitch(JsonObject &_switch)
{
  removeSwitch(_switch.get<String>("id"));
 _switch.set("id",_switch.get<String>("id").equals(NEW_ID) ?  (String(ESP.getChipId()) +normalize(_switch.get<String>("name"))) : _switch.get<String>("id"));
 _switch.set("class", "switch");

  rebuildSwitchMqttTopics(_switch);
  rebuildDiscoverySwitchMqttTopics(_switch);
    String sw = "";
  _switch.printTo(sw);
  sws.add(getJsonObject(sw.c_str()));
  persistSwitchesFile();
  return _switch;
}

void coverAutoStop(int gpioStop)
{
  logger("[COVERS] AUTO STOP");
  coverNeedsStop = true;
  openCloseonTime = millis();
  _gpioStop = gpioStop;
}

void stateSwitch(JsonObject &switchJson, String state)
{
Serial.println(state);
  if (switchJson.get<String>("typeControl").equals(RELAY_TYPE))
  {
    if (String(PAYLOAD_OPEN).equals(state))
    {
      openAction(switchJson);
    }
    else if (String(PAYLOAD_STOP).equals(state))
    {
      stopAction(switchJson);
    }
    else if (String(PAYLOAD_CLOSE).equals(state))
    {
      closeAction(switchJson);
    }
    else if (String(PAYLOAD_ON).equals(state))
    {
      turnOn(switchJson);
    }
    else if (String(PAYLOAD_OFF).equals(state))
    {
      turnOff(switchJson);
    }
    else if (String(PAYLOAD_LOCK).equals(state))
    {
      //TODO
    }
    else if (String(PAYLOAD_UNLOCK).equals(state))
    {
      //TODO
    }
  }else {
   if (String(PAYLOAD_OPEN).equals(state))
    {
      switchJson.set("positionControlCover", 100);
      switchJson.set("stateControlCover", "OPEN");
      switchJson.set("statePayload", "open");
      publishState(switchJson);
    }
    else if (String(PAYLOAD_STOP).equals(state))
    {
       switchJson.set("positionControlCover", 50);
       switchJson.set("stateControlCover", "STOP");
       switchJson.set("statePayload", "");
       publishState(switchJson);
    }
    else if (String(PAYLOAD_CLOSE).equals(state))
    {
      switchJson.set("positionControlCover", 0);
      switchJson.set("stateControlCover", "CLOSE");
      switchJson.set("statePayload", "closed");
      publishState(switchJson);
    }
    else if (String(PAYLOAD_ON).equals(state))
    {
      switchJson.set("stateControl", state.equals("ON") ? true : false);
      switchJson.set("statePayload", state);
      publishState(switchJson);
    }
    else if (String(PAYLOAD_OFF).equals(state))
    {
       switchJson.set("stateControl", state.equals("OFF") ? false : true);
       switchJson.set("statePayload", state);
       publishState(switchJson);
    }
    else if (String(PAYLOAD_LOCK).equals(state))
    {
      switchJson.set("stateControl", state.equals("LOCK"));
      switchJson.set("statePayload", state);
      publishState(switchJson);
    }
    else if (String(PAYLOAD_UNLOCK).equals(state))
    {
       switchJson.set("stateControl", !state.equals("UNLOCK"));
      switchJson.set("statePayload", state);
      publishState(switchJson);
    }
  }
}

void applyJsonSwitchs()
{
  _switchs.clear();
  for (int i = 0; i < sws.size(); i++)
  {
    JsonObject &switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<unsigned int>("gpio") == 99){   
      continue;
    }
    
    uint8_t mode = 0;
    if (switchJson.get<unsigned int>("mode") == OPEN_CLOSE_SWITCH || switchJson.get<unsigned int>("mode") == BUTTON_SWITCH)
    {
      mode = mode | BUTTON_SWITCH;
    }
    else if (switchJson.get<unsigned int>("mode") == OPEN_CLOSE_PUSH || switchJson.get<unsigned int>("mode") == BUTTON_PUSH)
    {
      mode = mode | BUTTON_PUSHBUTTON;
    }

    if (switchJson.get<bool>("pullup"))
    {
      mode = mode | BUTTON_SET_PULLUP;
    }

    if (switchJson.get<unsigned int>("mode") == OPEN_CLOSE_SWITCH)
    {   Bounce* debouncer1 = new Bounce();
        Bounce* debouncer2 = new Bounce();
        debouncer1->attach(switchJson.get<unsigned int>("gpioOpen"));
        debouncer1->interval(5);
        debouncer2->attach(switchJson.get<unsigned int>("gpioClose"));
       debouncer2->interval(5);
      _switchs.push_back({debouncer1});
      _switchs.push_back({debouncer2});
    }
    else
    {
      Bounce* debouncer3 =  new Bounce();
      pinMode(switchJson.get<unsigned int>("gpio"),INPUT_PULLUP);
       debouncer3->attach(switchJson.get<unsigned int>("gpio"));
        debouncer3->interval(5);
      _switchs.push_back({ debouncer3});
    }
  }
  swsSize = _switchs.size();
}

void mqttSwitchControl(String topic, String payload)
{
  for (unsigned int i = 0; i < sws.size(); i++)
  {
    JsonObject &switchJson = sws.get<JsonVariant>(i);
    if (switchJson.get<String>("mqttCommandTopic").equals(topic))
    {
      stateSwitch(switchJson, payload);
    }
  }
}

void publishState(JsonObject &switchJson)
{
  persistSwitchesFile();
  String swtr = "";
  switchJson.printTo(swtr);
  //publishOnEventSource("switch", swtr);
  if (switchJson.get<String>("type").equals("cover"))
  {
    publishOnMqtt(switchJson.get<String>("mqttStateTopic"), switchJson.get<String>("stateControlCover"), switchJson.get<bool>("mqttRetain"));
    publishOnMqtt(switchJson.get<String>("mqttPositionStateTopic"), switchJson.get<String>("positionControlCover"), switchJson.get<bool>("mqttRetain"));
  }
  else
  {
    publishOnMqtt(switchJson.get<String>("mqttStateTopic"), switchJson.get<String>("statePayload"), switchJson.get<bool>("mqttRetain"));
  }
}

JsonArray &getStoredSwitchs()
{
  return sws;
}

void loadStoredSwitchs()
{
  bool loadDefaults = false;
  if (SPIFFS.begin())
  {
    File cFile;

    if (SPIFFS.exists(switchsFilename))
    {
      cFile = SPIFFS.open(switchsFilename, "r+");
      if (!cFile)
      {
        logger("[SWITCH] Create file switchs Error!");
        return;
      }
      logger("[SWITCH] Read stored file config...");
      JsonArray &storedSwitchs = getJsonArray(cFile);
      for (int i = 0; i < storedSwitchs.size(); i++)
      {
        sws.add(storedSwitchs.get<JsonVariant>(i));
      }
      if (!storedSwitchs.success())
      {
        logger("[SWITCH] Json file parse Error!");
        loadDefaults = true;
      }
      else
      {
        logger("[SWITCH] Apply stored file config...");
        applyJsonSwitchs();
      }
    }
    else
    {
      loadDefaults = true;
    }
    cFile.close();
    if (loadDefaults)
    {
      logger("[SWITCH] Apply default config...");
      cFile = SPIFFS.open(switchsFilename, "w+");
      if(String(FACTORY_TYPE).equals("cover")){
        sws.add(getJsonObject(COVER_SWITCH.c_str()));
      }else if(String(FACTORY_TYPE).equals("light")){
        sws.add(getJsonObject(LIGHT_ONE.c_str()));
        sws.add(getJsonObject(LIGHT_TWO.c_str()));
       }
      sws.printTo(cFile);
      applyJsonSwitchs();
      cFile.close();
    }
  }
  else
  {
    logger("[SWITCH] Open file system Error!");
  }
  SPIFFS.end();
}

void persistSwitchesFile()
{

  if (SPIFFS.begin())
  {
    logger("[SWITCH] Open " + switchsFilename);
    File rFile = SPIFFS.open(switchsFilename, "w+");
    if (!rFile)
    {
      logger("[SWITCH] Open switch file Error!");
    }
    else
    {

      sws.printTo(rFile);
    }
    rFile.close();
  }
  else
  {
    logger("[SWITCH] Open file system Error!");
  }
  SPIFFS.end();
  applyJsonSwitchs();
  logger("[SWITCH] New switch config loaded.");
}

void removeSwitch(String _id)
{
  int switchFound = false;
  int index = 0;
  for (unsigned int i = 0; i < sws.size(); i++)
  {
    JsonObject &switchJson = sws.get<JsonVariant>(i);
    if (switchJson.get<String>("id").equals(_id))
    {
      switchFound = true;
      index = i;
      removeFromAlexaDiscovery(switchJson.get<String>("name"));
      removeFromHaDiscovery(switchJson.get<String>("type"), switchJson.get<String>("id"));
    }
  }
  if (switchFound)
  {
    sws.remove(index);
  }

  persistSwitchesFile();
}

void loopSwitchs()
{
  for (unsigned int i = 0; i < swsSize; i++)
  {
    Bounce *b = _switchs[i].debouncer;
   b->update();
   bool value = b->read();
   if(value != _switchs[i].lastState){
    _switchs[i].lastState = value;
    Serial.println(b->pin);
    callback(b->pin, EVENT_CHANGED);
    }
  }
}
