#pragma once
#include <Arduino.h>
#include "Sensors.h"
#include "Actuatores.h"
bool homeAssistantOnline(String topic, String payload);
void initHomeAssistantDiscovery();
void removeFromHomeAssistant(String family, String uniqueId);