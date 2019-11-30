#include "Sensors.h"
#include "Discovery.h"
#include "constants.h"
#include "Mqtt.h"
#include "Config.h"
void initHaDiscovery(const Switches &switches)
{
  for (const auto &sw : switches.items)
  {
    if (!sw.haSupport)
      continue;
    addToHaDiscovery(sw);
    publishOnMqtt(sw.mqttStateTopic, sw.mqttPayload, true);
  }
}

void initHaDiscovery(const Sensors &sensors)
{
  for (const auto &ss : sensors.items)
  {
    if (!ss.haSupport)
      continue;
    addToHaDiscovery(ss);
    publishOnMqtt(ss.mqttStateTopic, ss.mqttPayload, true);
  }
}

void createHaLock(const SwitchT &sw)
{
  if (!sw.haSupport)
    return;
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
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), objectStr.c_str(), false);
}
void createHaSwitch(const SwitchT &sw)
{
  if (!sw.haSupport)
    return;
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
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), objectStr.c_str(), false);
}
void createHaLight(const SwitchT &sw)
{
  if (!sw.haSupport)
    return;
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
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), objectStr.c_str(), false);
}
void createHaCover(const SwitchT &sw)
{
  if (!sw.haSupport)
    return;
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
  object["payload_stop"] = constanstsSwitch::payloadStop;
  object["device_class"] = "blind";
  object["position_open"] = 100;
  object["position_closed"] = 0;
  object["position_topic"] = sw.mqttPositionStateTopic;
  object["set_position_topic"] = sw.mqttPositionCommandTopic;
  serializeJson(object, objectStr);
  publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), objectStr.c_str(), false);
}

void addToHaDiscovery(const SensorT &s)
{
  if (!s.haSupport)
    return;
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(14) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = s.name;
  object["state_topic"] = s.mqttStateTopic;
  object["availability_topic"] = getAvailableTopic();
  switch (s.type)
  {
  case DHT_11:
  case DHT_21:
  case DHT_22:
    object["unit_of_measurement"] = "%";
    object["device_class"] = "humidity";
    object["value_template"] = "{{value_json.humidity}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/H" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    objectStr = "";
    object["unit_of_measurement"] = "ºC";
    object["device_class"] = "temperature";
    object["value_template"] = "{{value_json.temperature}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/T" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case DS18B20:
    object["unit_of_measurement"] = "ºC";
    object["device_class"] = "temperature";
    object["value_template"] = "{{value_json.temperature}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/T" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case LDR:
    object["unit_of_measurement"] = "lux";
    object["device_class"] = "illuminance";
    object["value_template"] = "{{value_json.illuminance}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/I" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PIR:
  case RCWL_0516:
    object["device_class"] = "occupancy";
    object["value_template"] = "{{value_json.binary_state}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/OC" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case REED_SWITCH:
    object["device_class"] = "window";
    object["value_template"] = "{{value_json.binary_state}}";
    object["payload_on"] = 1;
    object["payload_off"] = 0;
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/OP" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PZEM_004T:
    object["name"] = String(s.name) + " Power";
    object["unit_of_measurement"] = "W";
    object["device_class"] = "power";
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PW" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Current";
    object["unit_of_measurement"] = "A";
    object["value_template"] = "{{value_json.current}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/CU" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Voltage";
    object["unit_of_measurement"] = "V";
    object["value_template"] = "{{value_json.voltage}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/VT" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Energy";
    object["unit_of_measurement"] = "KhW";
    object["device_class"] = "power";
    object["value_template"] = "{{value_json.energy}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/EN" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PZEM_004T_V03:
    object["name"] = String(s.name) + " Power";
    object["unit_of_measurement"] = "W";
    object["device_class"] = "power";
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PW" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Current";
    object["unit_of_measurement"] = "A";
    object["value_template"] = "{{value_json.current}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/CU" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Voltage";
    object["unit_of_measurement"] = "V";
    object["value_template"] = "{{value_json.voltage}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/VT" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Energy";
    object["unit_of_measurement"] = "KhW";
    object["device_class"] = "power";
    object["value_template"] = "{{value_json.energy}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/EN" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " PF";
    object["value_template"] = "{{value_json.pf}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PF" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Frequency";
    object["unit_of_measurement"] = "Hz";
    object["value_template"] = "{{value_json.frequency}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(getAtualConfig().homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/FR" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;

  default:
    break;
  }
}
void addToHaDiscovery(const SwitchT &sw)
{
  if (!sw.haSupport)
    return;
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

  if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
  {
    createHaCover(sw);
  }
  else if (strcmp(sw.family, constanstsSwitch::familyLight) == 0)
  {
    createHaLight(sw);
  }
  else if (strcmp(sw.family, constanstsSwitch::familySwitch) == 0)
  {
    createHaSwitch(sw);
  }
  else if (strcmp(sw.family, constanstsSwitch::familyLock) == 0)
  {
    createHaLock(sw);
  }

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
