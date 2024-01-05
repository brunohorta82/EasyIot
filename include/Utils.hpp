#pragma once
#include "Arduino.h"
#include "Constants.h"
void normalize(String &inputStr);
boolean isValidNumber(const char *str);
void configPIN(uint8_t pin, uint8_t mode);
void writeToPIN(uint8_t pin, uint8_t val);
bool readPIN(uint8_t pin);
int rssiToWiFiQuality(int rssi);