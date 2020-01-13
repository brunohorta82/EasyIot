#include "Config.h"
#include "constants.h"
#include "WiFi.h"
#include "ESP8266WiFi.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "WebServer.h"
#include "Switches.h"
#include "Sensors.h"
const char *NTP_SERVER = "pt.pool.ntp.org";
const char *TZ_INFO = "WET-0WEST-1,M3.5.0/01:00:00,M10.5.0/02:00:00";
tm timeinfo;
time_t now;
unsigned long lastNTPtime = 0ul;
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
long getTime()
{
  if (WiFi.status() != WL_CONNECTED )
  {
    return 0;
  }
  time_t now = time(nullptr);
  return now;
}
void loopTime()
{
  if (WiFi.status() == WL_CONNECTED && lastNTPtime + 1000 < millis())
  {
    long time = getTime();
    if (getAtualConfig().connectedOn == 0ul && time > 1576082395)
    {
      getAtualConfig().connectedOn = time;
      refreshMDNS(getAtualConfig().nodeId);
    }
    lastNTPtime = millis();
  }
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
  inputStr.replace(",", "");
  inputStr.replace("\"", "");
  inputStr.replace(" ", "");
  inputStr.replace("â", "a");
}

size_t Config::serializeToJson(Print &output)
{
  const size_t CAPACITY = JSON_OBJECT_SIZE(39) + sizeof(Config);
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
  doc["firmware"] = VERSION;
  doc["chipId"] = chipId;
  doc["mac"] = WiFi.softAPmacAddress();
  doc["apiUser"] = apiUser;
  doc["apiPassword"] = apiPassword;
  doc["emoncmsServer"] = emoncmsServer;
  doc["emoncmsPath"] = emoncmsPath;
  doc["emoncmsApikey"] = emoncmsApikey;
  doc["knxArea"] = knxArea;
  doc["knxLine"] = knxLine;
  doc["knxMember"] = knxMember;
  doc["currentWifiIp"] = WiFi.localIP().toString();
  doc["wifiStatus"] = WiFi.isConnected();
  doc["signal"] = WiFi.RSSI();
  doc["mode"] = (int)WiFi.getMode();
  doc["mqttConnected"] = mqttConnected();
  doc["freeHeap"] = String(ESP.getFreeHeap());
  doc["connectedOn"] = connectedOn;
  doc["firmwareMode"] = constantsConfig::firmwareMode;

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
  file.write((uint8_t *)&knxArea, sizeof(knxArea));
  file.write((uint8_t *)&knxLine, sizeof(knxLine));
  file.write((uint8_t *)&knxMember, sizeof(knxMember));
}
void Config::load(File &file)
{
  file.read((uint8_t *)&firmware, sizeof(firmware));
  if (firmware < 7.8)
  {
    requestLoadDefaults();
  }
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
  file.read((uint8_t *)&knxArea, sizeof(knxArea));
  file.read((uint8_t *)&knxLine, sizeof(knxLine));
  file.read((uint8_t *)&knxMember, sizeof(knxMember));
  if (firmware < VERSION)
  {
#ifdef DEBUG
    Log.notice("%s Migrate Firmware from %F to %F" CR, tags::config, firmware, VERSION);
#endif
    firmware = VERSION;
  }
}
void loadStoredConfiguration(Config &config)
{
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_INFO, 1);
  if (!SPIFFS.begin())
  {
#ifdef DEBUG
    Log.error("%s File storage can't start" CR, tags::config);
#endif
    return;
  }

  if (!SPIFFS.exists(configFilenames::config))
  {
#ifdef DEBUG
    Log.notice("%s Default config loaded." CR, tags::config);
#endif
    strlcpy(config.nodeId, String(ESP.getChipId()).c_str(), sizeof(config.nodeId));
    strlcpy(config.chipId, config.nodeId, sizeof(config.chipId));
    config.mqttPort = constantsMqtt::defaultPort;
    config.staticIp = false;
    strlcpy(config.apSecret, constantsConfig::apSecret, sizeof(config.apSecret));
    strlcpy(config.apiUser, constantsConfig::apiUser, sizeof(config.apiUser));
    strlcpy(config.apiPassword, constantsConfig::apiPassword, sizeof(config.apiPassword));

#ifdef WIFI_SSID
    strlcpy(config.wifiSSID, WIFI_SSID, sizeof(config.wifiSSID));
#endif
#ifdef WIFI_SECRET
    strlcpy(config.wifiSecret, WIFI_SECRET, sizeof(config.wifiSecret));
#endif

    config.knxArea = 1;
    config.knxLine = 1;
    config.knxMember = 1;
    config.firmware = VERSION;
    strlcpy(config.homeAssistantAutoDiscoveryPrefix, constantsMqtt::homeAssistantAutoDiscoveryPrefix, sizeof(config.homeAssistantAutoDiscoveryPrefix));
    SPIFFS.end();
#ifdef DEBUG
    Log.notice("%s Config %s loaded." CR, tags::config, String(config.firmware).c_str());
#endif
  }

  File file = SPIFFS.open(configFilenames::config, "r+");
  config.load(file);
  file.close();
  SPIFFS.end();
#ifdef DEBUG
  Log.notice("%s Stored config loaded." CR, tags::config);
#endif
}
Config &Config::saveConfigurationOnDefaultFile()
{
  if (!SPIFFS.begin())
  {
#ifdef DEBUG
    Log.error("%s File storage can't start" CR, tags::config);
#endif
    return getAtualConfig();
  }

  File file = SPIFFS.open(configFilenames::config, "w+");
  save(file);
  file.close();
  SPIFFS.end();
#ifdef DEBUG
  Log.notice("%s Config stored." CR, tags::config);
#endif
  return *this;
}

Config &Config::updateFromJson(JsonObject doc)
{
  char lastNodeId[32];
  strlcpy(lastNodeId, getAtualConfig().nodeId, sizeof(lastNodeId));
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
  knxArea = static_cast<uint8_t>(doc["knxArea"] | 0);
  knxLine = static_cast<uint8_t>(doc["knxLine"] | 0);
  knxMember = static_cast<uint8_t>(doc["knxMember"] | 0);
  emoncmsServerStr.replace("https", "http");
  knx.physical_address_set(knx.PA_to_address(getAtualConfig().knxArea, getAtualConfig().knxLine, getAtualConfig().knxMember));
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
     reloadSwitches();
     reloadSensors();
  }
  refreshMDNS(lastNodeId);
  return *this;
}