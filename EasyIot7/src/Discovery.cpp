

#include "Discovery.h"
#define DISCOVERY_TAG "[DISCOVEY]"
String createHaLock(SwitchT sw)
{
  String objectStr = "";
const size_t capacity = JSON_OBJECT_SIZE(7 );
DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"]= sw.name;
  object["command_topic"]= sw.mqttCommandTopic;
  object["state_topic"]=sw.mqttStateTopic;
  object["retain"]=sw.mqttRetain;
  object["availability_topic"]= getAvailableTopic();
  object["payload_lock"]= String(PAYLOAD_LOCK);
  object["payload_unlock"]= String(PAYLOAD_UNLOCK);
  serializeJson(object, objectStr);
  return objectStr;
}
void  rebuildDiscoverySwitchMqttTopics(SwitchT sw)
{
  if(strlen(getAtualConfig().mqttIpDns) == 0)   return;

  String prefix = String(getAtualConfig().homeAssistantAutoDiscoveryPrefix);
    String _id = String(sw.id);
    String _name = String(sw.name);
    String family = String(sw.family);
    String payload = "";
    if (family.equals("cover"))
    {
   //  payload =  createHaCover(sw);
    }
    else if (family.equals("light"))
    {
     // payload =  createHaLight(sw);
    }
    else if (family.equals("switch"))
    {
     // payload = createHaSwitch(sw);
    }
    else if (family.equals("lock"))
    {
     // payload = createHaLock(sw);
      
    }
     publishOnMqtt(prefix + "/"+family+"/" +  _id + "/config", payload, true);
    subscribeOnMqtt(String(sw.mqttCommandTopic));
    logger(DISCOVERY_TAG, "RELOAD MQTT SWITCH DISCOVERY OK");
}


/* 
String createHaSwitch(JsonObject &_switchJson)
{
  String object = "";
  const size_t capacity = JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& switchJson = jsonBuffer.createObject();
  switchJson.set("name", _switchJson.get<String>("name"));
  switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
  switchJson.set("state_topic", _switchJson.get<String>("mqttStateTopic"));
  switchJson.set("retain", _switchJson.get<bool>("retain"));
  switchJson.set("payload_on", String(PAYLOAD_ON));
  switchJson.set("availability_topic", getAvailableTopic());
  switchJson.set("payload_off", String(PAYLOAD_OFF));
  switchJson.printTo(object);
  return object;
}
String createHaLight(JsonObject &_switchJson)
{
  String object = "";
  const size_t capacity = JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& switchJson = jsonBuffer.createObject();
  switchJson.set("name", _switchJson.get<String>("name"));
  switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
  switchJson.set("state_topic", _switchJson.get<String>("mqttStateTopic"));
  switchJson.set("retain", _switchJson.get<bool>("retain"));
  switchJson.set("availability_topic", getAvailableTopic());
  switchJson.set("payload_on", String(PAYLOAD_ON));
  switchJson.set("payload_off", String(PAYLOAD_OFF));
  switchJson.printTo(object);
  return object;
}
String createHaCover(JsonObject &_switchJson)
{
  String object = "";
  const size_t capacity = JSON_OBJECT_SIZE(10);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& switchJson = jsonBuffer.createObject();
  switchJson.set("name", _switchJson.get<String>("name"));
  switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
  switchJson.set("position_topic", _switchJson.get<String>("mqttPositionStateTopic"));
  switchJson.set("retain", _switchJson.get<bool>("retain"));
  switchJson.set("position_open", 100);
  switchJson.set("availability_topic", getAvailableTopic());
  switchJson.set("position_closed", 0);
  switchJson.set("payload_open", String(PAYLOAD_OPEN));
  switchJson.set("payload_close", String(PAYLOAD_CLOSE));
  switchJson.set("payload_stop", String(PAYLOAD_STOP));
  switchJson.printTo(object);
  return object;
}
void reloadMqttSubscriptionsAndDiscovery(){
  
  String ipMqtt = getConfigJson().get<String>("mqttIpDns");
  if (ipMqtt == "")
    return;
   String prefix = getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix");
    JsonArray &switches = getStoredSwitchs();
  for (int i = 0; i < switches.size(); i++)
  {
    JsonObject &switchJson = switches.get<JsonVariant>(i);
     
    String type = switchJson.get<String>("type");
    if (type.equals("cover"))
    {  
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    else if (type.equals("light"))
    {
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    else if (type.equals("switch"))
    {
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    else if (type.equals("lock"))
    { 
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    rebuildDiscoverySwitchMqttTopics(switchJson);
  }
    JsonArray &sensors = getStoredSensors();
  for (int i = 0; i < sensors.size(); i++)
  {
    JsonObject& sensorJson = sensors.get<JsonVariant>(i);
     rebuildDiscoverySensorMqttTopics(sensorJson);
  }
  
    logger("[MQTT] RELOAD MQTT SUBSCRIPTIONS OK");
}


void removeFromAlexaDiscovery(String _name)
{
  fauxmo.removeDevice(_name.c_str());
}

void removeFromHaDiscovery(String type, String _id)
{
  publishOnMqtt(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix") + "/" + type +"/" + _id + "/config", "", false);
}
*/