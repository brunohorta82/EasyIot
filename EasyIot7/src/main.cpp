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
#include <esp-knx-ip.h>
#include <ESP8266mDNS.h>
#include "CloudIO.h"

void checkInternalRoutines()
{
  if (restartRequested())
  {
#ifdef DEBUG
    Log.notice("%s Rebooting...", tags::system);
#endif
    ESP.restart();
  }
  if (loadDefaultsRequested())
  {
#ifdef DEBUG
    Log.notice("%s Loading defaults...", tags::system);
#endif
    SPIFFS.format();
    requestRestart();
  }
  if (autoUpdateRequested())
  {
#ifdef DEBUG
    Log.notice("%s Starting auto update make sure if this device is connected to the internet.", tags::system);
#endif
    WiFiClient client;
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, constantsConfig::updateURL, String(VERSION));
    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
#ifdef DEBUG
      Log.notice("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#endif
      break;
    case HTTP_UPDATE_NO_UPDATES:
#ifdef DEBUG
      Log.notice("HTTP_UPDATE_NO_UPDATES");
#endif
      break;
    case HTTP_UPDATE_OK:
#ifdef DEBUG
      Log.notice("HTTP_UPDATE_OK");
#endif
      break;
    }
  }

  if (reloadWifiRequested())
  {
#ifdef DEBUG
    Log.notice("%s Loading wifi configuration...", tags::system);
#endif
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
  setupWiFi();
  setupMQTT();
  knx.physical_address_set(knx.PA_to_address(getAtualConfig().knxArea, getAtualConfig().knxLine, getAtualConfig().knxMember));
  setupWebserverAsync();
  setupCloudIO();
}

void loop()
{
  checkInternalRoutines();
  webserverServicesLoop();
  loopWiFi();
  loopMqtt();
  if (!autoUpdateRequested())
  {
    loop(getAtualSwitchesConfig());
    loopMqtt();
    loop(getAtualSensorsConfig());
    loopMqtt();
    loopTime();
  }
  knx.loop();
  loopMqtt();
}
