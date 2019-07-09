#ifndef MQTT_H
#define MQTT_H
#define MQTT_TAG "[MQTT]"
#include "config.h"
#include <PubSubClient.h>
#define AVAILABLE_PAYLOAD "online"
#define UNAVAILABLE_PAYLOAD "offline"
void publishOnMqtt(String topic, String payload, bool retain);
void subscribeOnMqtt(String topic);
void setupMQTT();
void loopMqtt();
#endif