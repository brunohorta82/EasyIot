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
#include <Time.h>
const char *NTP_SERVER = "pt.pool.ntp.org";
const char *TZ_INFO = "WET-0WEST-1,M3.5.0/01:00:00,M10.5.0/02:00:00";
tm timeinfo;
time_t now;
unsigned long lastNTPtime;

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
  setupWiFi();
  setupMQTT();
  knx.physical_address_set(knx.PA_to_address(getAtualConfig().knxArea, getAtualConfig().knxLine, getAtualConfig().knxMember));
  configTime(0, 0, NTP_SERVER);
  setenv("TZ", TZ_INFO, 1);
  setupWebserverAsync();
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
  knx.loop();

  if (WiFi.status() == WL_CONNECTED && lastNTPtime + 1000 < millis())
  {
    time_t now = time(nullptr);
    lastNTPtime = millis();
    sendToServerEvents("my_time", String(ctime(&now)));
  }
}

void actualUpdate()
{
  WiFiClient client;
  ESPhttpUpdate.update(client, constantsConfig::updateURL, VERSION);
}