#ifndef SENSORS_S_H
#define SENSORS_S_H

#include <Arduino.h>
#include "FS.h"
#include <ArduinoJson.h>

class PZEM004T;
class PZEM004Tv30;
class DHT_nonblocking;
class DallasTemperature;
enum SensorType
{
  UNDEFINED = -1,
  PIR = 65,
  RCWL_0516 = 66,
  LDR = 21, //Analog signal, ex: A0
  DS18B20 = 90,
  REED_SWITCH_NC = 56,
  REED_SWITCH_NO = 57,
  DHT_11 = 0,
  DHT_21 = 1,
  DHT_22 = 2,
  PZEM_004T = 70,    // primaryGPIO is RX, secondaryGPIO is TX and tertiaryGPIO is CurrentDetection
  PZEM_004T_V03 = 71 // primaryGPIO is RX, secondaryGPIO is TX and tertiaryGPIO is CurrentDetection
};

struct SensorT
{
  double firmware;
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
  bool emoncmsSupport = false;
  //KNX
  uint8_t knxLevelOne;
  uint8_t knxLevelTwo;
  uint8_t knxLevelThree;
  //INPUT GPIO
  unsigned int primaryGpio;
  unsigned int secondaryGpio;
  unsigned int tertiaryGpio;
  bool pullup; //USE INTERNAL RESISTOR

  //TEMPERATURE AND HUMIDITY SENSORS
  DHT_nonblocking *dht;
  DallasTemperature *dallas;
  uint8_t busSensorCount = 0;

  PZEM004T *pzem;
  PZEM004Tv30 *pzemv03;

  unsigned long delayRead = 0ul;
  unsigned long lastRead = 0ul;

  //CONTROL VARIABLES
  bool lastBinaryState = false;

  //READINGS
  float temperature = static_cast<float>(0);
  float humidity = static_cast<float>(0);
  bool reading = false;
//CLOUDIO
 char mqttCloudStateTopic[128];
   bool cloudIOSupport = true;
  //CUSTOM PAYLOADS
  char payloadOff[10];
  char payloadOn[10];
  void load(File &file);
  void save(File &file) const;
  void updateFromJson(JsonObject doc);
  void reloadMqttTopics();
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
void reloadSensors();
#endif
