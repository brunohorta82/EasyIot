#ifndef SENSORS_S_H
#define SENSORS_S_H
#include <Arduino.h>
#include "Config.h"
#include "Discovery.h"
#include <dht_nonblocking.h>
#define SENSORS_TAG "[SENSORS]"
#define SENSORS_CONFIG_FILENAME  "sensors.json"
//FUNTIONS
#define TEMPERATURE_FUNCTION 1
#define HUMIDITY_FUNCTION 2
#define MOTION_FUNCTION 4
#define OPENING 5
#define ANALOG_FUNCTION 7


//SENSORS
#define PIR_TYPE 65
#define LDR_TYPE 21
#define DS18B20_TYPE 90
#define REED_SWITCH_TYPE 56

struct SensorT{
 char id[32]; //Generated from name without spaces and no special characters
  int type;
 char name[24];
   char family[16]; //sensor, binary_sensor
  char mqttStateTopic[128];
  DHT_nonblocking *dht;
  //DallasTemperature *dallas;
  //DebounceEvent *binaryDebounce;
  int gpio;
  long delayRead;
  long lastRead;
  float temperature;
  float humidity;
  char mqttPayload[128];
  bool mqttRetain;
};
#endif
