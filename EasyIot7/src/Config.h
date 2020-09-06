#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "LittleFS.h"
#ifdef DEBUG
#include <ArduinoLog.h>
#endif

struct Config
{
  double firmware = VERSION;
  char nodeId[32];
  char homeAssistantAutoDiscoveryPrefix[32];
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
  long configTime = 0ul;
  char chipId[24];
  char apiUser[32];
  char apiPassword[64];
  char emoncmsServer[80];
  char emoncmsPath[20];
  char emoncmsApikey[80];
  char emoncmsFingerprint[100];
  char availableCloudIO[120];

  uint8_t knxArea = 0;
  uint8_t knxLine = 0;
  uint8_t knxMember = 0;
  long connectedOn = -1;
  char mqttCloudRemoteActionsTopic[128];
  void save();
  void toJson(JsonVariant &root);
  Config &updateFromJson(JsonObject &root);
  void save(File &file) const;
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
void loopTime();
long getTime();
#endif