#ifndef DISCOVERY_H
#define DISCOVERY_H
#include <Arduino.h>
#include <fauxmoESP.h>
#include "Switches.h"
#include "Config.h"
#include "Mqtt.h"

void addToHaDiscovery(SwitchT *sw);
void removeFromHaDiscovery(SwitchT *sw);
void addToAlexaDiscovery(SwitchT *sw);
void removeFromAlexaDiscovery(SwitchT *sw);
#endif