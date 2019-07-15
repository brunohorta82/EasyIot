#ifndef DISCOVERY_H
#define DISCOVERY_H
#include <Arduino.h>
#include <fauxmoESP.h>
#include "Switches.h"
#include "Config.h"
#include "Mqtt.h"

void startAlexaDiscovery(fauxmoESP &fauxmo);
void addToHaDiscovery(SwitchT sw);
void removeFromHaDiscovery(SwitchT sw);
#endif