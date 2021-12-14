#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>

void setupWiFi();
void loopWiFi();
bool wifiConnected();
void reloadWiFiConfig();
void scanNewWifiNetworks();
void enableScan();
void refreshMDNS(const char *lastName);
#endif