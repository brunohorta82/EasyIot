#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>

void setupWiFi();
void loopWiFi();
void reloadWiFiConfig();
void refreshMDNS(const char *lastName);
size_t systemJSONStatus(Print &output);
#endif