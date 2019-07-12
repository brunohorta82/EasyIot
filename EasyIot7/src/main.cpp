#include <Arduino.h>
#include "Config.h"
#include "WebServer.h"
#include "WiFi.h"
#include "Mqtt.h"
#include "Switches.h"
#include <ESP8266httpUpdate.h>

void checkInternalRoutines()
{
  if (restartRequested())
  {
    logger(SYSTEM_TAG, "Rebooting...");
    ESP.restart();
  }
  if (loadDefaultsRequested())
  {
    logger(SYSTEM_TAG, "Loading defaults...");
    SPIFFS.format();
    requestRestart();
  }
  if (autoUpdateRequested())
  {
    logger(SYSTEM_TAG, "Starting auto update make sure if this device is connected to the internet.");
    WiFiClient client;
    ESPhttpUpdate.update(client, getUpdateUrl());
  }

  if (reloadWifiRequested())
  {
    logger(SYSTEM_TAG, "Loading wifi configuration...");
    reloadWiFiConfig();
  }
}

void setup()
{
  Serial.begin(115200);
  loadStoredConfiguration();
  loadStoredSwitchs();
  setupWebserverAsync();
  setupWiFi();
  setupMQTT();
}

void loop()
{
  checkInternalRoutines();
  mDnsLoop();
  loopWiFi();
  loopMqtt();
  loopSwitches();
}

void actualUpdate()
{
  WiFiClient client;
  const String url = getUpdateUrl();
  const String version = String(FIRMWARE_VERSION);
  ESPhttpUpdate.update(client, url, version);
}