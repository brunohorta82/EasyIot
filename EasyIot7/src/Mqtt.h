#ifndef MQTT_H
#define MQTT_H

#include "config.h"
#include <PubSubClient.h>
#define AVAILABLE_PAYLOAD "online"
#define UNAVAILABLE_PAYLOAD "offline"
void publishOnMqtt(String topic, String payload, bool retain);
void subscribeOnMqtt(String topic);
String getBaseTopic();
String getAvailableTopic();
void setupMQTT();
void loopMqtt();
#endif