#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>


void setupWiFi();
void loopWiFi();
void reloadWiFiConfig();
size_t systemJSONStatus(Print &output);
#endif