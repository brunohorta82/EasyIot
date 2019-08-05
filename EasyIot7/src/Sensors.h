#ifndef SENSORS_S_H
#define SENSORS_S_H

#include <Arduino.h>
#include "WebServer.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>

enum SensorType{
PIR = 65,
RCWL_0516 = 66,
LDR  = 21,
DS18B20 = 90,
REED_SWITCH = 56,
DHT_11 = DHT_TYPE_11, //0
DHT_21 = DHT_TYPE_21, //1
DHT_22 = DHT_TYPE_22 //2
};

struct SensorT
{
  char id[32]; //Generated from name without spaces and no special characters
  char name[24];
  char family[16]; //sensor, binary_sensor
  SensorType type; //TYPE_PIR, TYPE_LDR, TYPE_DS18B20, TYPE_REED_SWITCH
  char deviceClass[32];
  //MQTT
  char mqttStateTopic[128];
  char mqttPayload[128];
  bool mqttRetain;

  //INPUT GPIO
  unsigned int primaryGpio;
  bool pullup; //USE INTERNAL RESISTOR

  //TEMPERATURE AND HUMIDITY SENSORS
  DHT_nonblocking *dht;
  DallasTemperature *dallas;
  
  unsigned long delayRead;
  unsigned long lastRead;
  
  //CONTROL VARIABLES
  bool lastBinaryState;

  //READINGS
  float temperature;
  float humidity;

  //CUSTOM PAYLOADS
  char payloadOff[10];
  char payloadOn[10];
  
  
};
void saveSensors();
void loadStoredSensors();
void loopSensors();
void removeSensor(const char* id, bool persist);
void updateSensor(JsonObject doc, bool persist);

#endif
