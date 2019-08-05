#include "Sensors.h"
#include "Discovery.h"
#include "constants.h"

static std::vector<SensorT> sensors;

void removeSensor(const char* id, bool persist)
{
  Log.notice("%s Remove id: %s" CR, tags::sensors , id);

  unsigned int del = constantsConfig::noGPIO;
  for (unsigned int i = 0; i < sensors.size(); i++)
  {
    SensorT sStored = sensors[i];
    if (strcmp(id, sStored.id) == 0)
    {
      removeFromHaDiscovery(&sStored);
      del = i;
    }
  }
  if (del != constantsConfig::noGPIO)
  {
    sensors.erase(sensors.begin() + del);
  }
  
  if (persist)
  {
    saveSensors();
  }
}
void initSensorsHaDiscovery()
{
  for (unsigned int i = 0; i < sensors.size(); i++)
  {
    addToHaDiscovery(&sensors[i]);
    publishOnMqtt(sensors[i].mqttStateTopic, sensors[i].mqttPayload, sensors[i].mqttRetain);
  }
}
void updateSensor(JsonObject doc, bool persist)
{
  Log.notice("%s Update Environment" CR, tags::sensors);
  int sensorType = doc["type"] | -1;
  if(sensorType  < 0) return;
  SensorType type = static_cast<SensorType>(sensorType);
  
  String n_name = doc["name"] ;
  normalize(n_name);
  String newId = doc.getMember("id").as<String>().equals(constantsConfig::newID) ? String(String(ESP.getChipId()) + n_name) : doc.getMember("id").as<String>();
  if (persist)
    removeSensor(doc.getMember("id"), false);
  SensorT ss;
  strlcpy(ss.id, String(String(ESP.getChipId()) + n_name).c_str(), sizeof(ss.id));
  strlcpy(ss.name, doc.getMember("name").as<String>().c_str(), sizeof(ss.name));
  strlcpy(ss.deviceClass, doc["deviceClass"] | constantsSensor::noneClass, sizeof(ss.deviceClass));
  ss.type = type;

  addToHaDiscovery(&ss);
  ss.lastBinaryState = false;
  ss.lastRead = 0;
  ss.dht = NULL;
  ss.dallas = NULL;
  doc["id"] = String(ss.id);
  int primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  ss.primaryGpio = primaryGpio;
  ss.delayRead = doc["delayRead"];
  ss.mqttRetain = doc["mqttRetain"] | true;
  switch (type)
  {
  case LDR:
    strlcpy(ss.family, constantsSensor::familySensor, sizeof(ss.family));
    break;
  case PIR:
  case RCWL_0516:
   strlcpy(ss.family, constantsSensor::binarySensorFamily, sizeof(ss.family));
    configPIN(primaryGpio, INPUT);
    strlcpy(ss.payloadOn, doc["payloadOn"] | "ON", sizeof(ss.payloadOff));
    strlcpy(ss.payloadOff, doc["payloadOff"] | "OFF", sizeof(ss.payloadOff));
  break;
  case REED_SWITCH:
    strlcpy(ss.family, constantsSensor::binarySensorFamily, sizeof(ss.family));
    configPIN(primaryGpio, primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    strlcpy(ss.payloadOn, doc["payloadOn"] | "ON", sizeof(ss.payloadOff));
    strlcpy(ss.payloadOff, doc["payloadOff"] | "OFF", sizeof(ss.payloadOff));
    break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
    strlcpy(ss.family, constantsSensor::familySensor, sizeof(ss.family));
    ss.dht = new DHT_nonblocking(primaryGpio, type);
    break;
  case DS18B20:
    strlcpy(ss.family, constantsSensor::familySensor, sizeof(ss.family));
    ss.dallas = new DallasTemperature(new OneWire(primaryGpio));
    break;
  }
  String baseTopic = getBaseTopic() + "/" + String(ss.family) + "/" + String(ss.id);
  doc["mqttStateTopic"] = String(baseTopic + "/state");
  strlcpy(ss.mqttStateTopic, String(baseTopic + "/state").c_str(), sizeof(ss.mqttStateTopic));
  sensors.push_back(ss);
  saveSensors();
  
}

void loadStoredSensors()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(configFilenames::sensors, "r+");
    
    int sensorsSize = 2; //TODO GET THIS VALUE FROM FILE
    const size_t CAPACITY = JSON_ARRAY_SIZE(sensorsSize + 1) + sensorsSize * JSON_OBJECT_SIZE(11) + 500;
    DynamicJsonDocument doc(CAPACITY);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      file.close();
      file = SPIFFS.open(configFilenames::sensors, "w+");
      Log.warning("%s Default values was loaded." CR, tags::sensors);
      file.print(String("[]").c_str());
      file.close();
      SPIFFS.end();
      loadStoredSensors();
    }
    else
    {
      Log.notice("%s Stored values was loaded." CR, tags::sensors);
    }
    file.close();
    JsonArray ar = doc.as<JsonArray>();
    for (JsonVariant ss : ar)
    {
      updateSensor(ss.as<JsonObject>(), error);
    }
  }
  SPIFFS.end();
}
void saveSensors()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(configFilenames::sensors, "w+");
    if (!file)
    {
      Log.error("%s Open Sensors file Error!" CR, tags::sensors);
    }
    else
    {
      const size_t CAPACITY = JSON_ARRAY_SIZE(sensors.size() + 1) + sensors.size() * JSON_OBJECT_SIZE(10) + 500;
      DynamicJsonDocument doc(CAPACITY);
      for (unsigned int i = 0; i < sensors.size(); i++)
      {
        JsonObject sdoc = doc.createNestedObject();
        SensorT ss = sensors[i];
        sdoc["id"] = ss.id;
        sdoc["name"] = ss.name;
        sdoc["family"] = ss.family;
        sdoc["mqttStateTopic"] = String(ss.mqttStateTopic);
        sdoc["delayRead"] = ss.delayRead;
        sdoc["primaryGpio"] = ss.primaryGpio;
        sdoc["type"] = static_cast<int>( ss.type);
        sdoc["deviceClass"] = ss.deviceClass;
        sdoc["mqttRetain"] = ss.mqttRetain;
      }

      if (serializeJson(doc.as<JsonArray>(), file) == 0)
      {
        Log.error("%s Failed to write Sensors Config into file" CR, tags::sensors);
      }
      else
      {
        Log.notice("%s Sensors Config stored." CR, tags::sensors);
      }
    }
    file.close();
  }
  SPIFFS.end();
}

void loopSensors()
{
  for (unsigned int i = 0; i < sensors.size(); i++)
  {
    SensorT *ss = &sensors[i];
    if (ss->primaryGpio == constantsConfig::noGPIO)
      continue;
    switch (sensors[i].type)
    {
    case LDR:
    {
      if (ss->lastRead + ss->delayRead < millis())
      {
        ss->lastRead = millis();
       int ldrRaw =  analogRead(ss->primaryGpio);
        String analogReadAsString = String(ldrRaw);
        publishOnMqtt(ss->mqttStateTopic, String("{\"ldr_raw\":" + analogReadAsString + "}").c_str(), ss->mqttRetain);
        Log.notice("%s {\"ldr_raw\": %d }" CR , tags::mqtt , ldrRaw);
      }
    }
    break;

    case PIR:
    case REED_SWITCH:
    case RCWL_0516:
    {
      bool binaryState = readPIN(ss->primaryGpio);
      if (ss->lastBinaryState != binaryState)
      {
        ss->lastBinaryState = binaryState;
        String binaryStateAsString = String(binaryState);
        publishOnMqtt(ss->mqttStateTopic, String("{\"binary_state\":" + binaryStateAsString + "}").c_str(), ss->mqttRetain);
        Log.notice("%s {\"binary_state\": %t }" CR , tags::sensors, binaryState);
      }
    }
    break;

    case DHT_11:
    case DHT_21:
    case DHT_22:
    {

      if (ss->dht->measure(&ss->temperature, &ss->humidity) == true)
      {

        if (ss->lastRead + ss->delayRead < millis())
        {
          ss->lastRead = millis();
          String temperatureAsString = String(ss->temperature);
          String humidityAsString = String(ss->humidity);
          publishOnMqtt(ss->mqttStateTopic, String("{\"temperature\":" + temperatureAsString + ",\"humidity\":" + humidityAsString + "}").c_str(), ss->mqttRetain);
          
          Log.notice("%s {\"temperature\": %F ,\"humidity\": %F" CR, tags::sensors , ss->temperature, ss->humidity);
        }
      }
    }
    break;
    case DS18B20:
    {
      if (ss->lastRead + ss->delayRead < millis())
      {
        ss->dallas->begin();
        ss->dallas->requestTemperatures();
        ss->lastRead = millis();
        ss->temperature = ss->dallas->getTempCByIndex(0);
          String temperatureAsString = String(ss->temperature);
        publishOnMqtt(ss->mqttStateTopic, String("{\"temperature\":" + temperatureAsString + "}").c_str(), ss->mqttRetain);
        Log.notice("%s {\"temperature\": %F " CR, tags::sensors , ss->temperature);
      }
    }
    break;
    }
  }
}
