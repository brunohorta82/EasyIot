#include "Config.h"
#define CONFIG_TAG "[CONFIG]"
//CONTROL FLAGS
bool REBOOT = false;
bool LOAD_DEFAULTS = false;
bool AUTO_UPDATE = false;
bool STORE_CONFIG = false;
bool WIFI_SCAN = false;
bool WIFI_RELOAD = false;
bool restart = false;
String configRaw = "";
Config config;
struct Config &getAtualConfig()
{
  return config;
}

String getUpdateUrl()
{
  return String(UPDATE_URL) + String(FACTORY_TYPE) + ".bin";
}
boolean isValidNumber(String str)
{
  for (byte i = 0; i < str.length(); i++)
  {
    if (isDigit(str.charAt(i)))
      return true;
  }
  return false;
}
void requestReloadWifi()
{
  WIFI_RELOAD = true;
}
bool reloadWifiRequested()
{
  if (WIFI_RELOAD)
  {
    WIFI_RELOAD = false;
    return true;
  }
  return false;
}

void requestRestart()
{
  REBOOT = true;
}
bool restartRequested()
{
  if (REBOOT)
  {
    REBOOT = false;
    return true;
  }
  return false;
}

void requestAutoUpdate()
{
  AUTO_UPDATE = true;
}
bool autoUpdateRequested()
{
  if (AUTO_UPDATE)
  {
    AUTO_UPDATE = false;
    return true;
  }
  return false;
}

void requestLoadDefaults()
{
  LOAD_DEFAULTS = true;
}
bool loadDefaultsRequested()
{
  if (LOAD_DEFAULTS)
  {
    LOAD_DEFAULTS = false;
    return true;
  }
  return false;
}

void logger(String tag, String msg)
{
  if (msg.equals(""))
    return;

  Serial.println(tag + " " + msg);
}

String getHostname()
{
  return String(config.nodeId);
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
  inputStr.replace("â", "a");

  return inputStr;
}

String getConfigStatus()
{
return configRaw;
}
void loadStoredConfiguration()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(CONFIG_FILENAME, "r+");
    const size_t CAPACITY = JSON_OBJECT_SIZE(24) + 512;
    DynamicJsonDocument doc(CAPACITY);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      logger(CONFIG_TAG, "Default config loaded.");
    }
    else
    {
      logger(CONFIG_TAG, "Stored config loaded.");
    }
    file.close();
    updateConfig(doc.as<JsonObject>(), error);
  }
  SPIFFS.end();
}

void saveConfiguration()
{  const size_t CAPACITY = JSON_OBJECT_SIZE(26) + 512;
    DynamicJsonDocument doc(CAPACITY);
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(CONFIG_FILENAME, "w+");
    if (!file)
    {
      logger(CONFIG_TAG, "Open config file Error!");
    }
    else
    {
      doc["nodeId"] = config.nodeId;
      doc["homeAssistantAutoDiscoveryPrefix"] = config.homeAssistantAutoDiscoveryPrefix;
      doc["mqttIpDns"] = config.mqttIpDns;
      doc["mqttPort"] = config.mqttPort;
      doc["mqttUsername"] = config.mqttUsername;
      doc["mqttPassword"] = config.mqttPassword;
      doc["wifiSSID"] = config.wifiSSID;
      doc["wifiSSID2"] = config.wifiSSID2;
      doc["wifiSecret"] = config.wifiSecret;
      doc["wifiSecret2"] = config.wifiSecret2;
      doc["wifiIp"] = config.wifiIp;
      doc["wifiMask"] = config.wifiMask;
      doc["wifiGw"] = config.wifiGw;
      doc["staticIp"] = config.staticIp;
      doc["apSecret"] = config.apSecret;
      doc["configTime"] = config.configTime;
      doc["configkey"] = config.configkey;
      doc["hostname"] = config.hostname;
      doc["apName"] = config.apName;
      doc["firmware"] = config.firmware;
      doc["chipId"] = String(ESP.getChipId());
      doc["mac"] = WiFi.softAPmacAddress();
      if (serializeJson(doc, file) == 0)
      {
        logger(CONFIG_TAG, "Failed to write Config into file");
      }
      else
      {
        logger(CONFIG_TAG, "Config stored.");
      }
    }
    file.close();
  }
  SPIFFS.end();
  configRaw = "";
  serializeJson(doc, configRaw);
}
void updateConfig(JsonObject doc, bool persist)
{
  bool reloadWifi = config.staticIp != doc["staticIp"] || strcmp(config.wifiIp, doc["wifiIp"] | "") != 0 || strcmp(config.wifiMask, doc["wifiMask"] | "") != 0 || strcmp(config.wifiGw, doc["wifiGw"] | "") != 0 || strcmp(config.wifiSSID, doc["wifiSSID"] | "") != 0 || strcmp(config.wifiSecret, doc["wifiSecret"] | "") != 0 || strcmp(config.wifiSSID2, doc["wifiSSID2"] | "") != 0 || strcmp(config.wifiSecret2, doc["wifiSecret2"] | "") != 0;
  bool reloadMqtt =  strcmp(config.mqttIpDns, doc["mqttIpDns"] | "") != 0 || strcmp(config.mqttUsername, doc["mqttUsername"] | "") != 0 || strcmp(config.mqttPassword, doc["mqttPassword"] | "") != 0 || config.mqttPort != (doc["mqttPort"] | DEFAULT_MQTT_PORT);
  strlcpy(config.nodeId, normalize(doc["nodeId"] | String(String("MyNode")+String(ESP.getChipId()))).c_str(), sizeof(config.nodeId));
  strlcpy(config.homeAssistantAutoDiscoveryPrefix, doc["homeAssistantAutoDiscoveryPrefix"] | "homeassistant", sizeof(config.homeAssistantAutoDiscoveryPrefix));
  strlcpy(config.mqttIpDns, doc["mqttIpDns"] | "", sizeof(config.mqttIpDns));
  config.mqttPort = doc["mqttPort"] | DEFAULT_MQTT_PORT;
  strlcpy(config.mqttUsername, doc["mqttUsername"] | "", sizeof(config.mqttUsername));
  strlcpy(config.mqttPassword, doc["mqttPassword"] | "", sizeof(config.mqttPassword));
  strlcpy(config.wifiSSID, doc["wifiSSID"] | "", sizeof(config.wifiSSID));
  strlcpy(config.wifiSSID2, doc["wifiSSID2"] | "", sizeof(config.wifiSSID2));
  strlcpy(config.wifiSecret, doc["wifiSecret"] | "", sizeof(config.wifiSecret));
  strlcpy(config.wifiSecret2, doc["wifiSecret2"] | "", sizeof(config.wifiSecret2));
  strlcpy(config.wifiIp, doc["wifiIp"] | "", sizeof(config.wifiIp));
  strlcpy(config.wifiMask, doc["wifiMask"] | "", sizeof(config.wifiMask));
  strlcpy(config.wifiGw, doc["wifiGw"] | "", sizeof(config.wifiGw));
  config.staticIp = doc["staticIp"];
  strlcpy(config.apSecret, doc["apSecret"] | AP_SECRET, sizeof(config.apSecret));
  config.configTime = doc["configTime"];
  strlcpy(config.configkey, doc["configkey"] | "", sizeof(config.configkey));
  strlcpy(config.hostname, getHostname().c_str(), sizeof(config.hostname));
  strlcpy(config.hardware, HARDWARE, sizeof(config.firmware));
  config.firmware = doc["firmware"] | FIRMWARE_VERSION;
  if (persist || config.firmware  !=  FIRMWARE_VERSION)
  {
     config.firmware = FIRMWARE_VERSION;
    saveConfiguration();
  }
  if (reloadWifi)
  {
    requestReloadWifi();
  }
  if (reloadMqtt)
  {
    setupMQTT();
  }
  configRaw = "";
  serializeJson(doc, configRaw);
}