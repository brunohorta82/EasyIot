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
      logger("[SYSTEM]", "Rebooting...");
      ESP.restart();
  }
  if(autoUpdateRequested()){
     SPIFFS.format();
    requestRestart();
  }

  if(reloadWifiRequested()){
     reloadWiFiConfig();
  }
}

void setup()
{
  Serial.begin(115200);
  loadStoredConfiguration();
  setupWebserverAsync();
  setupWiFi();
  setupMQTT();
  setupSwitchs();
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