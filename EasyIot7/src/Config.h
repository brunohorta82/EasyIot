#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> 
#include "WiFi.h"
#include "Mqtt.h"
#include <ArduinoLog.h>

struct Config {
  char nodeId[32];
  char homeAssistantAutoDiscoveryPrefix[32];
  char mqttIpDns[40];
  char mqttUsername[32];
  int mqttPort;
  char mqttPassword[24];
  char wifiSSID[32];
  char wifiSecret[24];
  char wifiSSID2[24];
  char wifiSecret2[24];
  bool staticIp;
  char wifiIp[24];
  char wifiMask[24];
  char wifiGw[24];
  char apSecret[16];
  char apName[30];
  long configTime;
  char configkey[64];
  char hardware[24];
  double firmware;
};


void loadStoredConfiguration();
void saveConfiguration();
String getUpdateUrl();

void requestRestart();
bool restartRequested();

void requestAutoUpdate();
bool autoUpdateRequested();

void requestLoadDefaults();
bool loadDefaultsRequested();
size_t  serializeConfigStatus(Print& output);
void requestWifiScan();
void requestReloadWifi();
bool reloadWifiRequested();
void normalize(String  &inputStr);
void updateConfig(JsonObject doc, bool persist);
boolean isValidNumber(const char *str);
struct Config& getAtualConfig();
void configPIN(uint8_t pin, uint8_t mode);
void writeToPIN(uint8_t pin, uint8_t val);
bool readPIN(uint8_t pin);
void serializeFile(const char* filename, Print &output);
#endif