#ifndef CLOUDIO_H
#define CLOUDIO_H
#include "Arduino.h"
void connectToCloudIO();
void startCloudIOWatchdog();
bool cloudIOConnected();
void notifyStateToCloudIO(const char *topic, const char *state);
#endif