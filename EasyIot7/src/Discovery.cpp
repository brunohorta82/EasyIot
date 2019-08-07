#include "Sensors.h"
#include "Discovery.h"
#include "constants.h"

String createHaLock(const SwitchT &sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(8) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["command_topic"] = sw.mqttCommandTopic;
  //object["state_topic"] = sw.mqttStateTopic; //TODO CHECK STATE FROM SENSOR
  object["retain"] = sw.mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_lock"] = constanstsSwitch::payloadLock;
  object["payload_unlock"] = constanstsSwitch::payloadUnlock;
  serializeJson(object, objectStr);
  return objectStr;
}
String createHaSwitch(const SwitchT &sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(7) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["command_topic"] = sw.mqttCommandTopic;
  object["state_topic"] = sw.mqttStateTopic;
  object["retain"] = sw.mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_on"] = constanstsSwitch::payloadOn;
  serializeJson(object, objectStr);
  return objectStr;
}
String createHaLight(const SwitchT &sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(7) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["command_topic"] = sw.mqttCommandTopic;
  object["state_topic"] = sw.mqttStateTopic;
  object["retain"] = sw.mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_on"] = constanstsSwitch::payloadOn;
  object["payload_off"] = constanstsSwitch::payloadOff;
  serializeJson(object, objectStr);
  return objectStr;
}
String createHaCover(const SwitchT &sw)
{
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(14) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["command_topic"] = sw.mqttCommandTopic;
  object["state_topic"] = sw.mqttStateTopic;
  object["retain"] = sw.mqttRetain;
  object["availability_topic"] = getAvailableTopic();
  object["payload_open"] = constanstsSwitch::payloadOpen;
  object["payload_close"] = constanstsSwitch::payloadClose;
  object["payload_stop"] = constanstsSwitch::payloadStateStop;
  object["device_class"] = "blind";
  object["position_open"] = 100;
  object["position_closed"] = 0;
  object["position_topic"] = sw.mqttPositionStateTopic;
  object["set_position_topic"] = sw.mqttPositionCommandTopic;
  serializeJson(object, objectStr);
  return objectStr;
}

/*    name: "RSSI"
    state_topic: "home/sensor1/infojson"
    unit_of_measurement: 'dBm'
    value_template: "{{ value_json.RSSI }}"
    availability_topic: "home/sensor1/status"
    payload_available: "online"
    payload_not_available: "offline"
    json_attributes_topic: "home/sensor1/attributes" */
void addToHaDiscovery(const SensorT &s)
{
  //TODO
  switch (s.type)
  {
  case DHT_11:
  case DHT_21:
  case DHT_22:
    break;
  case DS18B20:
    break;
  case LDR:
    break;
  case PIR:
  case REED_SWITCH:
  case RCWL_0516:
    break;

  default:
    break;
  }
}
void addToHaDiscovery(const SwitchT &sw)
{
  if (strlen(getAtualConfig().mqttIpDns) == 0)
  {
    Log.warning("%s Setup required to publish discovery messages" CR, tags::mqtt);
    return;
  }
  if (strlen(getAtualConfig().mqttIpDns) == 0)
  {
    Log.warning("%s Mqtt not configured" CR, tags::discovery);
    return;
  }

  String payload = "";
  if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
  {
    payload = createHaCover(sw);
  }
  else if (strcmp(sw.family, constanstsSwitch::familyLight) == 0)
  {
    payload = createHaLight(sw);
  }
  else if (strcmp(sw.family, constanstsSwitch::familySwitch) == 0)
  {
    payload = createHaSwitch(sw);
  }
  else if (strcmp(sw.family, constanstsSwitch::familyLock) == 0)
  {
    payload = createHaLock(sw);
  }

  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), payload.c_str(), false);
  subscribeOnMqtt(sw.mqttCommandTopic);
  Log.notice("%s RELOAD HA SWITCH DISCOVERY OK" CR, tags::discovery);
}

void removeFromHaDiscovery(const SwitchT &sw)
{
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), "", false);
}

void removeFromHaDiscovery(const SensorT &s)
{
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/" + String(s.id) + "/config").c_str(), "", false);
}
