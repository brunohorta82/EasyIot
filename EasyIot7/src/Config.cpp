#include "Config.h"
#define CONFIG_TAG "[CONFIG]"
//CONTROL FLAGS
bool REBOOT = false;
bool LOAD_DEFAULTS = false;
bool AUTO_UPDATE = false;
bool STORE_CONFIG = false;
bool WIFI_SCAN = false;

Config config;

struct Config& getAtualConfig(){
  return config;
}

String getUpdateUrl()
{
  return String(UPDATE_URL) + String(FACTORY_TYPE) + ".bin";
}

void requestConfigStorage()
{
  STORE_CONFIG = true;
}
void requestReboot()
{
  REBOOT = true;
}
void requestAutoUpdate()
{
  AUTO_UPDATE = true;
}
void requestLoadDefaults()
{
  LOAD_DEFAULTS = true;
}
void requestWifiScan()
{
  WIFI_SCAN = true;
}

void logger(String tag, String msg)
{
  if (msg.equals(""))
    return;

  Serial.println(tag + " " + msg);
}

String getHostname(Config &config)
{
  String nodeId = String(config.nodeId);
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
  inputStr.replace("â", "a");

  return inputStr;
}
String getApName()
{
  String nodeId = String(config.nodeId);
  if (nodeId.equals(DEFAULT_NODE_ID))
  {
    return DEFAULT_NODE_ID;
  }
  return nodeId;
}
String getConfigStatus(){
  String object = "";
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(CONFIG_FILENAME, "r+");

 if (!file) {
    Serial.println(F("Failed to read file"));
    return "";
  }
  while (file.available()) {
    object += (char)file.read();
  }
  Serial.println();
    file.close();
  }
  SPIFFS.end();
  return object;
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
    updateConfig(doc.as<JsonObject>(),error);
    
   
  }
  SPIFFS.end();
}

void saveConfiguration()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(CONFIG_FILENAME, "w+");
    if (!file)
    {
      logger(CONFIG_TAG, "Open config file Error!");
    }
    else
    {
      const size_t CAPACITY = JSON_OBJECT_SIZE(24) + 512;
      DynamicJsonDocument doc(CAPACITY);
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
      if (serializeJson(doc, file) == 0) {
        logger(CONFIG_TAG, "Failed to write Config into file");
      }else{
        logger(CONFIG_TAG, "Config stored.");
    }
    }
    file.close();
  }
  SPIFFS.end();
}
void updateConfig(JsonObject doc, bool persist){
    strlcpy(config.nodeId, doc["nodeId"] | DEFAULT_NODE_ID.c_str(), sizeof(config.nodeId));
    strlcpy(config.homeAssistantAutoDiscoveryPrefix, doc["homeAssistantAutoDiscoveryPrefix"] | "homeassistant", sizeof(config.homeAssistantAutoDiscoveryPrefix));
    strlcpy(config.mqttIpDns, doc["mqttIpDns"] | "", sizeof(config.mqttIpDns));
    config.mqttPort = doc["mqttPort"] | 1883;
    strlcpy(config.mqttUsername, doc["mqttUsername"] | "", sizeof(config.mqttUsername));
    strlcpy(config.mqttPassword, doc["mqttPassword"] | "", sizeof(config.mqttPassword));
    strlcpy(config.wifiSSID, doc["wifiSSID"] | "", sizeof(config.wifiSSID));
    strlcpy(config.wifiSSID2,doc["wifiSSID2"] | "", sizeof(config.wifiSSID2));
    strlcpy(config.wifiSecret, doc["wifiSecret"] | "", sizeof(config.wifiSecret));
    strlcpy(config.wifiSecret2, doc["wifiSecret2"] | "", sizeof(config.wifiSecret2));
    strlcpy(config.wifiIp, doc["wifiIp"] | "", sizeof(config.wifiIp));
    strlcpy(config.wifiMask, doc["wifiMask"] | "", sizeof(config.wifiMask));
    strlcpy(config.wifiGw, doc["wifiGw"] | "", sizeof(config.wifiGw));
    config.staticIp = doc["staticIp"];
    strlcpy(config.apSecret, doc["apSecret"] | AP_SECRET, sizeof(config.apSecret));
    config.configTime = doc["configTime"];
    strlcpy(config.configkey, doc["configkey"] | "", sizeof(config.configkey));
    strlcpy(config.hostname, getHostname(config).c_str(),sizeof(config.hostname));
    strlcpy(config.apName, getApName().c_str(),sizeof(config.apName));
    strlcpy(config.hardware, HARDWARE,sizeof(config.firmware));
    config.firmware = FIRMWARE_VERSION;
    Serial.println("config.wifiSSID");
    Serial.println(config.wifiSSID);
    if(persist){
      saveConfiguration();
    }
    
}