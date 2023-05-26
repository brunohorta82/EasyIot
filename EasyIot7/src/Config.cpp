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
  this->save(file);
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
  root["wifiSSID"] = wifiSSID;
  root["wifiSecret"] = wifiSecret;
  root["wifiIp"] = wifiIp;
  root["wifiMask"] = wifiMask;
  root["wifiGw"] = wifiGw;
  root["staticIp"] = staticIp;
  root["apName"] = apName;
  root["firmware"] = VERSION;
  root["chipId"] = chipId;
  root["mac"] = WiFi.macAddress();
  root["apiUser"] = apiUser;
  root["apiPassword"] = apiPassword;
  root["emoncmsServer"] = emoncmsServer;
  root["emoncmsPath"] = emoncmsPath;
  root["emoncmsApikey"] = emoncmsApikey;
  root["knxArea"] = knxArea;
  root["knxLine"] = knxLine;
  root["knxMember"] = knxMember;
  root["currentWifiIp"] = WiFi.localIP().toString();
  root["wifiStatus"] = WiFi.isConnected();
  root["signal"] = WiFi.RSSI();
  root["mode"] = (int)WiFi.getMode();
  root["mqttConnected"] = mqttConnected();
  root["firmwareMode"] = constantsConfig::firmwareMode;
}

void Config::save(File &file) const
{
  file.write((uint8_t *)&firmware, sizeof(firmware));
  file.write((uint8_t *)nodeId, sizeof(nodeId));
  file.write((uint8_t *)mqttIpDns, sizeof(mqttIpDns));
  file.write((uint8_t *)mqttUsername, sizeof(mqttUsername));
  file.write((uint8_t *)mqttAvailableTopic, sizeof(mqttAvailableTopic));
  file.write((uint8_t *)&mqttPort, sizeof(mqttPort));
  file.write((uint8_t *)mqttPassword, sizeof(mqttPassword));
  file.write((uint8_t *)wifiSSID, sizeof(wifiSSID));
  file.write((uint8_t *)wifiSecret, sizeof(wifiSecret));
  file.write((uint8_t *)&staticIp, sizeof(staticIp));
  file.write((uint8_t *)wifiIp, sizeof(wifiIp));
  file.write((uint8_t *)wifiMask, sizeof(wifiMask));
  file.write((uint8_t *)wifiGw, sizeof(wifiGw));
  file.write((uint8_t *)apSecret, sizeof(apSecret));
  file.write((uint8_t *)apName, sizeof(apName));
  file.write((uint8_t *)chipId, sizeof(chipId));
  file.write((uint8_t *)apiUser, sizeof(apiUser));
  file.write((uint8_t *)apiPassword, sizeof(apiPassword));
  file.write((uint8_t *)emoncmsServer, sizeof(emoncmsServer));
  file.write((uint8_t *)emoncmsPath, sizeof(emoncmsPath));
  file.write((uint8_t *)emoncmsApikey, sizeof(emoncmsApikey));
  file.write((uint8_t *)&knxArea, sizeof(knxArea));
  file.write((uint8_t *)&knxLine, sizeof(knxLine));
  file.write((uint8_t *)&knxMember, sizeof(knxMember));
  file.write((uint8_t *)cloudIOUserName, sizeof(cloudIOUserName));
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
  file.read((uint8_t *)apiUser, sizeof(apiUser));
  file.read((uint8_t *)apiPassword, sizeof(apiPassword));
  file.read((uint8_t *)emoncmsServer, sizeof(emoncmsServer));
  file.read((uint8_t *)emoncmsPath, sizeof(emoncmsPath));
  file.read((uint8_t *)emoncmsApikey, sizeof(emoncmsApikey));
  file.read((uint8_t *)&knxArea, sizeof(knxArea));
  file.read((uint8_t *)&knxLine, sizeof(knxLine));
  file.read((uint8_t *)&knxMember, sizeof(knxMember));
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
  knxArea = static_cast<uint8_t>(root["knxArea"] | 0);
  knxLine = static_cast<uint8_t>(root["knxLine"] | 0);
  knxMember = static_cast<uint8_t>(root["knxMember"] | 0);
  emoncmsServerStr.replace("https", "http");
  knx.physical_address_set(knx.PA_to_address(getAtualConfig().knxArea, getAtualConfig().knxLine, getAtualConfig().knxMember));
  while (emoncmsServerStr.endsWith("/"))
  {

    emoncmsServerStr.remove(emoncmsServerStr.lastIndexOf("/"));
  }
  strlcpy(emoncmsServer, emoncmsServerStr.c_str(), sizeof(emoncmsServer));
  String emoncmsPathStr = root["emoncmsPath"] | "";
  while (emoncmsPathStr.endsWith("/"))
  {
    emoncmsPathStr.remove(emoncmsPathStr.lastIndexOf("/"));
  }
  strlcpy(emoncmsPath, emoncmsPathStr.c_str(), sizeof(emoncmsPath));

  strlcpy(emoncmsApikey, root["emoncmsApikey"] | "", sizeof(emoncmsApikey));
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
  getAtualConfig().save();
  return *this;
}