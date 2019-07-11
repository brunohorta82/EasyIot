#ifndef WIFI_H
#define WIFI_H
#include <Arduino.h>
#include <JustWifi.h>
#include "Config.h"
#include "WebServer.h"
void setupWiFi();
void loopWiFi();
void reloadWiFiConfig();
String wifiJSONStatus();
#endif