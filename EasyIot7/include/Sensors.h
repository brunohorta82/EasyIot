#ifndef SENSORS_S_H
#define SENSORS_S_H

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
  UNDEFINED = -1,
  PIR = 65,
  RCWL_0516 = 66,
  LDR = 21, // Analog signal, ex: A0
  DS18B20 = 90,
  REED_SWITCH_NC = 56,
  REED_SWITCH_NO = 57,
  DHT_11 = 0,
  DHT_21 = 1,
  DHT_22 = 2,
  PZEM_004T = 70,     // primaryGPIO is RX, secondaryGPIO is TX and tertiaryGPIO is CurrentDetection
  PZEM_004T_V03 = 71, // primaryGPIO is RX, secondaryGPIO is TX and tertiaryGPIO is CurrentDetection
  HAN = 80,
};

struct SensorT
{
  char uniqueId[24]{}; // Generated from name without spaces and no special characters
  char name[24];
  SensorType interface; // TYPE_PIR, TYPE_LDR, TYPE_DS18B20, TYPE_REED_SWITCH
  char family[32];
  // MQTT
  char readTopic[128];
  String state;

  bool haSupport = false;

  // INPUT GPIO
  unsigned int primaryGpio = constantsConfig::noGPIO;
  unsigned int secondaryGpio = constantsConfig::noGPIO;
  unsigned int tertiaryGpio = constantsConfig::noGPIO;

  // TEMPERATURE AND HUMIDITY SENSORS
  DHT_nonblocking *dht;
  DallasTemperature *dallas;
  uint8_t oneWireSensorsCount = 0;

  PZEM004T *pzem;
  PZEM004Tv30 *pzemv03;
  Modbus *pzemModbus;
  unsigned long delayRead = 1000ul;
  unsigned long lastRead = 0ul;
  // CONTROL VARIABLES
  bool lastBinaryState = false;
  // CLOUDIO
  char cloudIOreadTopic[128]{};
  void setup();
  void loop();
  void notifyState();
};

#endif
