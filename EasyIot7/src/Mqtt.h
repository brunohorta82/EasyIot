#ifndef MQTT_H
#define MQTT_H
#include "config.h"
#include <PubSubClient.h>
void publishOnMqtt(String topic, String payload, bool retain);
void setupMQTT();
void loopMqtt();
#endif