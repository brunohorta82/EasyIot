#pragma once
#include <Arduino.h>
#include "Sensors.h"
#include "Actuatores.h"
bool homeAssistantOnline(String topic, String payload);
void initHomeAssistantDiscovery();
void removeFromHomeAssistant(String family, String uniqueId);

/** Publishes the irrigation entities: a closing time per zone, whether a cycle is
    running, a button per program, a stop button, and how many zones may water at
    once. Re-published when the schedule changes, because the buttons are the
    programs. Does nothing on a device with no valves. */
void createHaIrrigation();

/** The state behind those entities, on the local broker. */
void publishIrrigationHomeAssistantState();