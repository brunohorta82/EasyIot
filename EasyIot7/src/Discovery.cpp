

#include "Discovery.h"
#include "Sensors.h"

#define DISCOVERY_TAG "[DISCOVERY]"
String createHaLock(SwitchT *sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(8) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw->name;
  object["command_topic"] = sw->mqttCommandTopic;
  //object["state_topic"] = sw->mqttStateTopic; //TODO CHECK STATE FROM SENSOR
  object["retain"] = sw->mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_lock"] = String(PAYLOAD_LOCK);
  object["payload_unlock"] = String(PAYLOAD_UNLOCK);
  serializeJson(object, objectStr);
  return objectStr;
}
String createHaSwitch(SwitchT *sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(7) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw->name;
  object["command_topic"] = sw->mqttCommandTopic;
  object["state_topic"] = sw->mqttStateTopic;
  object["retain"] = sw->mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_on"] = String(PAYLOAD_ON);
  serializeJson(object, objectStr);
  return objectStr;
}
String createHaLight(SwitchT *sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(7) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw->name;
  object["command_topic"] = sw->mqttCommandTopic;
  object["state_topic"] = sw->mqttStateTopic;
  object["retain"] = sw->mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_on"] = String(PAYLOAD_ON);
  object["payload_off"] = String(PAYLOAD_OFF);
  serializeJson(object, objectStr);
  return objectStr;
}
String createHaCover(SwitchT *sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(14) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw->name;
  object["command_topic"] = sw->mqttCommandTopic;
  object["state_topic"] = sw->mqttStateTopic;
  object["retain"] = sw->mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_open"] = String(PAYLOAD_OPEN);
  object["payload_close"] = String(PAYLOAD_CLOSE);
  object["payload_stop"] = String(PAYLOAD_STOP);
  object["device_class"] = "blind";
  object["position_open"] = 100;
  object["position_closed"] = 0;
  object["position_topic"] = String(sw->mqttPositionStateTopic);
  object["set_position_topic"] = String(sw->mqttPositionCommandTopic);
  serializeJson(object, objectStr);
  return objectStr;
}
void addToAlexaDiscovery(SwitchT *sw)
{
  addSwitchToAlexa(sw->name);
}

/*    name: "RSSI"
    state_topic: "home/sensor1/infojson"
    unit_of_measurement: 'dBm'
    value_template: "{{ value_json.RSSI }}"
    availability_topic: "home/sensor1/status"
    payload_available: "online"
    payload_not_available: "offline"
    json_attributes_topic: "home/sensor1/attributes" */
void addToHaDiscovery(SensorT *s)
{
  //TODO
  switch (s->type)
  {
  case TYPE_DHT_11:
  case TYPE_DHT_21:
  case TYPE_DHT_22:
    break;
  case TYPE_DS18B20:
    break;
  case TYPE_LDR:
  break;
  case TYPE_PIR:
  case TYPE_REED_SWITCH:
  case TYPE_RCWL_0516:
  break;

  default:
    break;
  }
}
void addToHaDiscovery(SwitchT *sw)
{

  if (strlen(getAtualConfig().mqttIpDns) == 0)
  {
    Log.warning("%s Mqtt not configured" CR, DISCOVERY_TAG);
    return;
  }

  String payload = "";
  if (strcmp(sw->family, FAMILY_COVER) == 0)
  {
    payload = createHaCover(sw);
  }
  else if (strcmp(sw->family, FAMILY_LIGHT) == 0)
  {
    payload = createHaLight(sw);
  }
  else if (strcmp(sw->family, FAMILY_SWITCH) == 0)
  {
    payload = createHaSwitch(sw);
  }
  else if (strcmp(sw->family, FAMILY_LOCK) == 0)
  {
    payload = createHaLock(sw);
  }

  

  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw->family) + "/" + String(sw->id) + "/config").c_str(), payload.c_str(), false);
  subscribeOnMqtt(sw->mqttCommandTopic);
  Log.notice("%s RELOAD HA SWITCH DISCOVERY OK" CR, DISCOVERY_TAG);
}

void removeFromHaDiscovery(SwitchT *sw)
{
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw->family) + "/" + String(sw->id) + "/config").c_str(), "", false);
}
void removeFromHaDiscovery(SensorT *s)
{
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s->family) + "/" + String(s->id) + "/config").c_str(), "", false);
}
void removeFromAlexaDiscovery(SwitchT *sw)
{
  removeSwitchFromAlexa(sw->name);
}