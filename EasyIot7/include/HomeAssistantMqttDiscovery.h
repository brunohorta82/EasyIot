#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Arduino.h>
#include "Sensors.h"
#include "Switches.h"

void initHomeAssistantDiscovery();
void addToHomeAssistant(SensorT &ss);
void addToHomeAssistant(SwitchT &sw);
void removeFromHomeAssistant(SensorT &ss);
void removeFromHomeAssistant(SwitchT &sw);

#endif