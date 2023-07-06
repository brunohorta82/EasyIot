#include <Arduino.h>

#include "Config.h"
#include "WebServer.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include "Switches.h"
#include "Sensors.h"
#include <esp-knx-ip.h>
#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>

#endif
#ifdef ESP32
#include <WebServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#endif
#include "constants.h"
#include "WebRequests.h"
#include "CloudIO.h"

void checkInternalRoutines()
{
  if (cloudIOSync())
  {
    connectoToCloudIO();
  }
  if (wifiScanRequested())
  {
    scanNewWifiNetworks();
  }
  if (restartRequested())
  {
#ifdef DEBUG_ONOFRE

    Log.notice("%s Rebooting...", tags::system);
#endif
    ESP.restart();
  }
  if (loadDefaultsRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Loading defaults...", tags::system);
#endif
    LittleFS.format();
    requestRestart();
  }
  if (autoUpdateRequested())
  {
    char updateURL[120];
#ifdef ESP32
    sprintf(updateURL, "http://update.bhonofre.pt/firmware/latest-binary?firmwareMode=%s&mcu=esp32", constantsConfig::firmwareMode);
#endif
#ifdef ESP8266
    sprintf(updateURL, "http://update.bhonofre.pt/firmware/latest-binary?firmwareMode=%s&mcu=esp8266", constantsConfig::firmwareMode);
#endif
    char lastVersionURL[120];
#ifdef ESP32
    sprintf(lastVersionURL, "http://update.bhonofre.pt/firmware/latest-version?firmwareMode=%s&mcu=esp32", constantsConfig::firmwareMode);
#endif
#ifdef ESP8266
    sprintf(lastVersionURL, "http://update.bhonofre.pt/firmware/latest-version?firmwareMode=%s&mcu=esp8266", constantsConfig::firmwareMode);
#endif
#ifdef DEBUG_ONOFRE
    Log.notice("%s Starting auto update make sure if this device is connected to the internet.", tags::system);
#endif
    WiFiClient client;
    t_httpUpdate_return ret;
#ifdef ESP8266
    ret = ESPhttpUpdate.update(client, updateURL, String(VERSION));
#endif
#ifdef ESP32
    ret = httpUpdate.update(client, updateURL, String(VERSION));
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

  if (reloadWifiRequested())
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Loading wifi configuration...", tags::system);
#endif
    reloadWiFiConfig();
  }
}

void setup()
{
  LittleFS.begin();
#ifdef DEBUG_ONOFRE
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif

  load(getAtualConfig());
  load(getAtualSwitchesConfig());
  load(getAtualSensorsConfig());
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
  if (!autoUpdateRequested())
  {
    loop(getAtualSwitchesConfig());
    loop(getAtualSensorsConfig());
    loopTime();
  }
  if (WiFi.status() == WL_CONNECTED)
    knx.loop();
}
