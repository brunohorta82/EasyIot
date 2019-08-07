#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Arduino.h>
#include "Sensors.h"
#include "Switches.h"

void addToHaDiscovery(const SensorT &ss);
void removeFromHaDiscovery(const SwitchT &sw);

void addToHaDiscovery(const SwitchT &sw);
void removeFromHaDiscovery(const SensorT &ss);



#endif