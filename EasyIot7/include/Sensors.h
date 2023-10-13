#pragma once
#include <Arduino.h>
#include "FS.h"
#include "constants.h"
#include <ArduinoJson.h>
#include <vector>
class PZEM004T;
class PZEM004Tv30;
class DHT_nonblocking;
class DallasTemperature;
class Modbus;
enum SensorType
{
  LDR = 21,
  DS18B20 = 90,
  DHT_11 = 0,
  DHT_21 = 1,
  DHT_22 = 2,
  SHT3x_SENSOR = 91,
  PZEM_004T_V03 = 71,
  HAN = 80,
};

class SensorT
{
public:
  // CONFIG
  char uniqueId[24]{};
  char name[24] = {0};
  SensorType type;

  // MQTT
  char readTopic[128];
  String state;
  // CLOUDIO
  char cloudIOreadTopic[128]{};

  // GPIOS INPUT
  std::vector<unsigned int> inputs;

  // SENSORS
  DHT_nonblocking *dht;
  DallasTemperature *dallas;
  uint8_t oneWireSensorsCount = 0;
  PZEM004Tv30 *pzemv03;
  Modbus *pzemModbus;

  // CONTROL VARIABLES
  bool lastBinaryState = false;
  unsigned long delayRead = 1000ul;
  unsigned long lastRead = 0ul;
  String familyToText()
  {
    switch (type)
    {
    case LDR:
    case DS18B20:
    case SHT3x_SENSOR:
    case DHT_11:
    case DHT_21:
    case DHT_22:
      return constanstsFamilies::F_CLIMATE;
    case PZEM_004T_V03:
    case HAN:
      return constanstsFamilies::F_ENERGY;
    }
    return constanstsFamilies::F_GENERIC;
  };
  void setup();
  void loop();
  void notifyState();
};
