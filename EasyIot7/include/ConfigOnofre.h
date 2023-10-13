#pragma once
#include "constants.h"
#include "Utils.hpp"
#include <ArduinoJson.h>
#include "Actuatores.h"
#include "Sensors.h"
#include <vector>
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif

class ConfigOnofre
{
public:
  int templateId = {0};
  char nodeId[32] = {};
  char chipId[32] = {};
  // MQTT
  char mqttIpDns[40];
  int mqttPort = 1883;
  char mqttUsername[32];
  char mqttPassword[64];
  char healthTopic[64];

  // CLOUDIO
  char cloudIOUsername[40];
  char cloudIOPassword[64];
  char cloudIOhealthTopic[64];
  char cloudIOwriteTopic[64];
  char cloudIOreadTopic[64];
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
  std::vector<Actuator> actuatores;
  std::vector<Sensor> sensors;
  int i2cSDA = -1;
  int i2cSCL = -1;
  void json(JsonVariant &root);
  ConfigOnofre &update(JsonObject &root);
  ConfigOnofre &save();
  ConfigOnofre &init();
  ConfigOnofre &load();
  ConfigOnofre &removeActuator(const char *id);
  void generateId(String &id, const String &name, int familyCode, size_t maxSize);
  void loadTemplate(int templateId);
  void loopActuators();
  void loopSensors();
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
  void controlFeature(StateOrigin origin, JsonObject &action, JsonVariant &result);
  void controlFeature(StateOrigin origin, String topic, String payload);
  void controlFeature(StateOrigin origin, String uniqueId, int state);

private:
  bool reboot = false;
  bool loadDefaults = false;
  bool autoUpdate = false;
  bool wifiReload = false;
  bool cloudIOSync = false;
  bool wifiScan = false;
};
