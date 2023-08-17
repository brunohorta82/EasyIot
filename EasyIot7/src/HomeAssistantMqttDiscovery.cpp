#include "Sensors.h"
#include "Switches.h"
#include "HomeAssistantMqttDiscovery.h"
#include "constants.h"
#include "Mqtt.h"
#include "Config.h"
extern Config config;
void initHomeAssistantDiscovery()
{
  for (auto &sw : config.switches)
  {
    if (!sw.haSupport)
      continue;
    publishOnMqtt(sw.stateTopic(), sw.getCurrentState().c_str(), true);
    addToHomeAssistant(sw);
  }
  for (auto &ss : config.sensors)
  {
    if (!ss.haSupport)
      continue;
    // TODO    publishOnMqtt(ss.stateTopic(), ss.mqttPayload, true);
    addToHomeAssistant(ss);
  }
}

void createHaSwitch(SwitchT &sw)
{
  if (!sw.haSupport)
    return;
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(8) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["unique_id"] = sw.id;
  object["cmd_t"] = sw.commandTopic();

  object["avty_t"] = getAvailableTopic();
  String family = String(sw.family);
  if (sw.isGarage())
  {
    object["stat_t"] = sw.stateTopic();
    object["payload_open"] = constanstsSwitch::payloadOpen;
    object["payload_close"] = constanstsSwitch::payloadClose;
    object["payload_stop"] = constanstsSwitch::payloadStop;
    object["state_open"] = constanstsSwitch::payloadOpen;
    object["state_closed"] = constanstsSwitch::payloadClose;
    object["state_stopped"] = constanstsSwitch::payloadStop;
    object["device_class"] = "garage";
    family = "cover";
  }

  if (sw.isCover())
  {
    object["payload_open"] = constanstsSwitch::payloadOpen;
    object["payload_close"] = constanstsSwitch::payloadClose;
    object["payload_stop"] = constanstsSwitch::payloadStop;
    object["device_class"] = "shutter";
    object["position_open"] = 0;
    object["position_closed"] = 100;
    object["position_topic"] = sw.stateTopic();
    object["set_position_topic"] = sw.commandTopic();
  }
  if (sw.isLight() || sw.isSwitch())
  {
    object["stat_t"] = sw.stateTopic();
    object["payload_on"] = constanstsSwitch::payloadOn;
    object["payload_off"] = constanstsSwitch::payloadOff;
  }
  serializeJson(object, objectStr);
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + family + "/" + String(sw.id) + "/config").c_str(), objectStr.c_str(), false);
}

void addToHomeAssistant(SensorT &s)
{
  if (!s.haSupport)
    return;
  String chip = getChipId();
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(14) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = s.name;
  object["unique_id"] = s.id;
  object["stat_t"] = s.mqttStateTopic;

  object["avty_t"] = getAvailableTopic();

  switch (s.type)
  {
  case DHT_11:
  case DHT_21:
  case DHT_22:
    object["unique_id"] = String(s.id) + "T";
    object["unit_of_measurement"] = "º";
    object["device_class"] = "temperature";
    object["value_template"] = "{{value_json.temperature}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/T" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    delay(100);
    objectStr = "";
    object["unique_id"] = String(s.id) + "H";
    object["unit_of_measurement"] = "%";
    object["device_class"] = "humidity";
    object["value_template"] = "{{value_json.humidity}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/H" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case DS18B20:
    for (int i = 0; i < s.oneWireSensorsCount; i++)
    {
      String idx = String(i + 1);
      object["name"] = String(s.name) + idx;
      object["unit_of_measurement"] = "ºC";
      object["unique_id"] = String(s.id) + idx;
      object["device_class"] = "temperature";
      object["value_template"] = String("{{value_json.temperature_") + idx + String("}}");
      objectStr = "";
      serializeJson(object, objectStr);
      publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/T" + String(s.id) + idx + "/config").c_str(), objectStr.c_str(), false);
      delay(100);
    }
    break;
  case LDR:
    object["unit_of_measurement"] = "lux";
    object["device_class"] = "illuminance";
    object["value_template"] = "{{value_json.illuminance}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/I" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PIR:
  case RCWL_0516:
    object["device_class"] = "occupancy";
    object["value_template"] = "{{value_json.binary_state}}";
    object["payload_on"] = 1;
    object["payload_off"] = 0;
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/OC" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case REED_SWITCH_NC:
  case REED_SWITCH_NO:
    object["device_class"] = "window";
    object["value_template"] = "{{value_json.binary_state}}";
    object["payload_on"] = 1;
    object["payload_off"] = 0;
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/OP" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PZEM_004T:
    object["name"] = String(s.name) + " Power";
    object["unit_of_measurement"] = "W";
    object["device_class"] = "energy";
    object["state_class"] = "measurement";
    object["unique_id"] = "P" + String(s.secondaryGpio) + chip;
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PW" + String(s.id) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Current";
    object["unit_of_measurement"] = "A";
    object["unique_id"] = "C" + String(s.secondaryGpio) + chip;
    object["value_template"] = "{{value_json.current}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/CU" + String(s.id) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Voltage";
    object["unit_of_measurement"] = "V";
    object["unique_id"] = "V" + String(s.secondaryGpio) + chip;
    object["value_template"] = "{{value_json.voltage}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/VT" + String(s.id) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Energy";
    object["unit_of_measurement"] = "kWh";
    object["unique_id"] = "E" + String(s.secondaryGpio) + chip;
    object["state_class"] = "total_increasing";
    object["value_template"] = "{{value_json.energy}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/EN" + String(s.id) + chip + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PZEM_004T_V03:
    object["name"] = String(s.name) + " Power";
    object["unique_id"] = "P" + String(s.secondaryGpio) + chip;
    object["unit_of_measurement"] = "W";
    object["device_class"] = "energy";
    object["state_class"] = "measurement";
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PW" + String(s.secondaryGpio) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Current";
    object["unique_id"] = "C" + String(s.secondaryGpio) + chip;
    object["unit_of_measurement"] = "A";
    object["value_template"] = "{{value_json.current}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/CU" + String(s.secondaryGpio) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Voltage";
    object["unique_id"] = "V" + String(s.secondaryGpio) + chip;
    object["unit_of_measurement"] = "V";
    object["value_template"] = "{{value_json.voltage}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/VT" + String(s.secondaryGpio) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " PF";
    object["unit_of_measurement"] = "";
    object["unique_id"] = "PF" + String(s.secondaryGpio) + chip;
    object["value_template"] = "{{value_json.pf}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PF" + String(s.secondaryGpio) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Frequency";
    object["unique_id"] = "F" + String(s.secondaryGpio) + chip;
    object["unit_of_measurement"] = "Hz";
    object["value_template"] = "{{value_json.frequency}}";
    objectStr = "";
    serializeJson(object, objectStr);
    (String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/FR" + String(s.secondaryGpio) + chip + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Energy";
    object["unique_id"] = "E" + String(s.secondaryGpio) + chip;
    object["unit_of_measurement"] = "kWh";
    object["state_class"] = "total_increasing";
    object["value_template"] = "{{value_json.energy}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/EN" + String(s.secondaryGpio) + chip + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    break;
  default:
    break;
  }
}
void addToHomeAssistant(SwitchT &sw)
{
  if (!sw.haSupport)
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

void removeFromHomeAssistant(SwitchT &sw)
{
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), "", false);
  delay(3);
}

void removeFromHomeAssistant(SensorT &s)
{
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/" + String(s.id) + "/config").c_str(), "", false);
  delay(3);
}