#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "LittleFS.h"
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif
#include "Utils.hpp"
class Config
{
public:
  double firmware = VERSION;
  char nodeId[32];
  char mqttIpDns[40];
  char cloudIOUserName[40];
  char cloudIOUserPassword[64];
  char mqttUsername[32];
  int mqttPort = 1883;
  char mqttPassword[64];
  char mqttAvailableTopic[128];
  char wifiSSID[32];
  char wifiSecret[64];
  bool dhcp = false;
  char wifiIp[24];
  char wifiMask[24];
  char wifiGw[24];
  char accessPointPassword[64];
  char apName[30];
  char chipId[24];
  char apiUser[32];
  char apiPassword[64];
  char emoncmsServer[80];
  char emoncmsPath[20];
  char emoncmsApikey[80];
  char emoncmsFingerprint[100];
  char availableCloudIO[120];

  uint8_t knxArea = 0;
  uint8_t knxLine = 0;
  uint8_t knxMember = 0;
  long connectedOn = -1;
  char mqttCloudRemoteActionsTopic[128];
  void save();
  void toJson(JsonVariant &root);
  Config &updateFromJson(JsonObject &root);
  void save(File &file) const;
  void load(File &file);
  void requestCloudIOSync();
  bool isCloudIOSyncRequested();

  void requestWifiScan();
  bool isWifiScanRequested();

  void requestRestart();
  bool isRestartRequested();

  void requestAutoUpdate();
  bool isAutoUpdateRequested();

  void requestLoadDefaults();
  bool isLoadDefaultsRequested();

  void requestReloadWifi();
  bool isReloadWifiRequested();

private:
  bool reboot = false;
  bool loadDefaults = false;
  bool autoUpdate = false;
  bool wifiReload = false;
  bool cloudIOSync = false;
  bool wifiScan = false;
};
void load(Config &config);
void normalize(String &inputStr);
boolean isValidNumber(const char *str);
void generateId(String &id, const String &name, int familyCode, size_t maxSize);
String getChipId();
#endif