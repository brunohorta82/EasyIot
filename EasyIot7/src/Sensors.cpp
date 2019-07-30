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

  logger(SENSORS_TAG, "Update Environment Sensors");
  String newId = doc.getMember("id").as<String>().equals(NEW_ID) ? String(String(ESP.getChipId()) + normalize(doc.getMember("name").as<String>())) : doc.getMember("id").as<String>();
  if (persist)
    removeSensor(doc.getMember("id").as<String>(), false);
  SensorT ss;
  strlcpy(ss.id, String(String(ESP.getChipId()) + normalize(doc.getMember("name").as<String>())).c_str(), sizeof(ss.id));
  strlcpy(ss.name, doc.getMember("name").as<String>().c_str(), sizeof(ss.name));
  strlcpy(ss.family, doc.getMember("family").as<String>().c_str(), sizeof(ss.family));
  strlcpy(ss.mqttStateTopic, doc.getMember("mqttStateTopic").as<String>().c_str(), sizeof(ss.mqttStateTopic));
  String baseTopic = getBaseTopic() + "/" + String(ss.family) + "/" + String(ss.id);
  addToHaDiscovery(&ss);
  ss.lastBinaryState = false;
  ss.lastRead = 0;
  ss.dht = NULL;
  ss.dallas = NULL;
  doc["id"] = String(ss.id);
  int gpio = doc["gpio"] | NO_GPIO;
  int type = doc["type"];
  switch (type)
  {
  case TYPE_LDR:
    ss.delayRead = 20000ul;
    break;
  case TYPE_PIR:
  case TYPE_REED_SWITCH:
    configPIN(gpio, gpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    strlcpy(ss.payloadOn, doc.getMember("payloadOff").as<String>().c_str(), sizeof(ss.payloadOff));
    strlcpy(ss.payloadOff, doc.getMember("payloadOff").as<String>().c_str(), sizeof(ss.payloadOff));
    break;
  case TYPE_DHT_11:
  case TYPE_DHT_21:
  case TYPE_DHT_22:
    ss.delayRead = 60000ul;
    ss.dht = new DHT_nonblocking(gpio, type);
    break;
  case TYPE_DS18B20:
    ss.dallas = new DallasTemperature(new OneWire(gpio));
    break;
  }

  sensors.push_back(ss);
  return doc;
}
void loadStoredSensors()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(SENSORS_CONFIG_FILENAME, "r+");
    int sensorsSize = 2; //TODO GET THIS VALUE FROM FILE
    const size_t CAPACITY = JSON_ARRAY_SIZE(sensorsSize + 1) + sensorsSize * JSON_OBJECT_SIZE(20) + 2000;
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
      const size_t CAPACITY = JSON_ARRAY_SIZE(sensors.size() + 1) + sensors.size() * JSON_OBJECT_SIZE(36) + 2200;
      ;
      DynamicJsonDocument doc(CAPACITY);
      for (unsigned int i = 0; i < sensors.size(); i++)
      {
        JsonObject sdoc = doc.createNestedObject();
        SensorT ss = sensors[i];
        sdoc["id"] = ss.id;
        sdoc["name"] = ss.name;
        sdoc["family"] = ss.family;
        sdoc["mqttStateTopic"] = String(ss.mqttStateTopic);
      }

      if (serializeJson(doc.as<JsonArray>(), file) == 0)
      {
        logger(SENSORS_TAG, "Failed to write Sensors Config into file");
      }
      else
      {
        logger(SENSORS_TAG, "Sensors Config stored.");
      }
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
    switch (sensors[i].type)
    {
    case TYPE_LDR:
    {
      if (ss->lastRead + ss->delayRead < millis())
      {
        ss->lastRead = millis();
        publishOnMqtt(ss->mqttStateTopic, String("{\"ldr_raw\":" + String(analogRead(A0)) + "}"), ss->mqttRetain);
      }
    }
    break;
    case TYPE_PIR:
    {
      bool binaryState = readPIN(ss->gpio);
      if (ss->lastBinaryState != binaryState)
      {
        publishOnMqtt(ss->mqttStateTopic, String("{\"binary_state\":" + String(binaryState) + "}"), ss->mqttRetain);
      }
    }
    break;
    case TYPE_REED_SWITCH:
    {
      bool binaryState = readPIN(ss->gpio);
      if (ss->lastBinaryState != binaryState)
      {
        publishOnMqtt(ss->mqttStateTopic, String("{\"binary_state\":" + String(binaryState) + "}"), ss->mqttRetain);
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
      }
    }
    break;
    }
  }
}
