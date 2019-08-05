#include "Config.h"
#include "constants.h"

//CONTROL FLAGS
static bool g_reboot = false;
static bool g_loadDefaults = false;
static bool g_autoUpdate = false;
static bool g_wifiReload = false;

static Config config;

struct Config &getAtualConfig()
{
  return config;
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


size_t serializeConfigStatus(Print &output)
{
  const size_t CAPACITY = JSON_OBJECT_SIZE(26) + 512;
  DynamicJsonDocument doc(CAPACITY);
  doc["nodeId"] = String(config.nodeId);
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
  doc["apName"] = config.apName;
  doc["firmware"] = config.firmware;
  doc["chipId"] = String(ESP.getChipId());
  doc["mac"] = WiFi.softAPmacAddress();
  return serializeJson(doc, output);
}

void Config::save(File& file) const
{
  file.write((uint8_t*)this, sizeof(Config)); 
}
void Config::load(File& file)
{
  file.read((uint8_t*)this, sizeof(Config)); 
}
void loadStoredConfiguration()
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
    config.mqttPort = constantsMqtt::defaultPort;
    config.staticIp = false;
    strlcpy(config.apSecret, constantsConfig::apSecret, sizeof(config.apSecret));
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
void saveConfiguration()
{
  if (!SPIFFS.begin())
  {
    Log.error("%s File storage can't start" CR, tags::config);
    return;
  }

  File file = SPIFFS.open(configFilenames::config, "w+");
  config.save(file);
  file.close();
  SPIFFS.end();
  Log.notice("%s Config stored." CR, tags::config);
}

void Config::update(JsonObject doc, bool persist)
{

  bool reloadWifi = staticIp != doc["staticIp"] || strcmp(wifiIp, doc["wifiIp"] | "") != 0 || strcmp(wifiMask, doc["wifiMask"] | "") != 0 || strcmp(wifiGw, doc["wifiGw"] | "") != 0 || strcmp(wifiSSID, doc["wifiSSID"] | "") != 0 || strcmp(wifiSecret, doc["wifiSecret"] | "") != 0 || strcmp(wifiSSID2, doc["wifiSSID2"] | "") != 0 || strcmp(wifiSecret2, doc["wifiSecret2"] | "") != 0;
  bool reloadMqtt = strcmp(mqttIpDns, doc["mqttIpDns"] | "") != 0 || strcmp(mqttUsername, doc["mqttUsername"] | "") != 0 || strcmp(mqttPassword, doc["mqttPassword"] | "") != 0 || mqttPort != (doc["mqttPort"] | constantsMqtt::defaultPort);
  String n_name = doc["nodeId"] | String(String("MyNode") + String(ESP.getChipId()));
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
  strlcpy(wifiIp, doc["wifiIp"] | "", sizeof(wifiIp));
  strlcpy(wifiMask, doc["wifiMask"] | "", sizeof(wifiMask));
  strlcpy(wifiGw, doc["wifiGw"] | "", sizeof(wifiGw));
  staticIp = doc["staticIp"];
  strlcpy(apSecret, doc["apSecret"] | constantsConfig::apSecret, sizeof(apSecret));
  configTime = doc["configTime"];
  strlcpy(configkey, doc["configkey"] | "", sizeof(configkey));
  firmware = doc["firmware"] | VERSION;
  if (strcmp(constantsMqtt::mqttCloudURL, mqttIpDns) == 0)
  {
    strlcpy(homeAssistantAutoDiscoveryPrefix, mqttUsername, sizeof(homeAssistantAutoDiscoveryPrefix));
  }
  else
  {
    strlcpy(homeAssistantAutoDiscoveryPrefix, "homeassistant", sizeof(homeAssistantAutoDiscoveryPrefix));
  }

  if (persist || firmware != VERSION)
  {
    firmware = VERSION;
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
}