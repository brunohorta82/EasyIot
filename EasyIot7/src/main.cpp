#include <Arduino.h>
#include "Config.h"
#include "WebServer.h"
#include "WiFi.h"
#include "Mqtt.h"
#include <ESP8266httpUpdate.h>




void setup() {
  Serial.begin(115200);
  loadStoredConfiguration();
  setupWebserverAsync();
  setupWiFi();
  setupMQTT();
}

void loop() {
  mDnsLoop();
  loopWiFi();
  loopMqtt();
}

void actualUpdate()
{
  WiFiClient client;
  ESPhttpUpdate.update(client, getUpdateUrl());
}