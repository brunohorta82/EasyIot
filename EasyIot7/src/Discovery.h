#ifndef DISCOVERY_H
#define DISCOVERY_H
#include <Arduino.h>
#include <fauxmoESP.h>
#include "Config.h"
#include "Sensors.h"
#include "Switches.h"
#include "Mqtt.h"

void removeFromHaDiscovery(SensorT *ss);
void removeFromAlexaDiscovery(SwitchT *sw);
void addToHaDiscovery(SensorT *ss);
void addToHaDiscovery(SwitchT *sw);
void removeFromHaDiscovery(SwitchT *sw);
void addToAlexaDiscovery(SwitchT *sw);


#endif