#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <JustWifi.h>
#include "Config.h"
#include "WebServer.h"
#include "Mqtt.h"

void setupWiFi();
void loopWiFi();
void reloadWiFiConfig();
size_t systemJSONStatus(Print &output);
#endif