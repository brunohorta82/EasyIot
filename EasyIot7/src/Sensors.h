#ifndef SENSORS_S_H
#define SENSORS_S_H

#include <Arduino.h>
#include "FS.h"
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <dht_nonblocking.h>

enum SensorType
{
  UNDEFINED = -1,
  PIR = 65,
  RCWL_0516 = 66,
  LDR = 21,
  DS18B20 = 90,
  REED_SWITCH = 56,
  DHT_11 = DHT_TYPE_11, //0
  DHT_21 = DHT_TYPE_21, //1
  DHT_22 = DHT_TYPE_22  //2
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
  bool mqttRetain = true;
  bool haSupport = true;

  //INPUT GPIO
  unsigned int primaryGpio;
  bool pullup; //USE INTERNAL RESISTOR

  //TEMPERATURE AND HUMIDITY SENSORS
  DHT_nonblocking *dht;
  DallasTemperature *dallas;

  unsigned long delayRead = 0ul;
  unsigned long lastRead = 0ul;

  //CONTROL VARIABLES
  bool lastBinaryState = false;

  //READINGS
  float temperature = static_cast<float>(0);
  float humidity = static_cast<float>(0);

  //CUSTOM PAYLOADS
  char payloadOff[10];
  char payloadOn[10];
  void load(File &file);
  void save(File &file) const;
  void updateFromJson(JsonObject doc);
};
struct Sensors
{
  std::vector<SensorT> items;
  void load(File &file);
  void save(File &file) const;
  bool remove(const char *id);
  size_t serializeToJson(Print &output);
};
struct Sensors &getAtualSensorsConfig();
void loop(Sensors &sensors);
void load(Sensors &sensors);
void remove(Sensors &sensors, const char *id);
void update(Sensors &sensors, const String &id, JsonObject doc);

#endif
