#ifndef WEBSERVER_O_H
#define WEBSERVER_O_H
#include <Arduino.h>
void setupWebserverAsync();
void webserverServicesLoop();
void addSwitchToAlexa(const char *name);
void removeSwitchFromAlexa(const char *name);
void sendToServerEvents(const String &topic, const char *payload);
#endif
