#pragma once
#include <Arduino.h>

void publishOnMqtt(const char *topic, String payload, bool retain);
void subscribeOnMqtt(const char *topic);
void setupMQTT();
void loopMqtt();
void unsubscribeOnMqtt(const char *topic);
bool mqttConnected();
