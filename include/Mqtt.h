#pragma once
#include <Arduino.h>

void publishOnMqtt(const char *topic, const char *payload, bool retain);
void subscribeOnMqtt(const char *topic);
void setupMQTT(bool forceDisconnect);
/** Close the local broker socket before a memory-heavy ESP8266 OTA. */
void disconnectMqttForUpdate();
void loopMqtt();
void unsubscribeOnMqtt(const char *topic);
bool mqttConnected();
