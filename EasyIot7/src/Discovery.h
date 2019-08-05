#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <Arduino.h>
#include <fauxmoESP.h>
#include "Config.h"
#include "Mqtt.h"

struct SensorT;
struct SwitchT;

void removeFromHaDiscovery(SensorT *ss);
void removeFromAlexaDiscovery(SwitchT *sw);
void addToHaDiscovery(SensorT *ss);
void addToHaDiscovery(SwitchT *sw);
void removeFromHaDiscovery(SwitchT *sw);
void addToAlexaDiscovery(SwitchT *sw);


#endif