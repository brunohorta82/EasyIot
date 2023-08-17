#include <Arduino.h>
#include "Config.h"
#include "CloudIO.h"
#include "WebServer.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include "Switches.h"
#include "Sensors.h"
#include "LittleFS.h"
#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#endif
#ifdef ESP32
#include <WebServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#endif
#include <esp-knx-ip.h>
Config config;
void checkInternalRoutines()
{
  if (config.isCloudIOSyncRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s CloudIO requested.", tags::system);
#endif
    connectoToCloudIO();
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
    ESP.restart();
  }

  if (config.isLoadDefaultsRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Load Defaults requested.", tags::system);
#endif
    LittleFS.format();
    config.requestRestart();
  }

  if (config.isAutoUpdateRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Starting auto update make sure if this device is connected to the internet.", tags::system);
#endif
    WiFiClient client;
    t_httpUpdate_return ret;
#ifdef ESP8266
    ret = ESPhttpUpdate.update(client, "http://update.bhonofre.pt/firmware/update", String(VERSION));
#endif
#ifdef ESP32
    ret = httpUpdate.update(client, "http://update.bhonofre.pt/firmware/update", String(VERSION));
#endif
    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
#ifdef DEBUG_ONOFRE
#ifdef ESP8266
      Log.notice("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#endif
#ifdef ESP32
      Log.notice("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
#endif
#endif
      break;
    case HTTP_UPDATE_NO_UPDATES:
#ifdef DEBUG_ONOFRE
      Log.notice("HTTP_UPDATE_NO_UPDATES");
#endif
      break;
    case HTTP_UPDATE_OK:
#ifdef DEBUG_ONOFRE
      Log.notice("HTTP_UPDATE_OK");
#endif
      break;
    }
  }

  if (config.isReloadWifiRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Loading wifi configuration...", tags::system);
#endif
    reloadWiFiConfig();
  }
}
void setTime()
{
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_INFO, 1);
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

#ifdef DEBUG_ONOFRE
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif
  setTime();
  startFileSystem();
  config.load();

  setupWiFi();
  setupMQTT();
  setupWebserverAsync();
}

void loop()
{
  checkInternalRoutines();
  webserverServicesLoop();
  loopWiFi();
  loopMqtt();
  if (!config.isAutoUpdateRequested())
  {
    config.loopSwitches();
  }
  if (WiFi.status() == WL_CONNECTED)
    knx.loop();
}
