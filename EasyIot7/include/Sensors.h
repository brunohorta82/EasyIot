#pragma once
#include <Arduino.h>
#include <vector>
#include "constants.h"
#include <ModbusMaster.h>
#include <Wire.h>
enum Sensorype
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

class Sensor
{
public:
  // CONFIG
  char uniqueId[24]{};
  char name[24] = {0};
  Sensorype type;
  String state;
  // MQTT
  char readTopic[128];

  // CLOUDIO
  char cloudIOreadTopic[128]{};

  // GPIOS INPUT
  std::vector<unsigned int> inputs;
  ModbusMaster *modbus;
  // CONTROL VARIABLES
  bool lastBinaryState = false;
  unsigned long delayRead = 1000ul;
  unsigned long lastRead = 0ul;
  bool initialized = false;
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
      return Family::CLIMATE;
    case PZEM_004T_V03:
    case HAN:
      return Family::ENERGY;
    }
    return Family::GENERIC;
  };
  String typeToText()
  {
    switch (type)
    {
    case LDR:
      return FeatureTypes::LDR;
    case DS18B20:
      return FeatureTypes::DS18B20;
    case SHT3x_SENSOR:
      return FeatureTypes::SHT3X;
    case DHT_11:
      return FeatureTypes::DHT_11;
    case DHT_21:
      return FeatureTypes::DHT_21;
    case DHT_22:
      return FeatureTypes::DHT_22;
    case PZEM_004T_V03:
      return FeatureTypes::PZEM_004T_V03;
    case HAN:
      return FeatureTypes::HAN;
    }
    return FeatureTypes::GENERIC;
  };
  const bool isInitialized()
  {
    if (!initialized)
    {
      initialized = true;
      return false;
    }
    return initialized;
  };
  void loop();
  void notifyState();
};
