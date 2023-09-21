#ifndef CLOUDIO_H
#define CLOUDIO_H
#include "Arduino.h"
void connectToCloudIO();
bool cloudIOConnected();
void notifyStateToCloudIO(const char *topic, const char *state, size_t length);
#endif