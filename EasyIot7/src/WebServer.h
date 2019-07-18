#ifndef WEBSERVER_O_H
#define WEBSERVER_O_H
#include <Arduino.h>
#include "AsyncJson.h"
#include "Switches.h"
#include "ArduinoJson.h"
#include "StaticSite.h"
#include "StaticCss.h"
#include "StaticJs.h"
#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <fauxmoESP.h>
#include <Config.h>
#include "WiFi.h"


void setupWebserverAsync();
void webserverServicesLoop();
unsigned char addSwitchToAlexa(char* name);
void removeSwitchFromAlexa(char* name);
#endif
