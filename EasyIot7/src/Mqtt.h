#ifndef MQTT_H
#define MQTT_H
#include "config.h"
#include <PubSubClient.h>
#define AVAILABLE_PAYLOAD "online"
#define UNAVAILABLE_PAYLOAD "offline"
#define DEFAULT_MQTT_PORT 1883
void publishOnMqtt(String topic, String payload, bool retain);
void subscribeOnMqtt(String topic);
String getBaseTopic();
String getAvailableTopic();
void setupMQTT();
void loopMqtt();
void unsubscribeOnMqtt(String topic);
bool getMqttState();
#endif