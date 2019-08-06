#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Arduino.h>
#include <fauxmoESP.h>
#include "Config.h"
#include "Mqtt.h"

struct SensorT;
struct SwitchT;

void addToHaDiscovery(const SensorT &ss);
void removeFromHaDiscovery(const SwitchT &sw);

void addToHaDiscovery(const SwitchT &sw);
void removeFromHaDiscovery(const SensorT &ss);



#endif