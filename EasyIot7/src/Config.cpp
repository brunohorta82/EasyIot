#include "Config.h"
#include "constants.h"
#include "CoreWiFi.h"
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
// CONTROL FLAGS
static bool g_reboot = false;
static bool g_loadDefaults = false;
static bool g_autoUpdate = false;
static bool g_wifiReload = false;
static bool g_cloudIOSync = false;
static bool g_wifiScan = false;
struct Config &
getAtualConfig()
{
  static Config config;
  return config;
}
void generateId(String &id, const String &name, int familyCode, size_t maxSize)
{
  id.reserve(maxSize);
  id.concat(getAtualConfig().chipId);
  id.concat(name);
  id.concat(familyCode);
  normalize(id);
}
long getTime()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return 0;
  }
  time_t now = time(nullptr);
  return now;
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
void requestWifiScan()
{
  g_wifiScan = true;
}
bool wifiScanRequested()
{
  if (g_wifiScan)
  {
    g_wifiScan = false;
    return true;
  }
  return false;
}
void requestCloudIOSync()
{
  g_cloudIOSync = true;
}
bool cloudIOSync()
{
  if (g_cloudIOSync)
  {
    g_cloudIOSync = false;
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
#ifdef ESP8266
  if (pin == 16)
  {
    if (mode == INPUT_PULLUP)
    {
      mode = INPUT_PULLDOWN_16;
    }
  }
#endif
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

void Config::save()
{
  if (!LittleFS.begin())
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s File storage can't start" CR, tags::config);
#endif
    return;
  }
  File file = LittleFS.open(configFilenames::config, "w+");
  // this->save(file);
  file.close();

#ifdef DEBUG_ONOFRE
  Log.notice("%s Config stored." CR, tags::config);
#endif
}
void Config::toJson(JsonVariant &root)
{
  root["nodeId"] = nodeId;
  root["mqttIpDns"] = mqttIpDns;
  root["mqttPort"] = mqttPort;
  root["mqttUsername"] = mqttUsername;
  root["mqttPassword"] = mqttPassword;
  root["mqttConnected"] = mqttConnected();
  root["wifiSSID"] = wifiSSID;
  root["wifiSecret"] = wifiSecret;
  root["wifiIp"] = WiFi.localIP().toString();
  root["wifiMask"] = wifiMask;
  root["wifiGw"] = wifiGw;
  root["staticIp"] = staticIp;
  root["apName"] = apName;
  root["firmware"] = VERSION;
  root["chipId"] = chipId;
  root["mac"] = WiFi.macAddress();
  root["wifiStatus"] = WiFi.isConnected();
  root["signal"] = WiFi.RSSI();
  JsonVariant pins = root.createNestedArray("pins");
#ifdef ESP8266
  std::vector<int> pinsRef = {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16};
#endif
#ifdef ESP32
  std::vector<int> pinsRef = {4, 5, 7, 8, 13, 14, 19, 20, 21, 22, 25, 26, 27, 32, 33, 34, 35, 36, 37, 38};
#endif
  for (auto p : pinsRef)
  {
    pins.add(p);
  }
}

void Config::load(File &file)
{
  file.read((uint8_t *)&firmware, sizeof(firmware));
  if (firmware < 8.1)
  {
    requestLoadDefaults();
  }
  file.read((uint8_t *)nodeId, sizeof(nodeId));
  file.read((uint8_t *)mqttIpDns, sizeof(mqttIpDns));
  file.read((uint8_t *)mqttUsername, sizeof(mqttUsername));
  file.read((uint8_t *)mqttAvailableTopic, sizeof(mqttAvailableTopic));
  file.read((uint8_t *)&mqttPort, sizeof(mqttPort));
  file.read((uint8_t *)mqttPassword, sizeof(mqttPassword));
  file.read((uint8_t *)wifiSSID, sizeof(wifiSSID));
  file.read((uint8_t *)wifiSecret, sizeof(wifiSecret));
  file.read((uint8_t *)&staticIp, sizeof(staticIp));
  file.read((uint8_t *)wifiIp, sizeof(wifiIp));
  file.read((uint8_t *)wifiMask, sizeof(wifiMask));
  file.read((uint8_t *)wifiGw, sizeof(wifiGw));
  file.read((uint8_t *)apSecret, sizeof(apSecret));
  file.read((uint8_t *)apName, sizeof(apName));
  file.read((uint8_t *)chipId, sizeof(chipId));
  strlcpy(chipId, getChipId().c_str(), sizeof(chipId));
  file.read((uint8_t *)cloudIOUserName, sizeof(cloudIOUserName));
  if (firmware < VERSION)
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Migrate Firmware from %F to %F" CR, tags::config, firmware, VERSION);
#endif
    firmware = VERSION;
  }
}
void load(Config &config)
{
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_INFO, 1);
  if (!LittleFS.begin())
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s File storage can't start" CR, tags::config);
#endif
    if (!LittleFS.format())
    {
#ifdef DEBUG_ONOFRE
      Log.error("%s Unable to format Filesystem, please ensure you built firmware with filesystem support." CR, tags::config);
#endif
    }
  }

  if (!LittleFS.exists(configFilenames::config))
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Default config loaded." CR, tags::config);
#endif
    strlcpy(config.nodeId, getChipId().c_str(), sizeof(config.nodeId));
    config.mqttPort = constantsMqtt::defaultPort;
    config.staticIp = false;
    strlcpy(config.apSecret, constantsConfig::apSecret, sizeof(config.apSecret));

#ifdef WIFI_SSID
    strlcpy(config.wifiSSID, WIFI_SSID, sizeof(config.wifiSSID));
#endif
#ifdef WIFI_SECRET
    strlcpy(config.wifiSecret, WIFI_SECRET, sizeof(config.wifiSecret));
#endif
    config.firmware = VERSION;

#ifdef DEBUG_ONOFRE
    Log.notice("%s Config %s loaded." CR, tags::config, String(config.firmware).c_str());
#endif
  }

  File file = LittleFS.open(configFilenames::config, "r+");
  config.load(file);
  file.close();

#ifdef DEBUG_ONOFRE
  Log.notice("%s Stored config loaded." CR, tags::config);
#endif
}
String getChipId()
{
#ifdef ESP8266
  return String(ESP.getChipId());
#endif
#ifdef ESP32
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  return String(chipId);
#endif
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
  inputStr.replace("ã", "a");
  inputStr.replace("ú", "u");
  inputStr.replace("ù", "u");
  inputStr.replace("é", "e");
  inputStr.replace("è", "e");
  inputStr.replace("ê", "e");
  inputStr.replace("í", "i");
  inputStr.replace("ì", "i");
  inputStr.replace("õ", "o");
  inputStr.replace("ó", "o");
  inputStr.replace("ò", "o");
  inputStr.replace("@", "o");
  inputStr.replace("|", "");
}
Config &Config::updateFromJson(JsonObject &root)
{
  char lastNodeId[32];
  strlcpy(lastNodeId, getAtualConfig().nodeId, sizeof(lastNodeId));
  bool reloadWifi = staticIp != root["staticIp"] || strcmp(wifiIp, root["wifiIp"] | "") != 0 || strcmp(wifiMask, root["wifiMask"] | "") != 0 || strcmp(wifiGw, root["wifiGw"] | "") != 0 || strcmp(wifiSSID, root["wifiSSID"] | "") != 0 || strcmp(wifiSecret, root["wifiSecret"] | "") != 0;
  bool reloadMqtt = strcmp(mqttIpDns, root["mqttIpDns"] | "") != 0 || strcmp(mqttUsername, root["mqttUsername"] | "") != 0 || strcmp(mqttPassword, root["mqttPassword"] | "") != 0 || mqttPort != (root["mqttPort"] | constantsMqtt::defaultPort);
  String chipIdStr = getChipId();
  String n_name = root["nodeId"] | chipIdStr;
  normalize(n_name);
  strlcpy(nodeId, n_name.c_str(), sizeof(nodeId));
  strlcpy(mqttIpDns, root["mqttIpDns"] | "", sizeof(mqttIpDns));
  mqttPort = root["mqttPort"] | constantsMqtt::defaultPort;
  strlcpy(mqttUsername, root["mqttUsername"] | "", sizeof(mqttUsername));
  strlcpy(mqttPassword, root["mqttPassword"] | "", sizeof(mqttPassword));
  strlcpy(wifiSSID, root["wifiSSID"] | "", sizeof(wifiSSID));
  strlcpy(wifiSecret, root["wifiSecret"] | "", sizeof(wifiSecret));
  String emoncmsServerStr = root["emoncmsServer"] | "";
  emoncmsServerStr.replace("https", "http");
  while (emoncmsServerStr.endsWith("/"))
  {
    emoncmsServerStr.remove(emoncmsServerStr.lastIndexOf("/"));
  }
  strlcpy(wifiIp, root["wifiIp"] | "", sizeof(wifiIp));
  strlcpy(wifiMask, root["wifiMask"] | "", sizeof(wifiMask));
  strlcpy(wifiGw, root["wifiGw"] | "", sizeof(wifiGw));
  staticIp = root["staticIp"];
  String ap = root["apSecret"] | String(constantsConfig::apSecret);
  strlcpy(apSecret, ap.c_str(), sizeof(apSecret));
  firmware = root["firmware"] | VERSION;
  strlcpy(mqttAvailableTopic, getAvailableTopic().c_str(), sizeof(mqttAvailableTopic));

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