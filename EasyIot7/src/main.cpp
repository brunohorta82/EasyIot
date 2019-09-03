#include <Arduino.h>
#include "Config.h"
#include "WebServer.h"
#include "WiFi.h"
#include "Mqtt.h"
#include "Switches.h"
#include "Sensors.h"
#include <ESP8266httpUpdate.h>
#include "constants.h"
#include "WebRequests.h"

void checkInternalRoutines()
{
  if (restartRequested())
  {
    Log.notice("%s Rebooting...", tags::system);
    ESP.restart();
  }
  if (loadDefaultsRequested())
  {
    Log.notice("%s Loading defaults...", tags::system);
    SPIFFS.format();
    requestRestart();
  }
  if (autoUpdateRequested())
  {
    Log.notice("%s Starting auto update make sure if this device is connected to the internet.", tags::system);
    WiFiClient client;
    ESPhttpUpdate.update(client, constantsConfig::updateURL);
  }

  if (reloadWifiRequested())
  {
    Log.notice("%s Loading wifi configuration...", tags::system);
    reloadWiFiConfig();
  }
}

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif

  loadStoredConfiguration(getAtualConfig());
  load(getAtualSwitchesConfig());
  load(getAtualSensorsConfig());
  setupWebserverAsync();
  setupWiFi();
  setupMQTT();
}

void loop()
{
  checkInternalRoutines();
  webserverServicesLoop();
  loopWiFi();
  loopMqtt();
  loop(getAtualSwitchesConfig());
  loop(getAtualSensorsConfig());
  loopMqtt();
}

void actualUpdate()
{
  WiFiClient client;
  ESPhttpUpdate.update(client, constantsConfig::updateURL, VERSION);
}