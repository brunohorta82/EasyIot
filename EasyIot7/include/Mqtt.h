#ifndef MQTT_H
#define MQTT_H
#include <Arduino.h>

void publishOnMqtt(const char *topic, const char *payload, bool retain);
void subscribeOnMqtt(const char *topic);
String getBaseTopic();
String getAvailableTopic();
void setupMQTT();
void loopMqtt();
void unsubscribeOnMqtt(const char *topic);
bool mqttConnected();

#endif