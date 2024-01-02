#include <Arduino.h>
#include "ConfigOnofre.h"
#include "CloudIO.h"
#include "WebServer.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "LittleFS.h"
#ifdef ESP32
#include "driver/adc.h"
#endif
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
#ifdef DEBUG_ONOFREs
    Log.notice("%sScan Network.", tags::system);
#endif
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
#ifdef DEBUG_ONOFREs
    Log.notice("%sAuto Update Request.", tags::system);
#endif
    performUpdate();
  }

  if (config.isReloadWifiRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Loading wifi configuration...", tags::system);
#endif
#if defined(ESP8266) || defined(LEGACY_PROVISON)
    reloadWiFiConfig();
#endif
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
#ifdef ESP32
void featuresTask(void *pvParameters)
{
  for (;;)
  {
    if (!config.isLoopFeaturesPaused())
    {
      config.loopSensors();
    }
    vTaskDelay(1);
  }
}
#endif
void setup()
{
#ifdef DEBUG_ONOFRE
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif
  startFileSystem();
  config.load();
#ifdef ESP32
  config.i2cDiscovery();
  config.pzemDiscovery();
#endif
  setupWiFi();
  setupCors();
  setupMQTT();

#ifdef ESP32
  xTaskCreatePinnedToCore(featuresTask, "Features-Task", 4048, NULL, 100, NULL, 1);
#endif
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
    }
#ifdef ESP8266
    if (!config.isLoopFeaturesPaused())
    {
      config.loopSensors();
    }
#endif
  }
}
