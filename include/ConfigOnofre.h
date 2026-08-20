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

// Stable API result codes for POST /config. Keep the numeric values in sync
// with webpanel/js/index.js so the panel can explain a rejected wiring change.
enum class ConfigUpdateResult : uint8_t
{
  OK = 0,
  INVALID_REQUEST = 1,
  INVALID_PIN = 2,
  PIN_COUNT_MISMATCH = 3,
  PIN_CONFLICT = 4,
  BUSY = 5,
  PERSISTENCE_FAILED = 6
};

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
  ConfigUpdateResult update(JsonObject &root, JsonVariant &responseRoot);
  bool persist();
  ConfigOnofre &save();
  ConfigOnofre &init();
  ConfigOnofre &load();
  bool tryBeginFeatureAccess();
  void endFeatureAccess();
  ConfigOnofre &reloadFeatures();
  void i2cDiscovery();
  void requestI2cDiscovery();
  void serviceDeferredI2cDiscovery();
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

  bool requestTemplateChange(int templateId);
  int peekTemplateChangeRequest() const;
  void clearTemplateChangeRequest(int templateId);

  bool validOutputPin(unsigned int pin) const
  {
    for (auto p : DefaultPins::outputInputPins)
    {
      if (p == pin)
        return true;
    }
    return false;
  }
  bool validInputPin(unsigned int pin) const
  {
    if (validOutputPin(pin))
      return true;
#if defined(ESP32) && !defined(ESP32C6)
    for (auto p : DefaultPins::intputOnlyPins)
    {
      if (p == pin)
        return true;
    }
#endif
    return false;
  }
  // Sensor wiring is not uniformly input-only. Data buses and UART TX pins
  // must be safe to drive, while a few receive/echo pins may use an ESP32
  // input-only GPIO. Keep this separate from PinRole: sensor claims must never
  // enter the actuator output-release path.
  bool validSensorPin(SensorDriver driver, size_t slot, unsigned int pin) const
  {
    if (!Sensor::isSupportedOnCurrentTarget(driver))
      return false;
    switch (driver)
    {
    case SensorDriver::PIR:
      return slot == 0 && validInputPin(pin);
    case SensorDriver::HCSR04:
      return slot < 2 && (slot == 0 ? validOutputPin(pin) : validInputPin(pin));
    case SensorDriver::LD2410:
    case SensorDriver::PZEM_004T_V03:
    case SensorDriver::PZEM_004T_V01:
      return slot < 2 && (slot == 0 ? validInputPin(pin) : validOutputPin(pin));
    case SensorDriver::HAN:
#ifdef ESP32
      // ESP32's canonical fixed array is RX, TX.
      return slot < 2 && (slot == 0 ? validInputPin(pin) : validOutputPin(pin));
#else
      // The legacy ESP8266 implementation constructs SoftwareSerial from
      // inputs[1], inputs[0], so its effective order remains TX, RX.
      return slot < 2 && (slot == 0 ? validOutputPin(pin) : validInputPin(pin));
#endif
    case SensorDriver::DHT_11:
    case SensorDriver::DHT_21:
    case SensorDriver::DHT_22:
    case SensorDriver::RAIN:
    case SensorDriver::DOOR:
    case SensorDriver::WINDOW:
    case SensorDriver::DS18B20:
      return slot == 0 && validOutputPin(pin);
    case SensorDriver::LTR303X:
    case SensorDriver::SHT4X:
    case SensorDriver::TMF882X:
      return slot < 2 && validOutputPin(pin);
    case SensorDriver::INVALID_SENSOR:
    default:
      return false;
    }
  }
  // Compatibility for older call sites: a generic valid pin must be safe to
  // drive. Input-only GPIOs require the explicit validInputPin() check.
  bool validPin(unsigned int pin) const
  {
    return validOutputPin(pin);
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
    for (const auto &s : sensors)
    {
      std::vector<unsigned int> claimedInputs;
      if (!Sensor::fixedRuntimeInputs(s.driver, claimedInputs))
        claimedInputs = s.inputs;
      for (auto p : claimedInputs)
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
  bool tryBeginConfigUpdate();
  void endConfigUpdate();
#ifdef ESP32
  // These requests cross AsyncTCP/MQTT and main-loop task boundaries on ESP32.
  // They intentionally coalesce: each flag represents pending work, not a queue.
  std::atomic<bool> reboot{false};
  std::atomic<bool> loadDefaults{false};
  std::atomic<bool> autoUpdate{false};
  std::atomic<bool> wifiReload{false};
  std::atomic<bool> cloudIOSync{false};
  std::atomic<bool> wifiScan{false};
  std::atomic<bool> i2cDiscoveryRequested{false};
  // One non-reentrant lease serializes every top-level reader or writer of the
  // live feature vectors. Callers must fail fast instead of waiting while an
  // AsyncTCP/MQTT callback or another task owns it.
  std::atomic<bool> featureAccessInProgress{false};
  std::atomic<int> requestedTemplateId{Template::NO_TEMPLATE};
#else
  // ESP8266 callbacks run cooperatively; plain flags avoid unsupported atomic
  // helpers and no path below waits for a feature loop to finish.
  bool reboot = false;
  bool loadDefaults = false;
  bool autoUpdate = false;
  bool wifiReload = false;
  bool cloudIOSync = false;
  bool wifiScan = false;
  bool i2cDiscoveryRequested = false;
  // ESP8266 runs cooperatively. Never wait or yield for this lease: nested or
  // competing access is rejected and retried by the top-level caller.
  bool featureAccessInProgress = false;
  int requestedTemplateId = Template::NO_TEMPLATE;
#endif
};
