#include "ConfigOnofre.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "WebServer.h"
#include "Actuatores.h"
#include "Sensors.h"
#include "LittleFS.h"
ConfigOnofre &ConfigOnofre::init()
{
#ifdef ESP8266
  strlcpy(chipId, String(ESP.getChipId()).c_str(), sizeof(chipId));
#endif
#ifdef ESP32
  uint32_t chipIdHex = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipIdHex |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  strlcpy(chipId, String(chipIdHex).c_str(), sizeof(chipId));
#endif
  strlcpy(nodeId, chipId, sizeof(nodeId));
  mqttPort = constantsMqtt::defaultPort;
  dhcp = true;
  strlcpy(accessPointPassword, constantsConfig::apSecret, sizeof(accessPointPassword));
  strlcpy(apiUser, constantsConfig::apiUser, sizeof(apiUser));
  strlcpy(apiPassword, constantsConfig::apiPassword, sizeof(apiPassword));

#ifdef WIFI_SSID
  strlcpy(wifiSSID, WIFI_SSID, sizeof(wifiSSID));
#endif
#ifdef WIFI_SECRET
  strlcpy(wifiSecret, WIFI_SECRET, sizeof(wifiSecret));
#endif
#ifdef DEBUG_ONOFRE
  Log.notice("%s Default config loaded." CR, tags::config);
#endif
  return save();
}

ConfigOnofre &ConfigOnofre::load()
{

  if (!LittleFS.exists(configFilenames::config))
  {
    return init();
  }

  File file = LittleFS.open(configFilenames::config, "r+");
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, file);
#ifdef DEBUG_ONOFRE
  if (error)
    Log.notice("%s Failed to read file, using default configuration." CR, tags::config);
#endif
#ifdef ESP8266
  strlcpy(chipId, String(ESP.getChipId()).c_str(), sizeof(chipId));
#endif
#ifdef ESP32
  uint32_t chipIdHex = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipIdHex |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  idSequence = doc["idSequence"];
  strlcpy(chipId, String(chipIdHex).c_str(), sizeof(chipId));
#endif
  strlcpy(nodeId,
          doc["nodeId"] | chipId,
          sizeof(nodeId));
  // CLOUDIO
  strlcpy(cloudIOUsername, doc["cloudIOUsername"] | "", sizeof(cloudIOUsername));
  strlcpy(cloudIOPassword, doc["cloudIOPassword"] | "", sizeof(cloudIOPassword));
  // MQTT
  strlcpy(mqttIpDns, doc["mqttIpDns"] | "", sizeof(mqttIpDns));
  mqttPort = doc["mqttPort"] | 1883;
  strlcpy(mqttUsername, doc["mqttUsername"] | "", sizeof(mqttUsername));
  strlcpy(mqttPassword, doc["mqttPassword"] | "", sizeof(mqttPassword));

  // WIFI
  strlcpy(wifiSSID, doc["wifiSSID"] | "", sizeof(wifiSSID));
  strlcpy(wifiSecret, doc["wifiSecret"] | "", sizeof(wifiSecret));
  dhcp = doc["dhcp"] | false;
  strlcpy(wifiIp, doc["wifiIp"] | "", sizeof(wifiIp));
  strlcpy(wifiMask, doc["wifiMask"] | "", sizeof(wifiMask));
  strlcpy(wifiGw, doc["wifiGw"] | "", sizeof(wifiGw));
  // ACCESS POINT AND PANNEL ADMIN
  strlcpy(accessPointPassword, doc["accessPointPassword"] | constantsConfig::apSecret, sizeof(accessPointPassword));
  strlcpy(apiUser, doc["apiUser"] | constantsConfig::apiUser, sizeof(apiUser));
  strlcpy(apiPassword, doc["apiPassword"] | constantsConfig::apiPassword, sizeof(apiPassword));
  JsonArray features = doc["features"];
  for (auto d : features)
  {
    ActuatorT a;
    a.id = d["id"];
    strlcpy(a.name, d["name"] | "", sizeof(a.name));
    actuatores.push_back(a);
  }
  file.close();
#ifdef DEBUG_ONOFRE
  Log.notice("%s Stored config loaded." CR, tags::config);
#endif
  return *this;
}
ConfigOnofre &ConfigOnofre::save()
{
  File file = LittleFS.open(configFilenames::config, "w+");
  StaticJsonDocument<1024> doc;
  doc["idSequence"] = idSequence;
  if (!String(nodeId).isEmpty())
    doc["nodeId"] = nodeId;
  // MQTT
  if (!String(mqttIpDns).isEmpty())
    doc["mqttIpDns"] = mqttIpDns;
  doc["mqttPort"] = mqttPort;
  if (!String(mqttUsername).isEmpty())
    doc["mqttUsername"] = mqttUsername;
  if (!String(mqttPassword).isEmpty())
    doc["mqttPassword"] = mqttPassword;
  // CLOUDIO
  if (!String(cloudIOUsername).isEmpty())
    doc["cloudIOUsername"] = cloudIOUsername;
  if (!String(cloudIOPassword).isEmpty())
    doc["cloudIOPassword"] = cloudIOPassword;
  // WIFI
  if (!String(wifiSSID).isEmpty())
    doc["wifiSSID"] = wifiSSID;
  if (!String(wifiSecret).isEmpty())
    doc["wifiSecret"] = wifiSecret;
  doc["dhcp"] = dhcp;
  if (!String(wifiIp).isEmpty())
    doc["wifiIp"] = wifiIp;
  if (!String(wifiMask).isEmpty())
    doc["wifiMask"] = wifiMask;
  if (!String(wifiGw).isEmpty())
    doc["wifiGw"] = wifiGw;
  // ACCESS POINT AND PANNEL ADMIN
  doc["accessPointPassword"] = accessPointPassword;
  doc["apiUser"] = apiUser;
  doc["apiPassword"] = apiPassword;
  JsonArray features = doc.createNestedArray("features");
  for (auto s : actuatores)
  {
    JsonObject a = features.createNestedObject();
    a["type"] = "ACTUATOR";
    a["id"] = s.id;
    a["name"] = s.name;
    a["family"] = s.family;
    a["cloudIOSupport"] = s.cloudIOSupport;
    a["haSupport"] = s.haSupport;
    char knxAddress[10];
    sprintf(knxAddress, "%d.%d.%d", s.knxAddress[0], s.knxAddress[1], s.knxAddress[2]);
    a["knxAddress"] = knxAddress;
    a["state"] = s.state;
  }
  if (serializeJson(doc, file) == 0)
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Fail to write File." CR, tags::config);
#endif
  }
  file.close();
#ifdef DEBUG_ONOFRE
  Log.notice("%s ConfigOnofre stored." CR, tags::config);
#endif
  return *this;
}

ConfigOnofre &ConfigOnofre::update(JsonObject &root)
{
  char lastNodeId[32];
  strlcpy(lastNodeId, nodeId, sizeof(lastNodeId));

  bool reloadWifi = dhcp != root["dhcp"] || strcmp(wifiIp, root["wifiIp"] | "") != 0 || strcmp(wifiMask, root["wifiMask"] | "") != 0 || strcmp(wifiGw, root["wifiGw"] | "") != 0 || strcmp(wifiSSID, root["wifiSSID"] | "") != 0 || strcmp(wifiSecret, root["wifiSecret"] | "") != 0;
  bool reloadMqtt = strcmp(mqttIpDns, root["mqttIpDns"] | "") != 0 || strcmp(mqttUsername, root["mqttUsername"] | "") != 0 || strcmp(mqttPassword, root["mqttPassword"] | "") != 0 || mqttPort != (root["mqttPort"] | constantsMqtt::defaultPort);

  String n_name = root["nodeId"] | chipId;
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
  dhcp = root["dhcp"];
  strlcpy(apiPassword, root["apiPassword"] | constantsConfig::apiPassword, sizeof(apiPassword));
  strlcpy(apiUser, root["apiUser"] | constantsConfig::apiUser, sizeof(apiUser));
  strlcpy(accessPointPassword, root["accessPointPassword"] | constantsConfig::apSecret, sizeof(accessPointPassword));
  if (reloadWifi)
  {
    requestReloadWifi();
  }
  if (reloadMqtt)
  {
    setupMQTT();
    // TODO  reloadSwitches();
    // reloadSensors();
  }
  refreshMDNS(lastNodeId);
  return this->save();
}

void ConfigOnofre::json(JsonVariant &root)
{
  root["nodeId"] = nodeId;
  root["idSequence"] = idSequence;
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
  JsonArray features = root.createNestedArray("features");
  for (auto s : actuatores)
  {
    JsonObject a = features.createNestedObject();
    a["type"] = "ACTUATOR";
    a["id"] = s.id;
    a["name"] = s.name;
    a["family"] = s.family;
    a["cloudIOSupport"] = s.cloudIOSupport;
    a["haSupport"] = s.haSupport;
    char knxAddress[10];
    sprintf(knxAddress, "%d.%d.%d", s.knxAddress[0], s.knxAddress[1], s.knxAddress[2]);
    a["knxAddress"] = knxAddress;
    a["state"] = s.state;
  }
}

void ConfigOnofre::requestWifiScan()
{
  wifiScan = true;
}

bool ConfigOnofre::isWifiScanRequested()
{
  if (wifiScan)
  {
    wifiScan = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestCloudIOSync()
{
  cloudIOSync = true;
}

bool ConfigOnofre::isCloudIOSyncRequested()
{
  if (cloudIOSync)
  {
    cloudIOSync = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestReloadWifi()
{
  wifiReload = true;
}
bool ConfigOnofre::isReloadWifiRequested()
{
  if (wifiReload)
  {
    wifiReload = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestRestart()
{
  reboot = true;
}
bool ConfigOnofre::isRestartRequested()
{
  if (reboot)
  {
    reboot = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestAutoUpdate()
{
  autoUpdate = true;
}
bool ConfigOnofre::isAutoUpdateRequested()
{
  if (autoUpdate)
  {
    autoUpdate = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestLoadDefaults()
{
  loadDefaults = true;
}
bool ConfigOnofre::isLoadDefaultsRequested()
{
  if (loadDefaults)
  {
    loadDefaults = false;
    return true;
  }
  return false;
}

int ConfigOnofre::nextId()
{
  idSequence++;
  this->save();
  return idSequence;
}