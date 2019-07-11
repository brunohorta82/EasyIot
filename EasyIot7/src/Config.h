#ifndef CONFIG_H
#define CONFIG_H
#define SYSTEM_TAG "[SYSTEM]"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> 
#include "WiFi.h"
#define HARDWARE "onofre"
#define FACTORY_TYPE "light"  //suported types [cover, light]
#define FIRMWARE_VERSION 7.0
#define FIRMWARE_VERSION_X "7x0"
#define UPDATE_URL "http://release.bhonofre.pt/release_"
#define CONFIG_FILENAME  "/config_bh"+String(HARDWARE)+".json"
#define NEW_ID "NEW"
#define NO_GPIO 99

//AP PASSWORD  
#define AP_SECRET "EasyIot@"



const String DEFAULT_NODE_ID = String(HARDWARE) +"-"+String(FACTORY_TYPE)+"-"+String(ESP.getChipId())+"-"+String(FIRMWARE_VERSION_X);
struct Config {
  char nodeId[32];
  char homeAssistantAutoDiscoveryPrefix[32];
  char mqttIpDns[32];
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
  char apName[16];
  char hostname[32];
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

void requestWifiScan();
void requestReloadWifi();
bool reloadWifiRequested();
void logger(String tag, String msg);
String normalize(String inputStr);
String getConfigStatus();
void updateConfig(JsonObject json, bool persist);
struct Config& getAtualConfig();

#endif