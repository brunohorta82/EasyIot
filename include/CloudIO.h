#ifndef CLOUDIO_H
#define CLOUDIO_H
#include "Arduino.h"
void connectToCloudIO();
void startCloudIOWatchdog();
bool cloudIOConnected();
void notifyStateToCloudIO(const char *topic, const char *state);
// Service callback-owned flags from the main execution context. Neither the
// Ticker nor AsyncMqttClient callbacks perform logging, configuration access,
// or connection lifecycle work directly.
void serviceCloudIOWatchdog();
void serviceCloudIOMqtt();
// Drain MQTT commands from the main execution context. AsyncMqttClient invokes
// its callback outside the feature loop on ESP32, so callbacks only enqueue and
// this function performs the actual configuration/feature access.
void drainCloudIOCommands();
#endif
