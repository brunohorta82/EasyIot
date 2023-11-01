#pragma once
#include <Arduino.h>
#include <vector>
#include "Constants.hpp"
#include <ModbusMaster.h>
#include <Wire.h>
enum SensorDriver
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
  SensorDriver driver;
  String state;
  // MQTT
  char readTopic[128];

  // CLOUDIO
  char cloudIOreadTopic[128]{};

  // GPIOS INPUT
  std::vector<unsigned int> inputs;

  // CONTROL VARIABLES
  bool lastBinaryState = false;
  unsigned long delayRead = 5000ul;
  unsigned long lastRead = 0ul;
  bool initialized = false;
  bool error = false;
  String familyToText()
  {
    switch (driver)
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
    return Family::NONE;
  };
  String driverToText()
  {
    switch (driver)
    {
    case LDR:
      return FeatureDrivers::LDR;
    case DS18B20:
      return FeatureDrivers::DS18B20;
    case SHT3x_SENSOR:
      return FeatureDrivers::SHT3X;
    case DHT_11:
      return FeatureDrivers::DHT_11;
    case DHT_21:
      return FeatureDrivers::DHT_21;
    case DHT_22:
      return FeatureDrivers::DHT_22;
    case PZEM_004T_V03:
      return FeatureDrivers::PZEM_004T_V03;
    case HAN:
      return FeatureDrivers::HAN;
    }
    return FeatureDrivers::GENERIC;
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
  const void reInit()
  {
    initialized = false;
  };
  void loop();
  void notifyState();
};
