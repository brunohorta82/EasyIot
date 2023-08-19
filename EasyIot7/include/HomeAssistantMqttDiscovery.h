#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Arduino.h>
#include "Sensors.h"
#include "Actuatores.h"

void initHomeAssistantDiscovery();
void addToHomeAssistant(SensorT &ss);
void addToHomeAssistant(ActuatorT &sw);
void removeFromHomeAssistant(SensorT &ss);
void removeFromHomeAssistant(ActuatorT &sw);

#endif