#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "Mqtt.h"
#include <ArduinoLog.h>

struct Config
{
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
  void update(JsonObject doc, bool persist);
  void save(File &file) const;
  void load(File &file);
  size_t serializeToJson(Print &output);
};

struct Config &getAtualConfig();
void loadStoredConfiguration(Config &config);

void requestRestart();
bool restartRequested();

void requestAutoUpdate();
bool autoUpdateRequested();

void requestLoadDefaults();
bool loadDefaultsRequested();

bool reloadWifiRequested();

void normalize(String &inputStr);
boolean isValidNumber(const char *str);

void configPIN(uint8_t pin, uint8_t mode);
void writeToPIN(uint8_t pin, uint8_t val);
bool readPIN(uint8_t pin);
void generateId(String &id, const String &name, size_t maxSize);

#endif