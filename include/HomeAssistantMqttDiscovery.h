#pragma once
#include <Arduino.h>
#include "Sensors.h"
#include "Actuatores.h"
bool homeAssistantOnline(String topic, String payload);
void initHomeAssistantDiscovery();
void addToHomeAssistant(Sensor &ss);
void addToHomeAssistant(Actuator &sw);
void removeFromHomeAssistant(Sensor &ss);
void removeFromHomeAssistant(Actuator &sw);