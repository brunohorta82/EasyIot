#pragma once
#include <Arduino.h>
#include <vector>
#include "Constants.h"
#include <ModbusMaster.h>
#include <Wire.h>
enum SensorDriver
{
  INVALID_SENSOR = 999,
  DHT_11 = 111,
  DHT_21 = 121,
  DHT_22 = 122,
  PZEM_004T_V03 = 71,
  HAN = 80,
  LTR303X = 81,
  PIR = 82,
  RAIN = 83,
  DOOR = 84,
  WINDOW = 85,
  DS18B20 = 90,
  SHT4X = 91,
  VL53l0X = 92

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
  int lastBinaryState = -1;
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
    case RAIN:
      return Family::GARDEN;
    case PZEM_004T_V03:
    case HAN:
      return Family::ENERGY;
    case PIR:
    case DOOR:
    case WINDOW:
      return Family::SECURITY;
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
    case RAIN:
      return FeatureDrivers::RAIN;
    case DOOR:
      return FeatureDrivers::DOOR;
    case WINDOW:
      return FeatureDrivers::WINDOW;
    case PIR:
      return FeatureDrivers::PIR;
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
