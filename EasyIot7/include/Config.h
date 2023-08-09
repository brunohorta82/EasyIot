#pragma once
#include "Utils.hpp"
#include <ArduinoJson.h>
#include "LittleFS.h"
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif

class Config
{
public:
  char nodeId[32] = {};
  // MQTT
  char mqttIpDns[40];
  int mqttPort = 1883;
  char mqttUsername[32];
  char mqttPassword[64];
  // CLOUDIO
  char cloudIOUsername[40];
  char cloudIOPassword[64];
  // WIFI
  char wifiSSID[32];
  char wifiSecret[64];
  bool staticIp = false;
  char wifiIp[24];
  char wifiMask[24];
  char wifiGw[24];
  // ACCESS POINT AND PANNEL ADMIN
  char accessPointPassword[64];
  char apiUser[32];
  char apiPassword[64];

  void json(JsonVariant &root);
  Config &update(JsonObject &root);
  Config &save();
  Config &init();
  Config &load();
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
