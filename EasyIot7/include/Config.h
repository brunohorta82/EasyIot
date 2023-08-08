#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include "LittleFS.h"
#ifdef DEBUG_ONOFRE
#include <ArduinoLog.h>
#endif

struct Config
{
  double firmware = VERSION;
  char nodeId[32] = {};
  char mqttIpDns[40];
  char cloudIOUserName[40];
  char cloudIOUserPassword[64];
  char mqttUsername[32];
  int mqttPort = 1883;
  char mqttPassword[64];
  char mqttAvailableTopic[128];
  char wifiSSID[32];
  char wifiSecret[64];
  bool staticIp = false;
  char wifiIp[24];
  char wifiMask[24];
  char wifiGw[24];
  char apSecret[64];
  char apName[30];
  char chipId[24];
  char availableCloudIO[120];
  char mqttCloudRemoteActionsTopic[128];
  char apiUser[32];
  char apiPassword[64];
  void toJson(JsonVariant &root);
  Config &updateFromJson(JsonObject &root);
  void save();
  void load(File &file);
};

struct Config &getAtualConfig();
void load(Config &config);
void requestCloudIOSync();
bool cloudIOSync();

void requestWifiScan();
bool wifiScanRequested();

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
void generateId(String &id, const String &name, int familyCode, size_t maxSize);
long getTime();
String getChipId();

#endif