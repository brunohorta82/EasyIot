#include "Sensors.h"
#include "Switches.h"
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
    publishOnMqtt(sw.mqttStateTopic, sw.getCurrentState().c_str(), true);
    addToHaDiscovery(sw);
  }
}

void initHaDiscovery(const Sensors &sensors)
{
  for (const auto &ss : sensors.items)
  {
    if (!ss.haSupport)
      continue;
    publishOnMqtt(ss.mqttStateTopic, ss.mqttPayload, true);
    addToHaDiscovery(ss);
  }
}

void createHaSwitch(const SwitchT &sw)
{
  if (!sw.haSupport)
    return;
  String objectStr = "";
  const size_t capacity = JSON_OBJECT_SIZE(8) + 300;
  DynamicJsonDocument doc(capacity);
  JsonObject object = doc.to<JsonObject>();
  object["name"] = sw.name;
  object["unique_id"] = sw.id;
  object["cmd_t"] = sw.mqttCommandTopic;

  object["avty_t"] = getAvailableTopic();
  String family = String(sw.family);
  if (strcmp(sw.family, constanstsSwitch::familyGate) == 0)
  {
    object["stat_t"] = sw.mqttStateTopic;
    object["payload_open"] = constanstsSwitch::payloadOpen;
    object["payload_close"] = constanstsSwitch::payloadClose;
    object["payload_stop"] = constanstsSwitch::payloadStop;
    object["state_open"] = constanstsSwitch::payloadOpen;
    object["state_closed"] = constanstsSwitch::payloadClose;
    object["state_stopped"] = constanstsSwitch::payloadStop;
    object["device_class"] = "garage";
    family = "cover";
  }

  if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
  {
    object["payload_open"] = constanstsSwitch::payloadOpen;
    object["payload_close"] = constanstsSwitch::payloadClose;
    object["payload_stop"] = constanstsSwitch::payloadStop;
    object["device_class"] = "shutter";
    object["position_open"] = 0;
    object["position_closed"] = 100;
    object["position_topic"] = sw.mqttStateTopic;
    object["set_position_topic"] = sw.mqttCommandTopic;
  }
  if (strcmp(sw.family, constanstsSwitch::familyLight) == 0 || strcmp(sw.family, constanstsSwitch::familySwitch) == 0)
  {
    object["stat_t"] = sw.mqttStateTopic;
    object["payload_on"] = constanstsSwitch::payloadOn;
    object["payload_off"] = constanstsSwitch::payloadOff;
  }
  serializeJson(object, objectStr);
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + family + "/" + String(sw.id) + "/config").c_str(), objectStr.c_str(), false);
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
    object["device_class"] = "power";
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PW" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Current";
    object["unit_of_measurement"] = "A";
    object["value_template"] = "{{value_json.current}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/CU" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Voltage";
    object["unit_of_measurement"] = "V";
    object["value_template"] = "{{value_json.voltage}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/VT" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    object["name"] = String(s.name) + " Energy";
    object["unit_of_measurement"] = "kWh";
    object["device_class"] = "energy";
    object["state_class"] = "total";
    object["value_template"] = "{{value_json.energy}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/EN" + String(s.id) + "/config").c_str(), objectStr.c_str(), false);
    break;
  case PZEM_004T_V03:
    object["name"] = String(s.name) + " Power";
    object["unique_id"] = "P" + String(s.secondaryGpio);
    object["unit_of_measurement"] = "W";
    object["device_class"] = "energy";
    object["value_template"] = "{{value_json.power}}";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PW" + String(s.secondaryGpio) + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Current";
    object["unique_id"] = "C" + String(s.secondaryGpio);
    object["unit_of_measurement"] = "A";
    object["value_template"] = "{{value_json.current}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/CU" + String(s.secondaryGpio) + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Voltage";
    object["unique_id"] = "V" + String(s.secondaryGpio);
    object["unit_of_measurement"] = "V";
    object["value_template"] = "{{value_json.voltage}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/VT" + String(s.secondaryGpio) + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Energy";
    object["unique_id"] = "E" + String(s.secondaryGpio);
    object["unit_of_measurement"] = "kWh";
    object["device_class"] = "energy";
    object["state_class"] = "total";
    object["value_template"] = "{{value_json.energy}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/EN" + String(s.secondaryGpio) + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " PF";
    object["unit_of_measurement"] = "";
    object["unique_id"] = "PF" + String(s.secondaryGpio);
    object["value_template"] = "{{value_json.pf}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/PF" + String(s.secondaryGpio) + "/config").c_str(), objectStr.c_str(), false);
    delay(200);
    object["name"] = String(s.name) + " Frequency";
    object["unique_id"] = "F" + String(s.secondaryGpio);
    object["unit_of_measurement"] = "Hz";
    object["value_template"] = "{{value_json.frequency}}";
    objectStr = "";
    serializeJson(object, objectStr);
    publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/FR" + String(s.secondaryGpio) + "/config").c_str(), objectStr.c_str(), false);
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
#ifdef DEBUG
    Log.warning("%s Setup required to publish discovery messages" CR, tags::mqtt);
#endif
    return;
  }
  if (strlen(getAtualConfig().mqttIpDns) == 0)
  {
#ifdef DEBUG
    Log.warning("%s Mqtt not configured" CR, tags::discovery);
#endif
    return;
  }

  createHaSwitch(sw);

#ifdef DEBUG
  Log.notice("%s RELOAD HA SWITCH DISCOVERY OK" CR, tags::discovery);
#endif
}

void removeFromHaDiscovery(const SwitchT &sw)
{
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(sw.family) + "/" + String(sw.id) + "/config").c_str(), "", false);
  delay(3);
}

void removeFromHaDiscovery(const SensorT &s)
{
  publishOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/" + String(s.family) + "/" + String(s.id) + "/config").c_str(), "", false);
  delay(3);
}
