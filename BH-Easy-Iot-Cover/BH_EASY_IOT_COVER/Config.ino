AsyncEventSource events("/events");

JsonObject &configJson = getJsonObject();
typedef struct
{
  int gpio;
} gpios_t;
std::vector<gpios_t> inUseGpios;
void logger(String payload)
{
  if (payload.equals(""))
    return;
  Serial.println(payload);
}

void resetToFactoryConfig()
{
  SPIFFS.format();
  shouldReboot = true;
}

JsonObject &getConfigJson()
{
  return configJson;
}

String getUpdateUrl()
{
  return "http://release.bhonofre.pt/release_" + String(FACTORY_TYPE) + ".bin";
}
String getHostname()
{
  String nodeId = configJson.get<String>("nodeId");
  if (nodeId.equals(DEFAULT_NODE_ID))
  {
    return DEFAULT_NODE_ID;
  }
  return nodeId;
}

String normalize(String inputStr)
{
  inputStr.toLowerCase();
  inputStr.trim();
  inputStr.replace("_", "");
  inputStr.replace(".", "");
  inputStr.replace("/", "");
  inputStr.replace("\\", "");
  inputStr.replace("º", "");
  inputStr.replace("ª", "");
  inputStr.replace("ç", "c");
  inputStr.replace("á", "a");
  inputStr.replace("à", "a");
  inputStr.replace("é", "e");
  inputStr.replace("&", "");
  inputStr.replace("%", "");
  inputStr.replace("$", "");
  inputStr.replace("#", "");
  inputStr.replace("!", "");
  inputStr.replace("+", "");
  inputStr.replace("-", "");
  inputStr.replace(",", "");
  inputStr.replace("\"", "");
  inputStr.replace(" ", "");
  return inputStr;
}
String getApName()
{
  String nodeId = configJson.get<String>("nodeId");
  if (nodeId.equals(DEFAULT_NODE_ID))
  {
    return DEFAULT_NODE_ID;
  }
  return nodeId;
}

void loadStoredConfiguration()
{
  bool configFail = true;
  if (SPIFFS.begin())
  {
    File cFile;
    if (SPIFFS.exists(CONFIG_FILENAME))
    {
      cFile = SPIFFS.open(CONFIG_FILENAME, "r+");
      if (cFile)
      {
        logger("[CONFIG] Read stored file config...");
        JsonObject &storedConfig = getJsonObject(cFile);
        if (storedConfig.success())
        {
          configJson.set("firmware", FIRMWARE_VERSION);
          configJson.set("nodeId", storedConfig.get<String>("nodeId"));
          configJson.set("hostname", storedConfig.get<String>("hostname"));
          configJson.set("hardware", HARDWARE);
          configJson.set("hardwareId", String(ESP.getChipId()));
          configJson.set("type", String(FACTORY_TYPE));

          configJson.set("homeAssistantAutoDiscoveryPrefix", storedConfig.get<String>("homeAssistantAutoDiscoveryPrefix"));

          configJson.set("mqttIpDns", storedConfig.get<String>("mqttIpDns"));
          configJson.set("mqttUsername", storedConfig.get<String>("mqttUsername"));
          configJson.set("mqttPort", storedConfig.get<unsigned int>("mqttPort"));
          configJson.set("mqttPassword", storedConfig.get<String>("mqttPassword"));

          configJson.set("wifiSSID", storedConfig.get<String>("wifiSSID"));
          configJson.set("wifiSecret", storedConfig.get<String>("wifiSecret"));

          configJson.set("wifiSSID2", storedConfig.get<String>("wifiSSID2"));
          configJson.set("wifiSecret2", storedConfig.get<String>("wifiSecret2"));

          configJson.set("staticIp", storedConfig.get<bool>("staticIp"));
          configJson.set("wifiIp", storedConfig.get<String>("wifiIp"));
          configJson.set("wifiMask", storedConfig.get<String>("wifiMask"));
          configJson.set("wifiGw", storedConfig.get<String>("wifiGw"));

          configJson.set("apSecret", storedConfig.get<String>("apSecret"));

          configJson.set("configTime", storedConfig.get<long>("configTime"));
          configJson.set("configkey", storedConfig.get<String>("configkey"));

          logger("[CONFIG] Apply stored file config with success...");
          cFile.close();
          configFail = false;
        }
      }
    }

    if (configFail)
    {
      logger("[CONFIG] Apply default config...");
      cFile = SPIFFS.open(CONFIG_FILENAME, "w+");
      configJson.set("nodeId", DEFAULT_NODE_ID);
      configJson.set("homeAssistantAutoDiscoveryPrefix", "homeassistant");
      configJson.set("homeAssistantAutoDiscoveryPrefixOld", "homeassistant");
      configJson.set("hostname", getHostname());
      configJson.set("mqttPort", 1883);
      configJson.set("type", String(FACTORY_TYPE));
      configJson.set("configTime", 0L);
      configJson.set("apSecret", AP_SECRET);
      configJson.set("hardware", HARDWARE);
      configJson.set("configTime", 0L);
      configJson.set("firmware", FIRMWARE_VERSION);
      configJson.printTo(cFile);
    }
    SPIFFS.end();
  }
  else
  {
    logger("[CONFIG] File system error...");
  }
}

JsonObject &saveNode(JsonObject &nodeConfig)
{
  String nodeId = nodeConfig.get<String>("nodeId");

  if (nodeId != nullptr && !configJson.get<String>("nodeId").equals(nodeId))
  {
    nodeId.replace(" ", "");
    String oldNodeId = configJson.get<String>("nodeId");
    configJson.set("nodeId", nodeId);
    reloadMqttConfig();
  }
  saveConfig();
  return configJson;
}

JsonObject &saveWifi(JsonObject &_config)
{
  configJson.set("wifiSSID", _config.get<String>("wifiSSID"));
  configJson.set("wifiSecret", _config.get<String>("wifiSecret"));
  configJson.set("wifiSSID2", _config.get<String>("wifiSSID2"));
  configJson.set("wifiSecret2", _config.get<String>("wifiSecret2"));
  configJson.set("wifiIp", _config.get<String>("wifiIp"));
  configJson.set("wifiMask", _config.get<String>("wifiMask"));
  configJson.set("wifiGw", _config.get<String>("wifiGw"));
  configJson.set("staticIp", _config.get<bool>("staticIp"));
  wifiUpdated = true;
  return configJson;
}

JsonObject &adoptControllerConfig(JsonObject &_config, String configkey)
{
  configJson.set("wifiSSID", _config.get<String>("wifiSSID"));
  configJson.set("wifiSecret", _config.get<String>("wifiSecret"));

  configJson.set("wifiSSID2", _config.get<String>("wifiSSID2"));
  configJson.set("wifiSecret2", _config.get<String>("wifiSecret2"));

  configJson.set("mqttIpDns", _config.get<String>("mqttIpDns"));
  configJson.set("mqttUsername", _config.get<String>("mqttUsername"));
  configJson.set("mqttPassword", _config.get<String>("mqttPassword"));

  configJson.set("configTime", _config.get<long>("configTime"));
  configJson.set("configkey", configkey);

  configJson.set("homeAssistantAutoDiscoveryPrefix", _config.get<String>("homeAssistantAutoDiscoveryPrefix"));

  reloadMqttConfig();
  saveConfig();
  return configJson;
}

void updateNetworkConfig()
{
  if (!configJson.get<bool>("staticIp"))
  {
    configJson.set("wifiIp", WiFi.localIP().toString());
    configJson.set("wifiMask", WiFi.subnetMask().toString());
    configJson.set("wifiGw", WiFi.gatewayIP().toString());
  }
  saveConfig();
}

JsonObject &saveMqtt(JsonObject &_config)
{
  configJson.set("mqttIpDns", _config.get<String>("mqttIpDns"));
  configJson.set("mqttUsername", _config.get<String>("mqttUsername"));
  configJson.set("mqttPassword", _config.get<String>("mqttPassword"));
  configJson.set("mqttEmbedded", _config.get<String>("mqttEmbedded"));
  reloadMqttConfig();
  saveConfig();
  return configJson;
}

void saveConfig()
{
  rebuildAllMqttTopics();
  logger("[MQTT] REBUILD TOPICS OK");
  if (SPIFFS.begin())
  {
    File rFile = SPIFFS.open(CONFIG_FILENAME, "w+");
    if (!rFile)
    {
      logger("[CONFIG] Open config file Error!");
    }
    else
    {

      configJson.printTo(rFile);
    }
    rFile.close();
  }
  else
  {
    logger("[CONFIG] Open file system Error!");
  }
  SPIFFS.end();
  logger("[CONFIG] New config stored.");
}

void configGpio(int gpio, int mode)
{
  pinMode(gpio, mode);
  inUseGpios.push_back({gpio});
}
