#include "Config.h"
#include "constants.h"
#include "WiFi.h"
#include "ESP8266WiFi.h"
#include "Mqtt.h"

//CONTROL FLAGS
static bool g_reboot = false;
static bool g_loadDefaults = false;
static bool g_autoUpdate = false;
static bool g_wifiReload = false;

struct Config &getAtualConfig()
{
  static Config config;
  return config;
}
void generateId(String &id, const String &name, size_t maxSize)
{
  id.reserve(maxSize);
  id.concat(getAtualConfig().chipId);
  id.concat(name);
  normalize(id);
}
boolean isValidNumber(const char *str)
{
  size_t length = strlen(str);
  for (byte i = 0; i < length; i++)
  {
    if (isDigit(str[i]))
      return true;
  }
  return false;
}

void requestReloadWifi()
{
  g_wifiReload = true;
}
bool reloadWifiRequested()
{
  if (g_wifiReload)
  {
    g_wifiReload = false;
    return true;
  }
  return false;
}

void requestRestart()
{
  g_reboot = true;
}
bool restartRequested()
{
  if (g_reboot)
  {
    g_reboot = false;
    return true;
  }
  return false;
}

void requestAutoUpdate()
{
  g_autoUpdate = true;
}
bool autoUpdateRequested()
{
  if (g_autoUpdate)
  {
    g_autoUpdate = false;
    return true;
  }
  return false;
}

void configPIN(uint8_t pin, uint8_t mode)
{
  if (pin == constantsConfig::noGPIO)
  {
    return;
  }
  pinMode(pin, mode);
}

void writeToPIN(uint8_t pin, uint8_t val)
{
  if (pin == constantsConfig::noGPIO)
  {
    return;
  }
  digitalWrite(pin, val);
}

bool readPIN(uint8_t pin)
{
  if (pin == constantsConfig::noGPIO)
  {
    return true;
  }
  return digitalRead(pin);
}
void requestLoadDefaults()
{
  g_loadDefaults = true;
}
bool loadDefaultsRequested()
{
  if (g_loadDefaults)
  {
    g_loadDefaults = false;
    return true;
  }
  return false;
}

void normalize(String &inputStr)
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
}

size_t Config::serializeToJson(Print &output)
{
  const size_t CAPACITY = JSON_OBJECT_SIZE(24) + sizeof(Config);
  StaticJsonDocument<CAPACITY> doc;
  doc["nodeId"] = nodeId;
  doc["homeAssistantAutoDiscoveryPrefix"] = homeAssistantAutoDiscoveryPrefix;
  doc["mqttIpDns"] = mqttIpDns;
  doc["mqttPort"] = mqttPort;
  doc["mqttUsername"] = mqttUsername;
  doc["mqttPassword"] = mqttPassword;
  doc["mqttAvailableTopic"] = mqttAvailableTopic;
  doc["wifiSSID"] = wifiSSID;
  doc["wifiSSID2"] = wifiSSID2;
  doc["wifiSecret"] = wifiSecret;
  doc["wifiSecret2"] = wifiSecret2;
  doc["wifiIp"] = wifiIp;
  doc["wifiMask"] = wifiMask;
  doc["wifiGw"] = wifiGw;
  doc["staticIp"] = staticIp;
  doc["apSecret"] = apSecret;
  doc["configTime"] = configTime;
  doc["configkey"] = configkey;
  doc["apName"] = apName;
  doc["firmware"] = firmware;
  doc["chipId"] = chipId;
  doc["mac"] = WiFi.softAPmacAddress();
  doc["apiUser"] = apiUser;
  doc["apiPassword"] = apiPassword;
  doc["emoncmsServer"] = emoncmsServer;
  doc["emoncmsPath"] = emoncmsPath;
  doc["emoncmsApikey"] = emoncmsApikey;
  return serializeJson(doc, output);
}

void Config::save(File &file) const
{
  file.write((uint8_t *)&firmware, sizeof(firmware));
  file.write((uint8_t *)nodeId, sizeof(nodeId));
  file.write((uint8_t *)homeAssistantAutoDiscoveryPrefix, sizeof(homeAssistantAutoDiscoveryPrefix));
  file.write((uint8_t *)mqttIpDns, sizeof(mqttIpDns));
  file.write((uint8_t *)mqttUsername, sizeof(mqttUsername));
  file.write((uint8_t *)mqttAvailableTopic, sizeof(mqttAvailableTopic));
  file.write((uint8_t *)&mqttPort, sizeof(mqttPort));
  file.write((uint8_t *)mqttPassword, sizeof(mqttPassword));
  file.write((uint8_t *)wifiSSID, sizeof(wifiSSID));
  file.write((uint8_t *)wifiSecret, sizeof(wifiSecret));
  file.write((uint8_t *)wifiSSID2, sizeof(wifiSSID2));
  file.write((uint8_t *)wifiSecret2, sizeof(wifiSecret2));
  file.write((uint8_t *)&staticIp, sizeof(staticIp));
  file.write((uint8_t *)wifiIp, sizeof(wifiIp));
  file.write((uint8_t *)wifiMask, sizeof(wifiMask));
  file.write((uint8_t *)wifiGw, sizeof(wifiGw));
  file.write((uint8_t *)apSecret, sizeof(apSecret));
  file.write((uint8_t *)apName, sizeof(apName));
  file.write((uint8_t *)&configTime, sizeof(configTime));
  file.write((uint8_t *)configkey, sizeof(configkey));
  file.write((uint8_t *)chipId, sizeof(chipId));
  file.write((uint8_t *)apiUser, sizeof(apiUser));
  file.write((uint8_t *)apiPassword, sizeof(apiPassword));
  file.write((uint8_t *)emoncmsServer, sizeof(emoncmsServer));
  file.write((uint8_t *)emoncmsPath, sizeof(emoncmsPath));
  file.write((uint8_t *)emoncmsApikey, sizeof(emoncmsApikey));
}
void Config::load(File &file)
{
  file.read((uint8_t *)&firmware, sizeof(firmware));
  file.read((uint8_t *)nodeId, sizeof(nodeId));
  file.read((uint8_t *)homeAssistantAutoDiscoveryPrefix, sizeof(homeAssistantAutoDiscoveryPrefix));
  file.read((uint8_t *)mqttIpDns, sizeof(mqttIpDns));
  file.read((uint8_t *)mqttUsername, sizeof(mqttUsername));
  file.read((uint8_t *)mqttAvailableTopic, sizeof(mqttAvailableTopic));
  file.read((uint8_t *)&mqttPort, sizeof(mqttPort));
  file.read((uint8_t *)mqttPassword, sizeof(mqttPassword));
  file.read((uint8_t *)wifiSSID, sizeof(wifiSSID));
  file.read((uint8_t *)wifiSecret, sizeof(wifiSecret));
  file.read((uint8_t *)wifiSSID2, sizeof(wifiSSID2));
  file.read((uint8_t *)wifiSecret2, sizeof(wifiSecret2));
  file.read((uint8_t *)&staticIp, sizeof(staticIp));
  file.read((uint8_t *)wifiIp, sizeof(wifiIp));
  file.read((uint8_t *)wifiMask, sizeof(wifiMask));
  file.read((uint8_t *)wifiGw, sizeof(wifiGw));
  file.read((uint8_t *)apSecret, sizeof(apSecret));
  file.read((uint8_t *)apName, sizeof(apName));
  file.read((uint8_t *)&configTime, sizeof(configTime));
  file.read((uint8_t *)configkey, sizeof(configkey));
  file.read((uint8_t *)chipId, sizeof(chipId));
  file.read((uint8_t *)apiUser, sizeof(apiUser));
  file.read((uint8_t *)apiPassword, sizeof(apiPassword));
  file.read((uint8_t *)emoncmsServer, sizeof(emoncmsServer));
  file.read((uint8_t *)emoncmsPath, sizeof(emoncmsPath));
  file.read((uint8_t *)emoncmsApikey, sizeof(emoncmsApikey));
  if (firmware < VERSION)
  {
    Log.notice("%s Migrate Firmware from %F to %F" CR, tags::config, firmware, VERSION);
  }
}
void loadStoredConfiguration(Config &config)
{
  if (!SPIFFS.begin())
  {
    Log.error("%s File storage can't start" CR, tags::config);
    return;
  }

  if (!SPIFFS.exists(configFilenames::config))
  {
    Log.notice("%s Default config loaded." CR, tags::config);
    strlcpy(config.nodeId, String(ESP.getChipId()).c_str(), sizeof(config.nodeId));
    strlcpy(config.chipId, config.nodeId, sizeof(config.chipId));
    config.mqttPort = constantsMqtt::defaultPort;
    config.staticIp = false;
    strlcpy(config.apSecret, constantsConfig::apSecret, sizeof(config.apSecret));
    strlcpy(config.apiUser, constantsConfig::apiUser, sizeof(config.apiUser));
    strlcpy(config.apiPassword, constantsConfig::apiPassword, sizeof(config.apiPassword));
    strlcpy(config.wifiSSID, "IOTBH", sizeof(config.wifiSSID));
    strlcpy(config.wifiSecret, "IOT2017@", sizeof(config.wifiSecret));
    config.firmware = VERSION;
    strlcpy(config.homeAssistantAutoDiscoveryPrefix, constantsMqtt::homeAssistantAutoDiscoveryPrefix, sizeof(config.homeAssistantAutoDiscoveryPrefix));
    SPIFFS.end();
    return;
  }

  File file = SPIFFS.open(configFilenames::config, "r+");
  config.load(file);
  file.close();
  SPIFFS.end();
  Log.notice("%s Stored config loaded." CR, tags::config);
}
Config &Config::saveConfigurationOnDefaultFile()
{
  if (!SPIFFS.begin())
  {
    Log.error("%s File storage can't start" CR, tags::config);
    return getAtualConfig();
  }

  File file = SPIFFS.open(configFilenames::config, "w+");
  save(file);
  file.close();
  SPIFFS.end();
  Log.notice("%s Config stored." CR, tags::config);
  return *this;
}

Config &Config::updateFromJson(JsonObject doc)
{
  bool reloadWifi = staticIp != doc["staticIp"] || strcmp(wifiIp, doc["wifiIp"] | "") != 0 || strcmp(wifiMask, doc["wifiMask"] | "") != 0 || strcmp(wifiGw, doc["wifiGw"] | "") != 0 || strcmp(wifiSSID, doc["wifiSSID"] | "") != 0 || strcmp(wifiSecret, doc["wifiSecret"] | "") != 0 || strcmp(wifiSSID2, doc["wifiSSID2"] | "") != 0 || strcmp(wifiSecret2, doc["wifiSecret2"] | "") != 0;
  bool reloadMqtt = strcmp(mqttIpDns, doc["mqttIpDns"] | "") != 0 || strcmp(mqttUsername, doc["mqttUsername"] | "") != 0 || strcmp(mqttPassword, doc["mqttPassword"] | "") != 0 || mqttPort != (doc["mqttPort"] | constantsMqtt::defaultPort);
  String n_name = doc["nodeId"] | String(ESP.getChipId());
  normalize(n_name);
  strlcpy(nodeId, n_name.c_str(), sizeof(nodeId));
  strlcpy(mqttIpDns, doc["mqttIpDns"] | "", sizeof(mqttIpDns));
  mqttPort = doc["mqttPort"] | constantsMqtt::defaultPort;
  strlcpy(mqttUsername, doc["mqttUsername"] | "", sizeof(mqttUsername));
  strlcpy(mqttPassword, doc["mqttPassword"] | "", sizeof(mqttPassword));
  strlcpy(wifiSSID, doc["wifiSSID"] | "", sizeof(wifiSSID));
  strlcpy(wifiSSID2, doc["wifiSSID2"] | "", sizeof(wifiSSID2));
  strlcpy(wifiSecret, doc["wifiSecret"] | "", sizeof(wifiSecret));
  strlcpy(wifiSecret2, doc["wifiSecret2"] | "", sizeof(wifiSecret2));
  String emoncmsServerStr = doc["emoncmsServer"] | "";
  emoncmsServerStr.replace("https", "http");

  while (emoncmsServerStr.endsWith("/"))
  {

    emoncmsServerStr.remove(emoncmsServerStr.lastIndexOf("/"));
  }
  strlcpy(emoncmsServer, emoncmsServerStr.c_str(), sizeof(emoncmsServer));
  String emoncmsPathStr = doc["emoncmsPath"] | "";
  while (emoncmsPathStr.endsWith("/"))
  {
    emoncmsPathStr.remove(emoncmsPathStr.lastIndexOf("/"));
  }
  strlcpy(emoncmsPath, emoncmsPathStr.c_str(), sizeof(emoncmsPath));

  strlcpy(emoncmsApikey, doc["emoncmsApikey"] | "", sizeof(emoncmsApikey));
  strlcpy(wifiIp, doc["wifiIp"] | "", sizeof(wifiIp));
  strlcpy(wifiMask, doc["wifiMask"] | "", sizeof(wifiMask));
  strlcpy(wifiGw, doc["wifiGw"] | "", sizeof(wifiGw));
  staticIp = doc["staticIp"];
  strlcpy(apSecret, doc["apSecret"] | constantsConfig::apSecret, sizeof(apSecret));
  configTime = doc["configTime"];
  strlcpy(configkey, doc["configkey"] | "", sizeof(configkey));
  firmware = doc["firmware"] | VERSION;
  strlcpy(mqttAvailableTopic, getAvailableTopic().c_str(), sizeof(mqttAvailableTopic));
  if (strcmp(constantsMqtt::mqttCloudURL, mqttIpDns) == 0)
  {
    strlcpy(homeAssistantAutoDiscoveryPrefix, mqttUsername, sizeof(homeAssistantAutoDiscoveryPrefix));
  }
  else
  {
    strlcpy(homeAssistantAutoDiscoveryPrefix, "homeassistant", sizeof(homeAssistantAutoDiscoveryPrefix));
  }

  if (reloadWifi)
  {
    requestReloadWifi();
  }
  if (reloadMqtt)
  {
    setupMQTT();
  }
  return *this;
}