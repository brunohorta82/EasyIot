#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Arduino.h>
#include "Sensors.h"
#include "Switches.h"

void initHaDiscovery(const Switches &switches);
void addToHaDiscovery(const SensorT &ss);
void removeFromHaDiscovery(const SensorT &ss);

void initHaDiscovery(const Sensors &sensors);
void removeFromHaDiscovery(const SwitchT &sw);
void addToHaDiscovery(const SwitchT &sw);

#endif