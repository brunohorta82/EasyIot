#include <Arduino.h>
#include "ConfigOnofre.h"
#include "CloudIO.h"
#include "WebServer.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "LittleFS.h"
#ifdef ESP32
#include "nvs_flash.h"
#include "driver/adc.h"
#include "esp_system.h"
#endif
ConfigOnofre config;

#ifdef DEBUG_ONOFRE
namespace
{
const char *webSecureState()
{
#if defined(WEB_SECURE_ON)
  return "on";
#else
  return "off";
#endif
}

const char *langDefault()
{
#if defined(CONFIG_LANG_PT)
  return "pt";
#else
  return "en";
#endif
}

void logBootBanner()
{
  const String firmwareVersion = String(VERSION);
  const String firmwareBuildDate = String(__DATE__ " " __TIME__);
  const String resetReason = deviceResetReason();

  Log.info("----------------------------------------------" CR);
  Log.info("%s Reset reason: %s" CR, tags::system, resetReason.c_str());
  Log.info("%s Firmware Version: %s" CR, tags::system, firmwareVersion.c_str());
  Log.info("----------------------------------------------" CR);
  Log.info("%s Device: %s" CR, tags::build, config.nodeId);
  Log.info("%s Version: %s" CR, tags::build, firmwareVersion.c_str());
  Log.info("%s buildDate: %s" CR, tags::build, firmwareBuildDate.c_str());
#ifdef ESP8266
  Log.info("%s MCU: ESP8266" CR, tags::build);
#else
  Log.info("%s MCU: ESP32" CR, tags::build);
#endif
  Log.info("%s Mode: DEBUG" CR, tags::build);
  Log.info("%s WEB_SECURE_ON: %s" CR, tags::build, webSecureState());
  Log.info("%s Lang default: %s" CR, tags::build, langDefault());
  Log.info("----------------------------------------------" CR);
}
} // namespace
#endif

void checkInternalRoutines()
{
  if (config.isCloudIOSyncRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s CloudIO requested." CR, tags::system);
    Log.info("----------------------------------------------" CR);
#endif
    connectToCloudIO();
  }

  if (config.isWifiScanRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Scan Network." CR, tags::system);
#endif
    scanNewWifiNetworks();
  }

  if (config.isRestartRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Restart requested." CR, tags::system);
#endif
    delay(100);
    ESP.restart();
  }

  if (config.isLoadDefaultsRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Load Defaults requested." CR, tags::system);
#endif
#if defined(ESP32) && !defined(LEGACY_PROVISON)
    ESP_ERROR_CHECK(nvs_flash_erase());
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_ERROR_CHECK(nvs_flash_init());
    }
#endif
    LittleFS.format();
    config.requestRestart();
  }

  if (config.isAutoUpdateRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Auto Update Request." CR, tags::system);
#endif
    config.pauseFeatures();
    stopWebserver();
    performUpdate();
  }

  if (config.isReloadWifiRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Loading wifi configuration..." CR, tags::system);
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
#ifndef DEBUG_SERIAL_BAUD
#define DEBUG_SERIAL_BAUD 115200
#endif
  Serial.begin(DEBUG_SERIAL_BAUD);
  Serial.println();
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif

  startFileSystem();
  config.load();
#ifdef DEBUG_ONOFRE
  logBootBanner();
#endif
  setupWiFi();
  setupCors();
#ifdef ESP32
#ifndef HAN_MODE
  config.i2cDiscovery();
//  config.pzemDiscovery();
#endif
#endif
  setupMQTT(false);
#ifdef ESP32
// Priority 100 is above configMAX_PRIORITIES, so FreeRTOS clamped it to the top of
// the range — putting sensor polling above the WiFi and TCP/IP tasks, which then
// only ran when this one happened to yield. Sensor reads are background work; keep
// them below the network stack. Arduino's own loop runs at 1.
#define FEATURES_TASK_PRIORITY 2
#ifdef HAN_MODE
  xTaskCreate(featuresTask, "Features-Task", 4048, NULL, FEATURES_TASK_PRIORITY, NULL);
#else
  xTaskCreatePinnedToCore(featuresTask, "Features-Task", 4048, NULL, FEATURES_TASK_PRIORITY, NULL, 1);
#endif
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
