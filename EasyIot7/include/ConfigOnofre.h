#pragma once
#include "constants.h"
#include "Utils.hpp"
#include <ArduinoJson.h>
#include "Actuatores.h"
#include "Sensors.h"
#include "Notify.h"
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif

class ConfigOnofre
{
public:
  int templateId = {0};
  char nodeId[32] = {};
  char chipId[32] = {};
  int idSequence = 0;
  // MQTT
  char mqttIpDns[40];
  int mqttPort = 1883;
  char mqttUsername[32];
  char mqttPassword[64];
  char writeTopic[128];
  char readTopic[128];
  char healthTopic[128];
  // CLOUDIO
  char cloudIOUsername[40];
  char cloudIOPassword[64];
  // WIFI
  char wifiSSID[32];
  char wifiSecret[64];
  bool dhcp = true;
  char wifiIp[24];
  char wifiMask[24];
  char wifiGw[24];
  // ACCESS POINT AND PANNEL ADMIN
  char accessPointPassword[64];
  char apiUser[32];
  char apiPassword[64];
  std::vector<ActuatorT> actuatores{};
  std::vector<SensorT> sensors{};
  int i2cSDA = -1;
  int i2cSCL = -1;
  void json(JsonVariant &root);
  ConfigOnofre &update(JsonObject &root);
  ConfigOnofre &save();
  ConfigOnofre &init();
  ConfigOnofre &load();
  ConfigOnofre &removeSwitch(const char *id);
  int nextId();
  void loadTemplate(int templateId);
  void loopSwitches();
  String controlSwitch(const char *id, SwitchStateOrigin origin, String state);
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
