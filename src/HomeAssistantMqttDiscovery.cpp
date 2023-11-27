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
  const size_t capacity = JSON_OBJECT_SIZE(8) + 400;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["uniq_id"] = uniqueId;
  object["cmd_t"] = sw.writeTopic;
  object["dev"] = config.healthTopic;
  JsonObject device = object.createNestedObject("dev");
  device["ids"] = "OnOfre-" + String(config.chipId);
  device["name"] = "OnOfre " + String(config.nodeId);
  device["mdl"] = config.chipId;
  device["mf"] = "OnOfre Portugal";
  object["avty_t"] = config.healthTopic;

  if (sw.isGarage())
  {
    object["stat_t"] = sw.readTopic;
    object["pl_open"] = ActuatorState::OFF_OPEN;
    object["pl_cls"] = ActuatorState::ON_CLOSE;
    object["pl_stop"] = ActuatorState::STOP;
    object["stat_open"] = ActuatorState::OFF_OPEN;
    object["stat_clsd"] = ActuatorState::ON_CLOSE;
    object["stat_stopped"] = ActuatorState::STOP;
    object["dev_cla"] = "garage";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/cover/" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
  }
  else if (sw.isCover())
  {
    object["pl_open"] = ActuatorState::OFF_OPEN;
    object["pl_cls"] = ActuatorState::ON_CLOSE;
    object["pl_stop"] = ActuatorState::STOP;
    object["dev_cla"] = "shutter";
    object["pos_open"] = ActuatorState::OFF_OPEN;
    object["pos_clsd"] = ActuatorState::ON_CLOSE;
    object["pos_t"] = sw.readTopic;
    object["set_pos_t"] = sw.writeTopic;
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/cover/" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
  }
  else if (sw.isLight() || sw.isSwitch())
  {
    object["stat_t"] = sw.readTopic;
    object["pl_on"] = ActuatorState::ON_CLOSE;
    object["pl_off"] = ActuatorState::OFF_OPEN;
    String family = sw.familyToText();
    family.toLowerCase();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + family + "/" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
  }
  doc.clear();
}

void addToHomeAssistant(Sensor &s)
{
  String chip = String(config.chipId);
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(14) + 400;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = s.name;
  object["uniq_id"] = s.uniqueId;
  object["stat_t"] = s.readTopic;
  object["avty_t"] = config.healthTopic;
  JsonObject device = object.createNestedObject("dev");
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
  case SHT4X:
    object["uniq_id"] = uniqueId + "t";
    object["unit_of_meas"] = "º";
    object["dev_cla"] = "temperature";
    object["val_tpl"] = "{{value_json.temperature}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/t" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    objectStr = "";
    object["uniq_id"] = uniqueId + "h";
    object["unit_of_meas"] = "%";
    object["dev_cla"] = "humidity";
    object["val_tpl"] = "{{value_json.humidity}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/h" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    break;
  case LTR303X:
    object["uniq_id"] = uniqueId + "lx";
    object["unit_of_meas"] = "lx";
    object["dev_cla"] = "illuminance";
    object["val_tpl"] = "{{value_json.lux}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/tx" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    objectStr = "";
    object["uniq_id"] = uniqueId + "h";
    object["unit_of_meas"] = "%";
    object["dev_cla"] = "humidity";
    object["val_tpl"] = "{{value_json.humidity}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/h" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    break;
  case DS18B20:
    object["name"] = String(s.name);
    object["unit_of_meas"] = "ºC";
    object["uniq_id"] = uniqueId;
    object["dev_cla"] = "temperature";
    object["val_tpl"] = "{{value_json.temperature}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/t" + uniqueId + "/config").c_str(), objectStr.c_str(), false);

    break;
  case PZEM_004T_V03:
    object["name"] = String(s.name) + " Power";
    object["uniq_id"] = "p" + uniqueId;
    object["unit_of_meas"] = "W";
    object["dev_cla"] = "power";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/p" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Current";
    object["uniq_id"] = "c" + uniqueId;
    object["unit_of_meas"] = "A";
    object["stat_cla"] = "measurement";
    object["dev_cla"] = "current";
    object["val_tpl"] = "{{value_json.current}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/c" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Voltage";
    object["uniq_id"] = "v" + uniqueId;
    object["unit_of_meas"] = "V";
    object["dev_cla"] = "voltage";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.voltage}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/v" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Power Factor";
    object["unit_of_meas"] = "%";
    object["dev_cla"] = "power_factor";
    object["stat_cla"] = "measurement";
    object["uniq_id"] = "pf" + uniqueId;
    object["val_tpl"] = "{{value_json.powerFactor}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/pf" + uniqueId + "/config").c_str(), objectStr.c_str(), false);

    object["name"] = String(s.name) + " Frequency";
    object["uniq_id"] = "f" + uniqueId;
    object["unit_of_meas"] = "Hz";
    object["dev_cla"] = "frequency";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.frequency}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/f" + uniqueId + "/config").c_str(), objectStr.c_str(), false);

    object["name"] = String(s.name) + " Energy";
    object["uniq_id"] = "e" + uniqueId;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.energy}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/e" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
    break;
  case HAN:
    object["name"] = "Power";
    object["uniq_id"] = "p" + chip;
    object["unit_of_meas"] = "W";
    object["dev_cla"] = "power";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/p" + chip + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Current";
    object["uniq_id"] = "c" + chip;
    object["unit_of_meas"] = "A";
    object["stat_cla"] = "measurement";
    object["dev_cla"] = "current";
    object["val_tpl"] = "{{value_json.current}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/c" + chip + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Voltage";
    object["uniq_id"] = "v" + chip;
    object["unit_of_meas"] = "V";
    object["dev_cla"] = "voltage";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.voltage}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/v" + chip + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Power Factor";
    object["unit_of_meas"] = "%";
    object["dev_cla"] = "power_factor";
    object["stat_cla"] = "measurement";
    object["uniq_id"] = "pf" + chip;
    object["val_tpl"] = "{{value_json.powerFactor}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/pf" + chip + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Frequency";
    object["uniq_id"] = "f" + chip;
    object["unit_of_meas"] = "Hz";
    object["dev_cla"] = "frequency";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.frequency}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/f" + chip + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Power Import";
    object["uniq_id"] = "e" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.powerImport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/e" + chip + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = "Power Export";
    object["uniq_id"] = "ex" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.powerExport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/ex" + chip + "/config").c_str(), objectStr.c_str(), false);
    break;
  default:
    break;
  }
  doc.clear();
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