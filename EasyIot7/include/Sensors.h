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
};

struct SensorT
{
  double firmware = VERSION;
  char id[32]; // Generated from name without spaces and no special characters
  char name[24];
  char family[16]; // sensor, binary_sensor
  SensorType type; // TYPE_PIR, TYPE_LDR, TYPE_DS18B20, TYPE_REED_SWITCH
  char deviceClass[32];
  // MQTT
  char readTopic[128];
  char mqttPayload[128];
  bool mqttRetain = true;
  bool emoncmsSupport = false;
  bool cloudIOSupport = true;
  bool haSupport = false;
  bool knxSupport = false;
  bool mqttSupport = false;

  // INPUT GPIO
  unsigned int primaryGpio = constantsConfig::noGPIO;
  unsigned int secondaryGpio = constantsConfig::noGPIO;
  unsigned int tertiaryGpio = constantsConfig::noGPIO;
  bool pullup = false; // USE INTERNAL RESISTOR

  // TEMPERATURE AND HUMIDITY SENSORS
  DHT_nonblocking *dht;
  DallasTemperature *dallas;
  uint8_t oneWireSensorsCount = 0;

  PZEM004T *pzem;
  PZEM004Tv30 *pzemv03;
  Modbus *pzemModbus;
  unsigned long delayRead = 0ul;
  unsigned long lastRead = 0ul;

  // CONTROL VARIABLES
  bool lastBinaryState = false;

  // READINGS
  float temperature = static_cast<float>(0);
  float humidity = static_cast<float>(0);
  char lastReading[200];
  // CLOUDIO
  char mqttCloudStateTopic[128];
  // CUSTOM PAYLOADS
  char payloadOff[10];
  char payloadOn[10];
  void load(File &file);
  void save(File &file) const;
  void updateFromJson(JsonObject doc);
  const char *commandTopic();
  const char *stateTopic();
};
class Sensors
{
public:
  std::vector<SensorT> items;
  void load(File &file);
  void save(File &file) const;
  void save();
  void resetAll();
  Sensors &remove(const char *id);
  Sensors &updateFromJson(const String &id, JsonObject doc);
  void toJson(JsonVariant &root);
};
struct Sensors &getAtualSensorsConfig();
void loop(Sensors &sensors);
void load(Sensors &sensors);
void resetSensors(Sensors &sensors);
void reloadSensors();
#endif
