#pragma once
#include <Arduino.h>

void publishOnMqtt(const char *topic, const char *payload, bool retain);
void subscribeOnMqtt(const char *topic);
void setupMQTT(bool forceDisconnect);
void loopMqtt();
void unsubscribeOnMqtt(const char *topic);
bool mqttConnected();
