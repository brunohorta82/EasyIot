#ifndef SENSORS_S_H
#define SENSORS_S_H
#include <Arduino.h>
#include "WebServer.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#define SENSORS_TAG "[SENSORS]"
#define SENSORS_CONFIG_FILENAME "sensors.json"

//SENSORS
#define TYPE_PIR 65
#define RCWL_0516 66
#define TYPE_LDR 21
#define TYPE_DS18B20 90
#define TYPE_REED_SWITCH 56
#define TYPE_DHT_11 DHT_TYPE_11 //0
#define TYPE_DHT_21 DHT_TYPE_21 //1
#define TYPE_DHT_22 DHT_TYPE_22 //2

#define NONE_CLASS "None"

#define SENSOR_FAMILY "sensor"
#define BINARY_SENSOR_FAMILY "binary_sensor"

struct SensorT
{
  char id[32]; //Generated from name without spaces and no special characters
  char name[24];
  char family[16]; //sensor, binary_sensor
  int type; //TYPE_PIR, TYPE_LDR, TYPE_DS18B20, TYPE_REED_SWITCH
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
void updateSensor(JsonObject doc, bool persist);
void removeSensor(String id, bool persist);
#endif
