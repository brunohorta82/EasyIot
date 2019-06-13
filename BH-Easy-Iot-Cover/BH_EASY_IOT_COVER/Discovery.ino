
fauxmoESP fauxmo;

void startAlexaDiscovery()
{
  fauxmo.createServer(false);
  fauxmo.setPort(80); // required for gen3 devices
  fauxmo.enable(true);
  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
    JsonArray &_devices = getStoredSwitchs();
    for (int i = 0; i < _devices.size(); i++)
    {
      JsonObject &switchJson = _devices[i];
      if (switchJson.get<String>("name").equals(String(device_name)))
      {
        stateSwitch(switchJson, state ? "ON" : "OFF");
        break;
      }
    }
  });
}

void reloadAlexaDiscoveryServices()
{
  JsonArray &_devices = getStoredSwitchs();
  for (int i = 0; i < _devices.size(); i++)
  {
    JsonObject &switchJson = _devices[i];
    String _name = switchJson.get<String>("name");
    fauxmo.removeDevice(_name.c_str());
    fauxmo.addDevice(_name.c_str());
  }
}
void reloadMqttDiscoveryServices()
{
  String ipMqtt = getConfigJson().get<String>("mqttIpDns");
  if (ipMqtt == "")
    return;
  String prefix = getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix");
  JsonArray &_devices = getStoredSwitchs();
  for (int i = 0; i < _devices.size(); i++)
  {
    JsonObject &switchJson = _devices[i];
    String _id = switchJson.get<String>("id");
    String _name = switchJson.get<String>("name");
    String type = switchJson.get<String>("type");
    if (type.equals("cover"))
    {
      publishOnMqttQueue(prefix + "/cover/" + String(ESP.getChipId()) + "/" + _id + "/config", createHaCover(switchJson), true);
      publishOnMqttQueue(prefix + "/cover/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    else if (type.equals("light"))
    {
      publishOnMqttQueue(prefix + "/light/" + String(ESP.getChipId()) + "/" + _id + "/config", createHaLight(switchJson), true);
      publishOnMqttQueue(prefix + "/light/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    else if (type.equals("switch"))
    {
      publishOnMqttQueue(prefix + "/switch/" + String(ESP.getChipId()) + "/" + _id + "/config", createHaSwitch(switchJson), true);
      publishOnMqttQueue(prefix + "/switch/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    else if (type.equals("lock"))
    {
      publishOnMqttQueue(prefix + "/lock/" + String(ESP.getChipId()) + "/" + _id + "/config", createHaLock(switchJson), true);
      publishOnMqttQueue(prefix + "/lock/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    else if (type.equals("sensor"))
    {
      publishOnMqttQueue(prefix + "/binary_sensor/" + String(ESP.getChipId()) + "/" + _id + "/config", createHaBinarySensor(switchJson), true);
      publishOnMqttQueue(prefix + "/binary_sensor/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
    }
  }

  JsonArray &_sensores = getStoredSensors();
  for (int i = 0; i < _sensores.size(); i++)
  {
    JsonObject &sensorJson = _sensores.get<JsonVariant>(i);
    String _class = sensorJson.get<String>("class");
    JsonArray &functions = sensorJson.get<JsonVariant>("functions");
    for (int i = 0; i < functions.size(); i++)
    {
      JsonObject &f = functions.get<JsonVariant>(i);
      String _id = normalize(f.get<String>("name"));
      if (_class.equals("binary_sensor"))
      {
        publishOnMqttQueue(prefix + "/binary_sensorr/" + String(ESP.getChipId()) + "/" + _id + "/config", createHaBinarySensor(sensorJson), true);
        publishOnMqttQueue(prefix + "/binary_sensor/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
      }
      else if (_class.equals("sensor"))
      {
        publishOnMqttQueue(prefix + "/sensor/" + String(ESP.getChipId()) + "/" + _id + "/config", createHaSensor(sensorJson), true);
        publishOnMqttQueue(prefix + "/sensor/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
      }
    }
  }
  logger("[MQTT] RELOAD MQTT DISCOVERY OK");
}

void loopDiscovery()
{
  fauxmo.handle();
}

String createHaSensor(JsonObject &_sensorJson)
{
  String object = "";
  JsonObject &sensorJson = getJsonObject();
  sensorJson.set("name", _sensorJson.get<String>("name"));
  sensorJson.set("state_topic", _sensorJson.get<String>("mqttStateTopic"));
  sensorJson.set("retain", _sensorJson.get<bool>("retain"));
  sensorJson.set("unit_of_measurement", _sensorJson.get<String>("unit"));
  sensorJson.set("device_class", _sensorJson.get<String>("functionClass"));
  sensorJson.printTo(object);
  return object;
}
String createHaBinarySensor(JsonObject &_sensorJson)
{
  String object = "";
  JsonObject &sensorJson = getJsonObject();
  sensorJson.set("name", _sensorJson.get<String>("name"));
  sensorJson.set("state_topic", _sensorJson.get<String>("mqttStateTopic"));
  sensorJson.set("retain", _sensorJson.get<bool>("retain"));
  sensorJson.set("payload_on", String(PAYLOAD_ON));
  sensorJson.set("payload_off", String(PAYLOAD_OFF));
  sensorJson.set("device_class", "opening");
  sensorJson.printTo(object);
  return object;
}
String createHaLock(JsonObject &_switchJson)
{
  String object = "";
  JsonObject &switchJson = getJsonObject();
  switchJson.set("name", _switchJson.get<String>("name"));
  switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
  switchJson.set("state_topic", _switchJson.get<String>("mqttStateTopic"));
  switchJson.set("retain", _switchJson.get<bool>("retain"));
  switchJson.set("payload_lock", String(PAYLOAD_LOCK));
  switchJson.set("payload_unlock", String(PAYLOAD_UNLOCK));
  switchJson.printTo(object);
  return object;
}
String createHaSwitch(JsonObject &_switchJson)
{
  String object = "";
  JsonObject &switchJson = getJsonObject();
  switchJson.set("name", _switchJson.get<String>("name"));
  switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
  switchJson.set("state_topic", _switchJson.get<String>("mqttStateTopic"));
  switchJson.set("retain", _switchJson.get<bool>("retain"));
  switchJson.set("payload_on", String(PAYLOAD_ON));
  switchJson.set("payload_off", String(PAYLOAD_OFF));
  switchJson.printTo(object);
  return object;
}
String createHaLight(JsonObject &_switchJson)
{
  String object = "";
  JsonObject &switchJson = getJsonObject();
  switchJson.set("name", _switchJson.get<String>("name"));
  switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
  switchJson.set("state_topic", _switchJson.get<String>("mqttStateTopic"));
  switchJson.set("retain", _switchJson.get<bool>("retain"));
  switchJson.set("payload_on", String(PAYLOAD_ON));
  switchJson.set("payload_off", String(PAYLOAD_OFF));
  switchJson.printTo(object);
  return object;
}
String createHaCover(JsonObject &_switchJson)
{
  String object = "";
  JsonObject &switchJson = getJsonObject();
  switchJson.set("name", _switchJson.get<String>("name"));
  switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
  switchJson.set("position_topic", _switchJson.get<String>("mqttPositionStateTopic"));
  switchJson.set("retain", _switchJson.get<bool>("retain"));
  switchJson.set("position_open", 100);
  switchJson.set("position_closed", 0);
  switchJson.set("payload_open", String(PAYLOAD_OPEN));
  switchJson.set("payload_close", String(PAYLOAD_CLOSE));
  switchJson.set("payload_stop", String(PAYLOAD_STOP));
  switchJson.printTo(object);
  return object;
}

void removeFromAlexaDiscovery(String _name)
{
  fauxmo.removeDevice(_name.c_str());
}

void removeFromHaDiscovery(String type, String _id)
{
  publishOnMqttQueue(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix") + "/" + type + "/" + String(ESP.getChipId()) + "/" + _id + "/config", "", false);
}
