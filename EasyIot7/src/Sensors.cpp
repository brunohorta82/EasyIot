#include "Discovery.h"

std::vector<SensorT> sensors;

void removeSensor(String id, bool persist)
{
  logger(SENSORS_TAG, "Remove sensor " + id);

  unsigned int del = NO_GPIO;
  for (unsigned int i = 0; i < sensors.size(); i++)
  {
    SensorT sStored = sensors[i];
    if (strcmp(id.c_str(), sStored.id) == 0)
    {
      removeFromHaDiscovery(&sStored);
      del = i;
    }
  }
  if (del != NO_GPIO)
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
JsonObject updateSensor(JsonObject doc, bool persist)
{
  serializeJson(doc, Serial);
  logger(SENSORS_TAG, "Update Environment Sensors");
  int type = doc["type"] | -1;
  if (type < 0)
    return doc;
  String n_name = doc["name"] ;
  normalize(n_name);
  String newId = doc.getMember("id").as<String>().equals(NEW_ID) ? String(String(ESP.getChipId()) + n_name) : doc.getMember("id").as<String>();
  if (persist)
    removeSensor(doc.getMember("id").as<String>(), false);
  SensorT ss;
  strlcpy(ss.id, String(String(ESP.getChipId()) + n_name).c_str(), sizeof(ss.id));
  strlcpy(ss.name, doc.getMember("name").as<String>().c_str(), sizeof(ss.name));
  strlcpy(ss.deviceClass, doc["deviceClass"] | NONE_CLASS, sizeof(ss.deviceClass));
  ss.type = type;

  addToHaDiscovery(&ss);
  ss.lastBinaryState = false;
  ss.lastRead = 0;
  ss.dht = NULL;
  ss.dallas = NULL;
  doc["id"] = String(ss.id);
  int primaryGpio = doc["primaryGpio"] | NO_GPIO;
  ss.primaryGpio = primaryGpio;
  ss.delayRead = doc["delayRead"];
  ss.mqttRetain = doc["mqttRetain"] | true;
  switch (type)
  {
  case TYPE_LDR:
    strlcpy(ss.family, SENSOR_FAMILY, sizeof(ss.family));
    break;
  case TYPE_PIR:
  case RCWL_0516:
   strlcpy(ss.family, BINARY_SENSOR_FAMILY, sizeof(ss.family));
    configPIN(primaryGpio, INPUT);
    strlcpy(ss.payloadOn, doc["payloadOn"] | "ON", sizeof(ss.payloadOff));
    strlcpy(ss.payloadOff, doc["payloadOff"] | "OFF", sizeof(ss.payloadOff));
  break;
  case TYPE_REED_SWITCH:
    strlcpy(ss.family, BINARY_SENSOR_FAMILY, sizeof(ss.family));
    configPIN(primaryGpio, primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    strlcpy(ss.payloadOn, doc["payloadOn"] | "ON", sizeof(ss.payloadOff));
    strlcpy(ss.payloadOff, doc["payloadOff"] | "OFF", sizeof(ss.payloadOff));
    break;
  case TYPE_DHT_11:
  case TYPE_DHT_21:
  case TYPE_DHT_22:
    strlcpy(ss.family, SENSOR_FAMILY, sizeof(ss.family));
    ss.dht = new DHT_nonblocking(primaryGpio, type);
    break;
  case TYPE_DS18B20:
    strlcpy(ss.family, SENSOR_FAMILY, sizeof(ss.family));
    ss.dallas = new DallasTemperature(new OneWire(primaryGpio));
    break;
  }
  String baseTopic = getBaseTopic() + "/" + String(ss.family) + "/" + String(ss.id);
  doc["mqttStateTopic"] = String(baseTopic + "/state");
  strlcpy(ss.mqttStateTopic, String(baseTopic + "/state").c_str(), sizeof(ss.mqttStateTopic));
  sensors.push_back(ss);
  saveSensors();
  return doc;
}
void loadStoredSensors()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(SENSORS_CONFIG_FILENAME, "r+");
    int sensorsSize = 2; //TODO GET THIS VALUE FROM FILE
    const size_t CAPACITY = JSON_ARRAY_SIZE(sensorsSize + 1) + sensorsSize * JSON_OBJECT_SIZE(11) + 500;
    DynamicJsonDocument doc(CAPACITY);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      file.close();
      file = SPIFFS.open(SENSORS_CONFIG_FILENAME, "w+");
      logger(SENSORS_TAG, "Default sensors loaded.");
      file.print(String("[]").c_str());
      file.close();
      SPIFFS.end();
      loadStoredSensors();
    }
    else
    {
      logger(SENSORS_TAG, "Stored ssitches loaded.");
    }
    file.close();
    JsonArray ar = doc.as<JsonArray>();
    serializeJson(ar, Serial);
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
    File file = SPIFFS.open(SENSORS_CONFIG_FILENAME, "w+");
    if (!file)
    {
      logger(SWITCHES_TAG, "Open Sensors config file Error!");
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
        sdoc["type"] = ss.type;
        sdoc["deviceClass"] = ss.deviceClass;
        sdoc["mqttRetain"] = ss.mqttRetain;
      }

      if (serializeJson(doc.as<JsonArray>(), file) == 0)
      {
        logger(SENSORS_TAG, "Failed to write Sensors Config into file");
      }
      else
      {
        logger(SENSORS_TAG, "Sensors Config stored.");
      }
      serializeJson(doc, Serial);
    }
    file.close();
  }
  SPIFFS.end();
}
String getSensorsConfigStatus()
{
  String object = "";
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(SENSORS_CONFIG_FILENAME, "r+");

    if (!file)
    {
      return "[]";
    }
    while (file.available())
    {
      object += (char)file.read();
    }
    file.close();
  }
  SPIFFS.end();
  if (object.equals("") || object.equals("null"))
    return "[]";
  return object;
}

void loopSensors()
{
  for (unsigned int i = 0; i < sensors.size(); i++)
  {
    SensorT *ss = &sensors[i];
    if (ss->primaryGpio == NO_GPIO)
      continue;
    switch (sensors[i].type)
    {
    case TYPE_LDR:
    {
      if (ss->lastRead + ss->delayRead < millis())
      {
        ss->lastRead = millis();
        publishOnMqtt(ss->mqttStateTopic, String("{\"ldr_raw\":" + String(analogRead(ss->primaryGpio)) + "}"), ss->mqttRetain);
        logger(SENSORS_TAG, String("{\"ldr_raw\":" + String(analogRead(ss->primaryGpio)) + "}"));
      }
    }
    break;

    case TYPE_PIR:
    case TYPE_REED_SWITCH:
    case RCWL_0516:
    {
      bool binaryState = readPIN(ss->primaryGpio);
      if (ss->lastBinaryState != binaryState)
      {
        ss->lastBinaryState = binaryState;
        publishOnMqtt(ss->mqttStateTopic, String("{\"binary_state\":" + String(binaryState) + "}"), ss->mqttRetain);
        logger(SENSORS_TAG, String("{\"binary_state\":" + String(binaryState) + "}"));
      }
    }
    break;

    case TYPE_DHT_11:
    case TYPE_DHT_21:
    case TYPE_DHT_22:
    {

      if (ss->dht->measure(&ss->temperature, &ss->humidity) == true)
      {

        if (ss->lastRead + ss->delayRead < millis())
        {
          ss->lastRead = millis();
          publishOnMqtt(ss->mqttStateTopic, String("{\"temperature\":" + String(ss->temperature) + ",\"humidity\":" + String(ss->humidity) + "}"), ss->mqttRetain);
          logger(SENSORS_TAG, String("{\"temperature\":" + String(ss->temperature) + ",\"humidity\":" + String(ss->humidity) + "}"));
        }
      }
    }
    break;
    case TYPE_DS18B20:
    {
      if (ss->lastRead + ss->delayRead < millis())
      {
        ss->dallas->begin();
        ss->dallas->requestTemperatures();
        ss->lastRead = millis();
        ss->temperature = ss->dallas->getTempCByIndex(0);
        publishOnMqtt(ss->mqttStateTopic, String("{\"temperature\":" + String(ss->temperature) + "}"), ss->mqttRetain);
        logger(SENSORS_TAG, String("{\"temperature\":" + String(ss->temperature) + "}"));
      }
    }
    break;
    }
  }
}
