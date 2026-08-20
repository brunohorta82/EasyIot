#pragma once
#include "Constants.h"
#include "Utils.hpp"
#include <ArduinoJson.h>
#include "Actuatores.h"
#include "Sensors.h"
#include <vector>
#ifdef ESP32
#include <atomic>
#endif
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
/** Why the device last restarted, in plain text. Available in release builds too. */
String deviceResetReason();

class ConfigOnofre
{
public:
  int templateId = {0};
  char nodeId[32] = {};
  char chipId[32] = {};
  char provisionId[32] = {};
  // MQTT
  char mqttIpDns[40];
  int mqttPort = 1883;
  char mqttUsername[32];
  char mqttPassword[64];
  char healthTopic[64];
  // KNX
  uint8_t knxIdRegister = 0;
  // CLOUDIO
  char cloudIOUsername[40];
  char cloudIOPassword[64];
  bool cloudIOReady{false};
  char cloudIOhealthTopic[128];
  char cloudIOwriteTopic[128];
  char cloudIOreadTopic[128];
  // WIFI
  char wifiSSID[32];
  char wifiSecret[64];
  bool dhcp = 1;
  char wifiIp[24];
  char wifiMask[24];
  char wifiGw[24];
  // ACCESS POINT AND PANNEL ADMIN
  char accessPointPassword[64];
  char apiUser[32];
  char apiPassword[64];
  std::vector<Actuator> actuatores;
  std::vector<Sensor> sensors;
  Adafruit_SSD1306 *display = NULL;

  // CONTROL VARIABLES
  int featureIds = 0;
  void json(JsonVariant &root, bool allFields);
  ConfigOnofre &update(JsonObject &root);
  ConfigOnofre &save();
  ConfigOnofre &init();
  ConfigOnofre &load();
  ConfigOnofre &pauseFeatures();
  ConfigOnofre &resumeFeatures();
  ConfigOnofre &reloadFeatures();
  void i2cDiscovery();
#ifdef ESP32
  void pzemDiscovery();
#endif
  bool isSensorExists(int hwAddress);
  void generateId(String &id, const String &name, int familyCode, int io, size_t maxSize);
  bool loadTemplate(int templateId);
  void loopActuators();
  void loopSensors();

  // CLOUDIO
  void requestCloudIOSync();
  bool isCloudIOSyncRequested();
  void startCloudIOWatchdog();
  void stopCloudIOWatchdog();

  void requestWifiScan();
  bool isWifiScanRequested();

  void requestRestart();
  bool isRestartRequested();

  void requestAutoUpdate();
  bool isAutoUpdateRequested();
  bool takeAutoUpdateRequest();

  void requestLoadDefaults();
  bool isLoadDefaultsRequested();

  void requestTemplateChange(int templateId);
  int takeTemplateChangeRequest();

  bool isLoopFeaturesPaused() const;
  bool validPin(unsigned int pin)
  {
    for (auto p : DefaultPins::outputInputPins)
    {
      if (p == pin)
        return true;
    }
    return false;
  }
  // True when a feature already drives this pin. Without this a second feature
  // could be created on top of the first, leaving two drivers on one GPIO.
  bool pinInUse(unsigned int pin)
  {
    for (auto &a : actuatores)
    {
      for (auto p : a.inputs)
        if (p == pin)
          return true;
      for (auto p : a.outputs)
        if (p == pin)
          return true;
    }
    for (auto &s : sensors)
    {
      for (auto p : s.inputs)
        if (p == pin)
          return true;
    }
    return false;
  }
  void requestReloadWifi();
  bool isReloadWifiRequested();
  void controlFeature(StateOrigin origin, JsonObject &action, JsonVariant &result);
  void controlFeature(StateOrigin origin, String topic, String payload);
  void controlFeature(StateOrigin origin, String uniqueId, int state);

private:
  bool beginFeatureLoop();
  void endFeatureLoop();
  bool reboot = false;
  bool loadDefaults = false;
  bool autoUpdate = false;
  bool wifiReload = false;
  bool cloudIOSync = false;
  bool wifiScan = false;
#ifdef ESP32
  std::atomic<bool> pauseFeaturesLoop{false};
  std::atomic<unsigned int> activeFeatureLoops{0};
  std::atomic<int> requestedTemplateId{Template::NO_TEMPLATE};
#else
  bool pauseFeaturesLoop = false;
  int requestedTemplateId = Template::NO_TEMPLATE;
#endif
};
