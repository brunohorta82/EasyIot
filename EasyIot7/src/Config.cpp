#include "Config.h"
#include "constants.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "WebServer.h"
#include "Switches.h"
#include "Sensors.h"
#include "Templates.hpp"

extern Config config;
void generateId(String &id, const String &name, int familyCode, size_t maxSize)
{
  id.reserve(maxSize);
  id.concat(getChipId());
  id.concat(name);
  id.concat(familyCode);
  id.toLowerCase();
  normalize(id);
}

void Config::save()
{
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
  root["idSequence"] = 0;
  root["chipId"] = chipId;
  root["mqttIpDns"] = mqttIpDns;
  root["mqttPort"] = mqttPort;
  root["mqttUsername"] = mqttUsername;
  root["mqttPassword"] = mqttPassword;
  root["accessPointPassword"] = accessPointPassword;
  root["apiUser"] = apiUser;
  root["apiPassword"] = apiPassword;
  root["wifiSSID"] = wifiSSID;
  root["wifiSecret"] = wifiSecret;
  root["dhcp"] = dhcp;
  // DYNAMIC VALUES
  root["mqttConnected"] = mqttConnected();
  root["wifiIp"] = WiFi.localIP().toString();
  root["wifiMask"] = WiFi.subnetMask().toString();
  root["wifiGw"] = WiFi.gatewayIP().toString();
  root["firmware"] = String(VERSION);
  root["mac"] = WiFi.macAddress();
  root["wifiStatus"] = WiFi.isConnected();
  root["signal"] = WiFi.RSSI();
  JsonVariant outInPins = root.createNestedArray("outInPins");
  JsonVariant inPins = root.createNestedArray("inPins");
#ifdef ESP8266
  std::vector<int> pinsRef = {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16};
#endif
#ifdef ESP32
  std::vector<int> pinsRef = {4, 5, 7, 8, 13, 14, 19, 20, 21, 22, 25, 26, 27, 32, 33};
  std::vector<int> intputOnly = {34, 35, 36, 37, 38};
#endif
  for (auto p : pinsRef)
  {
    outInPins.add(p);
  }
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
  file.write((uint8_t *)&dhcp, sizeof(dhcp));
  file.write((uint8_t *)wifiIp, sizeof(wifiIp));
  file.write((uint8_t *)wifiMask, sizeof(wifiMask));
  file.write((uint8_t *)wifiGw, sizeof(wifiGw));
  file.write((uint8_t *)accessPointPassword, sizeof(accessPointPassword));
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
  strlcpy(chipId, getChipId().c_str(), sizeof(chipId));
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
  file.read((uint8_t *)&dhcp, sizeof(dhcp));
  file.read((uint8_t *)wifiIp, sizeof(wifiIp));
  file.read((uint8_t *)wifiMask, sizeof(wifiMask));
  file.read((uint8_t *)wifiGw, sizeof(wifiGw));
  file.read((uint8_t *)accessPointPassword, sizeof(accessPointPassword));
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
  if (!LittleFS.exists(configFilenames::config))
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Default config loaded." CR, tags::config);
#endif
    strlcpy(config.nodeId, getChipId().c_str(), sizeof(config.nodeId));
    config.mqttPort = constantsMqtt::defaultPort;
    config.dhcp = true;
    strlcpy(config.accessPointPassword, constantsConfig::apSecret, sizeof(config.accessPointPassword));
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
  strlcpy(lastNodeId, config.nodeId, sizeof(lastNodeId));
  bool reloadWifi = dhcp != root["dhcp"] || strcmp(wifiIp, root["wifiIp"] | "") != 0 || strcmp(wifiMask, root["wifiMask"] | "") != 0 || strcmp(wifiGw, root["wifiGw"] | "") != 0 || strcmp(wifiSSID, root["wifiSSID"] | "") != 0 || strcmp(wifiSecret, root["wifiSecret"] | "") != 0;
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
  strlcpy(wifiIp, root["wifiIp"] | "", sizeof(wifiIp));
  strlcpy(wifiMask, root["wifiMask"] | "", sizeof(wifiMask));
  strlcpy(wifiGw, root["wifiGw"] | "", sizeof(wifiGw));
  dhcp = root["dhcp"] | true;
  String ap = root["accessPointPassword"] | String(constantsConfig::apSecret);
  strlcpy(accessPointPassword, ap.c_str(), sizeof(accessPointPassword));
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
  config.save();
  return *this;
}

void Config::requestWifiScan()
{
  wifiScan = true;
}

bool Config::isWifiScanRequested()
{
  if (wifiScan)
  {
    wifiScan = false;
    return true;
  }
  return false;
}

void Config::requestCloudIOSync()
{
  cloudIOSync = true;
}

bool Config::isCloudIOSyncRequested()
{
  if (cloudIOSync)
  {
    cloudIOSync = false;
    return true;
  }
  return false;
}

void Config::requestReloadWifi()
{
  wifiReload = true;
}
bool Config::isReloadWifiRequested()
{
  if (wifiReload)
  {
    wifiReload = false;
    return true;
  }
  return false;
}

void Config::requestRestart()
{
  reboot = true;
}
bool Config::isRestartRequested()
{
  if (reboot)
  {
    reboot = false;
    return true;
  }
  return false;
}

void Config::requestAutoUpdate()
{
  autoUpdate = true;
}
bool Config::isAutoUpdateRequested()
{
  if (autoUpdate)
  {
    autoUpdate = false;
    return true;
  }
  return false;
}

void Config::requestLoadDefaults()
{
  loadDefaults = true;
}
bool Config::isLoadDefaultsRequested()
{
  if (loadDefaults)
  {
    loadDefaults = false;
    return true;
  }
  return false;
}
