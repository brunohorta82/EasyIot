#ifndef CLOUDIO_H
#define CLOUDIO_H
#include "Arduino.h"
void connectToCloudIO();
void startCloudIOWatchdog();
bool cloudIOConnected();
void notifyStateToCloudIO(const char *topic, const char *state);
/** Publishes the whole irrigation picture — schedule, running cycle, and the
    countdown of every open valve — as one retained message. Cheap enough to call
    on every valve change; there is nothing to publish while the cloud is down. */
void notifyIrrigationToCloudIO();
/** The same picture to both brokers: the cloud for the apps, the local one for
    Home Assistant. One call, so a caller cannot update half the world. */
void notifyIrrigation();
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
