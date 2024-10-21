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
  PZEM_004T_V01 = 72,
  HAN = 80,
  LTR303X = 81,
  PIR = 82,
  RAIN = 83,
  DOOR = 84,
  WINDOW = 85,
  DS18B20 = 90,
  SHT4X = 91,
  TMF882X = 92,
  HCSR04 = 93,
  LD2410 = 94

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
  int errorCounter = 0;
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
    case PZEM_004T_V01:
    case HAN:
      return Family::ENERGY;
    case PIR:
    case DOOR:
    case WINDOW:
    case LD2410:
      return Family::SECURITY;
    case LTR303X:
    case HCSR04:
      return Family::LEVEL_METER;
    case INVALID_SENSOR:
      return Family::NONE;
    default:
      return Family::NONE;
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
    case PZEM_004T_V01:
      return FeatureDrivers::PZEM_004T_V01;
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
    case HCSR04:
      return FeatureDrivers::HCSR04;
    case LD2410:
      return FeatureDrivers::LD2410;
    case TMF882X:
      return FeatureDrivers::TMF882X;
    case INVALID_SENSOR:
      return FeatureDrivers::INVALID;
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
    if (error)
      return;
    error = true;
    lastErrorTimestamp = millis();
    errorCounter++;
  };
  void loop();
  void notifyState();
};
