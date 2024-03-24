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
  if (!mqttConnected() || sw.isVirtual())
    return;
  String objectStr = "";
  String uniqueId = String(sw.uniqueId);
  JsonDocument doc;
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["uniq_id"] = uniqueId;
  object["cmd_t"] = sw.writeTopic;
  object["dev"] = config.healthTopic;
  JsonObject device = object["dev"].add<JsonObject>();
  device["ids"] = "OnOfre-" + String(config.chipId);
  device["name"] = config.nodeId;
#ifdef ESP32
  device["mdl"] = "V6 - " + String(config.chipId);
#endif
#ifdef ESP8266
  device["mdl"] = "V5 - " + String(config.chipId);
#endif
  device["mf"] = "OnOfre Portugal";
  device["sw"] = String(VERSION, 3);
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
  else if (sw.isGardenValve())
  {
    object["stat_t"] = sw.readTopic;
    object["ic"] = "mdi:pipe-valve";
    object["pl_on"] = ActuatorState::ON_CLOSE;
    object["pl_off"] = ActuatorState::OFF_OPEN;
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/switch/" + uniqueId + "/config").c_str(), objectStr.c_str(), false);
  }
  doc.clear();
}

void addToHomeAssistant(Sensor &s)
{
  String chip = String(config.chipId);
  String objectStr = "";
  JsonDocument doc;
  JsonObject object = doc.to<JsonObject>();
  object["name"] = s.name;
  object["uniq_id"] = s.uniqueId;
  object["stat_t"] = s.readTopic;
  object["avty_t"] = config.healthTopic;
  JsonObject device = object["dev"].add<JsonObject>();
  device["ids"] = "OnOfre-" + String(config.chipId);
  device["name"] = config.nodeId;
#ifdef ESP32
  device["mdl"] = "V6 - " + String(config.chipId);
#endif
#ifdef ESP8266
  device["mdl"] = "V5 - " + String(config.chipId);
#endif
  device["mf"] = "OnOfre Portugal";
  device["sw"] = String(VERSION, 3);
  String uniqueId = String(s.uniqueId);
  switch (s.driver)
  {
  case DHT_11:
  case DHT_21:
  case DHT_22:
  case SHT4X:
    object["uniq_id"] = "temperature" + uniqueId;
    object["unit_of_meas"] = "°C";
    object["dev_cla"] = "temperature";
    object["val_tpl"] = "{{value_json.temperature | round(2)}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1)
        objectStr = "";
    object["uniq_id"] = "humidity" + uniqueId;
    object["unit_of_meas"] = "%";
    object["dev_cla"] = "humidity";
    object["val_tpl"] = "{{value_json.humidity | round(2) }}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    break;
  case LTR303X:
    object["uniq_id"] = uniqueId;
    object["unit_of_meas"] = "lx";
    object["dev_cla"] = "illuminance";
    object["val_tpl"] = "{{value_json.lux  | round(2) }}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case HCSR04:
    object["uniq_id"] = uniqueId;
    object["unit_of_meas"] = "cm";
    object["dev_cla"] = "distance";
    object["val_tpl"] = "{{value_json.distance}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case TMF882X:
    object["uniq_id"] = uniqueId;
    object["unit_of_meas"] = "mm";
    object["dev_cla"] = "level";
    object["val_tpl"] = "{{value_json.distance}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case RAIN:
    object["uniq_id"] = uniqueId;
    object["ic"] = "mdi:weather-pouring";
    object["val_tpl"] = "{{value_json.rain}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PIR:
    object["uniq_id"] = uniqueId;
    object["dev_cla"] = "motion";
    object["pl_on"] = Payloads::motionOnPayload;
    object["pl_off"] = Payloads::motionOffPayload;
    object["val_tpl"] = "{{value_json.motion}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/binary_sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case DOOR:
    object["uniq_id"] = uniqueId;
    object["dev_cla"] = "door";
    object["pl_on"] = Payloads::windowDoornOnPayload;
    object["pl_off"] = Payloads::windowDoornOffPayload;
    object["val_tpl"] = "{{value_json.state}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/binary_sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case WINDOW:
    object["uniq_id"] = uniqueId;
    object["dev_cla"] = "window";
    object["pl_on"] = Payloads::windowDoornOnPayload;
    object["pl_off"] = Payloads::windowDoornOffPayload;
    object["val_tpl"] = "{{value_json.state}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/binary_sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case DS18B20:
    object["uniq_id"] = uniqueId;
    object["unit_of_meas"] = "°C";
    object["dev_cla"] = "temperature";
    object["val_tpl"] = "{{value_json.temperature | round(2)}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PZEM_004T_V03:
    object["name"] = String(s.name) + " Power";
    object["uniq_id"] = "p" + uniqueId;
    object["unit_of_meas"] = "W";
    object["dev_cla"] = "power";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.power | round(2)}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = String(s.name) + " Current";
    object["uniq_id"] = "c" + uniqueId;
    object["unit_of_meas"] = "A";
    object["stat_cla"] = "measurement";
    object["dev_cla"] = "current";
    object["val_tpl"] = "{{value_json.current | round(2)}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = String(s.name) + " Voltage";
    object["uniq_id"] = "v" + uniqueId;
    object["unit_of_meas"] = "V";
    object["dev_cla"] = "voltage";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.voltage | round(2)}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = String(s.name) + " Power Factor";
    object.remove("unit_of_meas");
    object["dev_cla"] = "power_factor";
    object["stat_cla"] = "measurement";
    object["uniq_id"] = "pf" + uniqueId;
    object["val_tpl"] = "{{value_json.powerFactor | round(2)}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = String(s.name) + " Frequency";
    object["uniq_id"] = "f" + uniqueId;
    object["unit_of_meas"] = "Hz";
    object["dev_cla"] = "frequency";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.frequency}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = String(s.name) + " Energy";
    object["uniq_id"] = "e" + uniqueId;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.energy | round(2)}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    break;
  case HAN:
    object["name"] = "Temperature";
    object["uniq_id"] = "temperature" + chip;
    object["unit_of_meas"] = "°C";
    object["dev_cla"] = "temperature";
    object["val_tpl"] = "{{value_json.temperature | round(2)}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Humidity";
    object["uniq_id"] = "humidity" + chip;
    object["unit_of_meas"] = "%";
    object["dev_cla"] = "humidity";
    object["val_tpl"] = "{{value_json.humidity | round(2) }}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Contract";
    object["uniq_id"] = "demandControlT1" + chip;
    object["dev_cla"] = "power";
    object["unit_of_meas"] = "kVA";
    object["val_tpl"] = "{{value_json.demandControlT1}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Tarif";
    object["uniq_id"] = "tarif" + chip;
    object["dev_cla"] = "enum";
    object["unit_of_meas"] = "";
    object["val_tpl"] = "{{value_json.tarif}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object.remove("unit_of_meas");
    object.remove("stat_cla");
    object["name"] = "Date";
    object["uniq_id"] = "dt" + chip;
    object["dev_cla"] = "enum";
    object["ic"] = "mdi:calendar";
    object["val_tpl"] = "{{value_json.dateTime}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Date Profile";
    object["uniq_id"] = "dtp" + chip;
    object["dev_cla"] = "enum";
    object["ic"] = "mdi:calendar";
    object["val_tpl"] = "{{value_json.dateProfile}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    object.remove("ic");
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Power Import";
    object["uniq_id"] = "power" + chip;
    object["unit_of_meas"] = "W";
    object["dev_cla"] = "power";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.power}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Power Export";
    object["uniq_id"] = "export" + chip;
    object["unit_of_meas"] = "W";
    object["dev_cla"] = "power";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.export}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Current";
    object["uniq_id"] = "current" + chip;
    object["unit_of_meas"] = "A";
    object["stat_cla"] = "measurement";
    object["dev_cla"] = "current";
    object["val_tpl"] = "{{value_json.current}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Voltage";
    object["uniq_id"] = "voltage" + chip;
    object["unit_of_meas"] = "V";
    object["dev_cla"] = "voltage";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.voltage}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object.remove("unit_of_meas");
    object["name"] = "Power Factor";
    object["dev_cla"] = "power_factor";
    object["stat_cla"] = "measurement";
    object["uniq_id"] = "powerFactor" + chip;
    object["val_tpl"] = "{{value_json.powerFactor}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Frequency";
    object["uniq_id"] = "frequency" + chip;
    object["unit_of_meas"] = "Hz";
    object["dev_cla"] = "frequency";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.frequency}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Energy Import";
    object["uniq_id"] = "powerImport" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.powerImport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Energy Export";
    object["uniq_id"] = "powerExport" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.powerExport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Off Peak";
    object["uniq_id"] = "rate1" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.rate1}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Peak";
    object["uniq_id"] = "rate2" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.rate2}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Middle";
    object["uniq_id"] = "rate3" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "total_increasing";
    object["val_tpl"] = "{{value_json.rate3}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Active Energy Import";
    object["uniq_id"] = "paei" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.activeEnergyImport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Reactive Energy RC";
    object["uniq_id"] = "prerc" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.reactiveEnergyRC}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Reactive Energy RI";
    object["uniq_id"] = "preri" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.reactiveEnergyRI}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object["name"] = "Active Energy Export";
    object["uniq_id"] = "paee" + chip;
    object["unit_of_meas"] = "kWh";
    object["dev_cla"] = "energy";
    object["stat_cla"] = "measurement";
    object["val_tpl"] = "{{value_json.activeEnergyExport}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
    delay(1);
    object.remove("stat_cla");
    object["name"] = "Signal";
    object["uniq_id"] = "sg" + chip;
    object["dev_cla"] = "signal_strength";
    object["unit_of_meas"] = "dB";
    object["val_tpl"] = "{{value_json.signal}}";
    objectStr.clear();
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/sensor/" + (object["uniq_id"] | "") + "/config").c_str(), objectStr.c_str(), false);
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
#ifdef DEBUG_ONOFRE
  Log.notice("%s REMOVE FROM HA %s" CR, tags::discovery, uniqueId.c_str());
#endif
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + family + "/" + uniqueId + "/config").c_str(), "\0", false);
}
void initHomeAssistantDiscovery()
{
  if (!mqttConnected())
    return;
#ifdef DEBUG_ONOFRE
  Log.notice("%s Init HomeAssistant Discovery" CR, tags::homeassistant);
#endif
  for (auto &sw : config.actuatores)
  {
    publishOnMqtt(sw.readTopic, String(sw.state).c_str(), true);
    addToHomeAssistant(sw);
  }
  for (auto &ss : config.sensors)
  {
    publishOnMqtt(ss.readTopic, ss.state.c_str(), true);
    addToHomeAssistant(ss);
  }
}