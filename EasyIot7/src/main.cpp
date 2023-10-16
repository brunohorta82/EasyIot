#include <Arduino.h>
#include "ConfigOnofre.h"
#include "CloudIO.h"
#include "WebServer.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "LittleFS.h"

ConfigOnofre config;

void checkInternalRoutines()
{
  if (config.isCloudIOSyncRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s CloudIO requested.", tags::system);
#endif
    connectToCloudIO();
  }

  if (config.isWifiScanRequested())
  {
    scanNewWifiNetworks();
  }

  if (config.isRestartRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Restart requested.", tags::system);
#endif
    delay(100);
    ESP.restart();
  }

  if (config.isLoadDefaultsRequested())
  {
#ifdef DEBUG_ONOFREs
    Log.notice("%s Load Defaults requested.", tags::system);
#endif
    LittleFS.format();
    config.requestRestart();
  }

  if (config.isAutoUpdateRequested())
  {
    performUpdate();
  }

  if (config.isReloadWifiRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Loading wifi configuration...", tags::system);
#endif
    reloadWiFiConfig();
  }
}

void startFileSystem()
{
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
}

void setup()
{
#ifdef LOW_POWER
#ifdef ESP32
  setCpuFrequencyMhz(80);
#endif
#endif
#ifdef DEBUG_ONOFRE
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif
  startFileSystem();
  config.load();
  config.i2cDiscovery();
  setupWiFi();
  setupCors();
  setupMQTT();
}

void loop()
{
  checkInternalRoutines();
  loopWiFi();
  if (!config.isAutoUpdateRequested())
  {
    webserverServicesLoop();
    loopMqtt();
    if (!config.isLoopFeaturesPaused())
    {
      config.loopActuators();
      config.loopSensors();
    }
    if (WiFi.status() == WL_CONNECTED)
      knx.loop();
  }
}
