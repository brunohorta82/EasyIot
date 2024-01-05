#pragma once
#include "Constants.h"
#include "Actuatores.h"
#include "Sensors.h"
void templateSelect(enum Template _template);
void preparePzem(String name, unsigned int tx, unsigned int rx, int hwAddress);
int prepareNewFeature(String name, unsigned int input1, unsigned int input2, int driverCode);
void prepareActuator(String name, unsigned int output, unsigned int input, ActuatorDriver driver, ActuatorControlType type);
int prepareVirtualSwitch(String name, unsigned int input1, unsigned int input2, ActuatorDriver driver);
void prepareCover(String name, unsigned int outputDown, unsigned int outputUp, unsigned int inputDown, unsigned int inputUp, ActuatorDriver driver, ActuatorControlType type);
void prepareGarage(String name, unsigned int gateOne, unsigned int gateTwo, unsigned int openCloseSensor, unsigned int pushSwitch, ActuatorDriver driver, ActuatorControlType type);
void prepareSHT4X(int hwAddress);
void prepareLTR303(int hwAddress);
