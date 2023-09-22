#ifndef CLOUDIO_H
#define CLOUDIO_H
#include "Arduino.h"
#include "Switches.h"
void connectToCloudIO();
void notifyStateToCloudIO(SwitchT *s);
void notifyStateToCloudIO(const String topic, const String state);
#endif