#pragma once
#include <Arduino.h>
#include <vector>
#include "Constants.h"
#include <ModbusMaster.h>
#include <Wire.h>
enum SensorDriver
{
  INVALID_SENSOR = 999,
  DS18B20 = 90,
  DHT_11 = 0,
  DHT_21 = 1,
  DHT_22 = 2,
  SHT4X = 91,
  LTR303X = 81,
  PZEM_004T_V03 = 71,
  HAN = 80,
};

class Sensor
{
public:
  // CONFIG
  char uniqueId[24]{};

  char name[24] = {0};
  int hwAddress{0x10};
  SensorDriver driver;
  String state = "";
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
  unsigned long lastErrorTimestamp = 0ul;
  int id = 0;
  String
  familyToText()
  {
    switch (driver)
    {
    case DS18B20:
    case SHT4X:
    case DHT_11:
    case DHT_21:
    case DHT_22:
      return Family::CLIMATE;
    case PZEM_004T_V03:
    case HAN:
      return Family::ENERGY;
    case LTR303X:
      return Family::LEVEL_METER;
    }
    return Family::NONE;
  };
  String driverToText()
  {
    switch (driver)
    {
    case DS18B20:
      return FeatureDrivers::DS18B20;
    case LTR303X:
      return FeatureDrivers::LTR303;
    case SHT4X:
      return FeatureDrivers::SHT4X;
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
    return FeatureDrivers::INVALID;
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

  const void clearError()
  {
    error = false;
    lastErrorTimestamp = 0ul;
  };
  const void setError()
  {
    error = true;
    lastErrorTimestamp = millis();
  };
  void loop();
  void notifyState();
};
