#include "HomeAssistantMqttDiscovery.h"
#include "Mqtt.h"
#include "ConfigOnofre.h"
extern ConfigOnofre config;

bool homeAssistantOnline(String topic, String payload)
{
  return (topic.compareTo(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/status")) == 0 || topic.compareTo(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefixLegacy) + "/status")) == 0) && payload.compareTo(constantsMqtt::availablePayload) == 0;
}
void createHaSwitch(Actuator &sw)
{
  if (!mqttConnected())
    return;
  String objectStr = "";
  String uniqueId = String(sw.uniqueId);
  const size_t capacity = JSON_OBJECT_SIZE(8) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["unique_id"] = uniqueId;
  object["cmd_t"] = sw.writeTopic;
  object["device"] = config.healthTopic;
  JsonObject device = object.createNestedObject("device");
  device["ids"] = "OnOfre-" + String(config.chipId);
  device["name"] = "OnOfre " + String(config.nodeId);
  device["mdl"] = config.chipId;
  device["mf"] = "OnOfre Portugal";
  object["avty_t"] = config.healthTopic;
  String family = sw.familyToText();
  family.toLowerCase();
  if (sw.isGarage())
  {
    object["stat_t"] = sw.readTopic;
    object["payload_open"] = ActuatorState::OFF_OPEN;
    object["payload_close"] = ActuatorState::ON_CLOSE;
    object["payload_stop"] = ActuatorState::STOP;
    object["state_open"] = ActuatorState::OFF_OPEN;
    object["state_closed"] = ActuatorState::ON_CLOSE;
    object["state_stopped"] = ActuatorState::STOP;
    object["device_class"] = "garage";
    family = "cover";
  }

  if (sw.isCover())
  {
    object["payload_open"] = ActuatorState::OFF_OPEN;
    object["payload_close"] = ActuatorState::ON_CLOSE;
    object["payload_stop"] = ActuatorState::STOP;
    object["device_class"] = "shutter";
    object["position_open"] = ActuatorState::OFF_OPEN;
    object["position_closed"] = ActuatorState::ON_CLOSE;
    object["position_topic"] = sw.readTopic;
    object["set_position_topic"] = sw.writeTopic;
  }
  if (sw.isLight() || sw.isSwitch())
  {
    object["stat_t"] = sw.readTopic;
    object["payload_on"] = ActuatorState::ON_CLOSE;
    object["payload_off"] = ActuatorState::OFF_OPEN;
  }
  serializeJson(object, objectStr);
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + family + "/" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
}

void addToHomeAssistant(Sensor &s)
{
  String chip = String(config.chipId);
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(14) + 400;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = s.name;
  object["unique_id"] = s.uniqueId;
  object["stat_t"] = s.readTopic;
  object["avty_t"] = config.healthTopic;
  JsonObject device = object.createNestedObject("device");
  device["ids"] = "OnOfre-" + String(config.chipId);
  device["name"] = "OnOfre " + String(config.nodeId);
  device["mdl"] = config.chipId;
  device["mf"] = "OnOfre Portugal";
  String uniqueId = String(s.uniqueId);
  switch (s.driver)
  {
  case DHT_11:
  case DHT_21:
  case DHT_22:
  case SHT3x_SENSOR:
    object["unique_id"] = uniqueId + "t";
    object["unit_of_measurement"] = "º";
    object["device_class"] = "temperature";
    object["value_template"] = "{{value_json.temperature}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/t" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    objectStr = "";
    object["unique_id"] = uniqueId + "h";
    object["unit_of_measurement"] = "%";
    object["device_class"] = "humidity";
    object["value_template"] = "{{value_json.humidity}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/h" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    break;
  case DS18B20:
    object["name"] = String(s.name);
    object["unit_of_measurement"] = "ºC";
    object["unique_id"] = uniqueId;
    object["device_class"] = "temperature";
    object["value_template"] = "{{value_json.temperature}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/t" + uniqueId + "/config").c_str(), objectStr.c_str(), false);

    break;
  case PZEM_004T_V03:
    object["name"] = String(s.name) + " Power";
    object["unique_id"] = "p" + uniqueId;
    object["unit_of_measurement"] = "W";
    object["device_class"] = "power";
    object["state_class"] = "measurement";
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/p" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Current";
    object["unique_id"] = "c" + uniqueId;
    object["unit_of_measurement"] = "A";
    object["state_class"] = "measurement";
    object["device_class"] = "current";
    object["value_template"] = "{{value_json.current}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/c" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Voltage";
    object["unique_id"] = "v" + uniqueId;
    object["unit_of_measurement"] = "V";
    object["device_class"] = "voltage";
    object["state_class"] = "measurement";
    object["value_template"] = "{{value_json.voltage}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/v" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " PF";
    object["unit_of_measurement"] = "";
    object["device_class"] = "power_factor";
    object["state_class"] = "measurement";
    object["unique_id"] = "pf" + uniqueId;
    object["value_template"] = "{{value_json.powerFactor}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/pf" + uniqueId + "/config").c_str(), objectStr.c_str(), false);

    object["name"] = String(s.name) + " Frequency";
    object["unique_id"] = "f" + uniqueId;
    object["unit_of_measurement"] = "Hz";
    object["device_class"] = "frequency";
    object["state_class"] = "measurement";
    object["value_template"] = "{{value_json.frequency}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/f" + uniqueId + "/config").c_str(), objectStr.c_str(), false);

    object["name"] = String(s.name) + " Energy";
    object["unique_id"] = "e" + uniqueId;
    object["unit_of_measurement"] = "kWh";
    object["device_class"] = "energy";
    object["state_class"] = "total_increasing";
    object["value_template"] = "{{value_json.energy}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/e" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    break;
  case HAN:
    object["name"] = "Power";
    object["unique_id"] = "p" + uniqueId;
    object["unit_of_measurement"] = "W";
    object["device_class"] = "power";
    object["state_class"] = "measurement";
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/p" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Current";
    object["unique_id"] = "c" + uniqueId;
    object["unit_of_measurement"] = "A";
    object["state_class"] = "measurement";
    object["device_class"] = "current";
    object["value_template"] = "{{value_json.current}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/c" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Voltage";
    object["unique_id"] = "v" + uniqueId;
    object["unit_of_measurement"] = "V";
    object["device_class"] = "voltage";
    object["state_class"] = "measurement";
    object["value_template"] = "{{value_json.voltage}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/v" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Power Factor";
    object["unit_of_measurement"] = "";
    object["device_class"] = "power_factor";
    object["state_class"] = "measurement";
    object["unique_id"] = "pf" + uniqueId;
    object["value_template"] = "{{value_json.powerFactor}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/pf" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Frequency";
    object["unique_id"] = "f" + uniqueId;
    object["unit_of_measurement"] = "Hz";
    object["device_class"] = "frequency";
    object["state_class"] = "measurement";
    object["value_template"] = "{{value_json.frequency}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/f" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Power Import";
    object["unique_id"] = "e" + uniqueId;
    object["unit_of_measurement"] = "kWh";
    object["device_class"] = "energy";
    object["state_class"] = "total_increasing";
    object["value_template"] = "{{value_json.powerImport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/e" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Power Export";
    object["unique_id"] = "ex" + uniqueId;
    object["unit_of_measurement"] = "kWh";
    object["device_class"] = "energy";
    object["state_class"] = "total_increasing";
    object["value_template"] = "{{value_json.powerExport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/ex" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    break;
  default:
    break;
  }
}
void addToHomeAssistant(Actuator &sw)
{
  if (!mqttConnected())
    return;
  if (strlen(config.mqttIpDns) == 0)
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s Setup required to publish discovery messages" CR, tags::mqtt);
#endif
    return;
  }
  if (strlen(config.mqttIpDns) == 0)
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s Mqtt not configured" CR, tags::discovery);
#endif
    return;
  }

  createHaSwitch(sw);

#ifdef DEBUG_ONOFRE
  Log.notice("%s RELOAD HA SWITCH DISCOVERY OK" CR, tags::discovery);
#endif
}

void removeFromHomeAssistant(String family, String uniqueId)
{
  if (!mqttConnected())
    return;
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + family + "/" + uniqueId + "/config").c_str(), "", false);
}
void initHomeAssistantDiscovery()
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Init HomeAssistant Discovery" CR, tags::homeassistant);
#endif
  for (auto &sw : config.actuatores)
  {
    if (!mqttConnected())
      return;
    publishOnMqtt(sw.readTopic, String(sw.state).c_str(), true);
    addToHomeAssistant(sw);
  }
  for (auto &ss : config.sensors)
  {
    publishOnMqtt(ss.readTopic, String(ss.state).c_str(), true);
    addToHomeAssistant(ss);
  }
}