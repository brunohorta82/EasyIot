#include <Arduino.h>
#include "Config.h"
#include "WebServer.h"
#include "WiFi.h"
#include "Mqtt.h"
#include "Switches.h"
#include <ESP8266httpUpdate.h>




void setup() {
  Serial.begin(115200);
  loadStoredConfiguration();
  setupWebserverAsync();
  setupWiFi();
  setupMQTT();
  setupSwitchs();
}

void loop() {
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
  ESPhttpUpdate.update(client,url , version);
}