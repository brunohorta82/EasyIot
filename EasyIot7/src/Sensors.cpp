#include "Sensors.h"
#include "Discovery.h"
#include "constants.h"
#include "WebServer.h"
#include "Config.h"
#include "Mqtt.h"
struct Sensors &getAtualSensorsConfig()
{
  static Sensors sensors;
  return sensors;
}
void Sensors::load(File &file)
{
  auto n_items = items.size();
  file.read((uint8_t *)&n_items, sizeof(n_items));
  items.clear();
  items.resize(n_items);
  for (auto &item : items)
  {
    item.load(file);
    switch (item.type)
    {
    case UNDEFINED:
      Log.error("%s Invalid type" CR, tags::sensors);
      continue;
      break;
    case LDR:
      //nothing to do
      break;
    case PIR:
    case RCWL_0516:
      configPIN(item.primaryGpio, INPUT);
      break;
    case REED_SWITCH:
      configPIN(item.primaryGpio, item.primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
      break;
    case DHT_11:
    case DHT_21:
    case DHT_22:
      item.dht = new DHT_nonblocking(item.primaryGpio, item.type);
      break;
    case DS18B20:
      item.dallas = new DallasTemperature(new OneWire(item.primaryGpio));
      break;
    }
  }
}
void Sensors::save(File &file) const
{
  auto n_items = items.size();
  file.write((uint8_t *)&n_items, sizeof(n_items));
  for (const auto &item : items)
  {
    item.save(file);
  }
}
void SensorT::load(File &file)
{
  file.read((uint8_t *)id, sizeof(id));
  file.read((uint8_t *)name, sizeof(name));
  file.read((uint8_t *)family, sizeof(family));
  file.read((uint8_t *)&type, sizeof(type));
  file.read((uint8_t *)deviceClass, sizeof(deviceClass));
  file.read((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.read((uint8_t *)&mqttRetain, sizeof(mqttRetain));
  file.read((uint8_t *)&haSupport, sizeof(haSupport));
  file.read((uint8_t *)&primaryGpio, sizeof(primaryGpio));
  file.read((uint8_t *)&pullup, sizeof(pullup));
  file.read((uint8_t *)&delayRead, sizeof(delayRead));
  file.read((uint8_t *)&lastBinaryState, sizeof(lastBinaryState));
  file.read((uint8_t *)payloadOff, sizeof(payloadOff));
  file.read((uint8_t *)payloadOn, sizeof(payloadOn));
}
void SensorT::save(File &file) const
{

  file.write((uint8_t *)id, sizeof(id));
  file.write((uint8_t *)name, sizeof(name));
  file.write((uint8_t *)family, sizeof(family));
  file.write((uint8_t *)&type, sizeof(type));
  file.write((uint8_t *)deviceClass, sizeof(deviceClass));
  file.write((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.write((uint8_t *)&mqttRetain, sizeof(mqttRetain));
  file.write((uint8_t *)&haSupport, sizeof(haSupport));
  file.write((uint8_t *)&primaryGpio, sizeof(primaryGpio));
  file.write((uint8_t *)&pullup, sizeof(pullup));
  file.write((uint8_t *)&delayRead, sizeof(delayRead));
  file.write((uint8_t *)&lastBinaryState, sizeof(lastBinaryState));
  file.write((uint8_t *)payloadOff, sizeof(payloadOff));
  file.write((uint8_t *)payloadOn, sizeof(payloadOn));
}

void load(Sensors &sensors)
{
  if (!SPIFFS.begin())
  {
    Log.error("%s File storage can't start" CR, tags::sensors);
    return;
  }

  if (!SPIFFS.exists(configFilenames::sensors))
  {
    Log.notice("%s Default config loaded." CR, tags::sensors);
    SPIFFS.end();
    return;
  }

  File file = SPIFFS.open(configFilenames::sensors, "r+");
  sensors.load(file);
  file.close();
  SPIFFS.end();
  Log.notice("%s Stored values was loaded." CR, tags::sensors);
}
void save(Sensors &sensors)
{
  if (!SPIFFS.begin())
  {
    Log.error("%s File storage can't start" CR, tags::sensors);
    return;
  }

  File file = SPIFFS.open(configFilenames::sensors, "w+");
  if (!file)
  {
    Log.error("%s Failed to create file" CR, tags::sensors);
    return;
  }
  sensors.save(file);
  file.close();
  SPIFFS.end();
  Log.notice("%s Config stored." CR, tags::sensors);
}
void remove(Sensors &sensors, const char *id)
{
  if (sensors.remove(id))
    save(sensors);
}
void saveAndRefreshServices(Sensors &sensors, const SensorT &ss)
{
  save(sensors);
  removeFromHaDiscovery(ss);
  delay(10);
  if (ss.haSupport)
  {
    addToHaDiscovery(ss);
  }
}
void update(Sensors &sensors, const String &id, JsonObject doc)
{
  for (auto &ss : sensors.items)
  {
    if (strcmp(id.c_str(), ss.id) == 0)
    {
      ss.updateFromJson(doc);
      saveAndRefreshServices(sensors, ss);
      return;
    }
  }
  SensorT newSs;
  newSs.updateFromJson(doc);
  sensors.items.push_back(newSs);
  saveAndRefreshServices(sensors, newSs);
}
size_t Sensors::serializeToJson(Print &output)
{
  if (items.empty())
    return output.write("[]");
  const size_t CAPACITY = JSON_ARRAY_SIZE(items.size()) + items.size() * JSON_OBJECT_SIZE(16) + 1000;
  DynamicJsonDocument doc(CAPACITY);
  for (const auto &ss : items)
  {
    JsonObject sdoc = doc.createNestedObject();
    sdoc["id"] = ss.id;
    sdoc["name"] = ss.name;
    sdoc["family"] = ss.family;
    sdoc["type"] = static_cast<int>(ss.type);
    sdoc["deviceClass"] = ss.deviceClass;
    sdoc["mqttStateTopic"] = ss.mqttStateTopic;
    sdoc["mqttRetain"] = ss.mqttRetain;
    sdoc["primaryGpio"] = ss.primaryGpio;
    sdoc["pullup"] = ss.pullup;
    sdoc["delayRead"] = ss.delayRead;
    sdoc["lastBinaryState"] = ss.lastBinaryState;
    sdoc["temperature"] = ss.temperature;
    sdoc["humidity"] = ss.humidity;
    sdoc["payloadOff"] = ss.payloadOff;
    sdoc["payloadOn"] = ss.payloadOn;
    sdoc["haSupport"] = ss.haSupport;
  }
  return serializeJson(doc, output);
}
bool Sensors::remove(const char *id)
{

  auto match = std::find_if(items.begin(), items.end(), [id](const SensorT &item) {
    return strcmp(id, item.id) == 0;
  });

  if (match == items.end())
  {
    return false;
  }

  removeFromHaDiscovery(*match);

  items.erase(match);

  return true;
}
void initSensorsHaDiscovery(const Sensors &sensors)
{
  for (auto &ss : sensors.items)
  {
    addToHaDiscovery(ss);
    publishOnMqtt(ss.mqttStateTopic, ss.mqttPayload, ss.mqttRetain);
  }
}

void SensorT::updateFromJson(JsonObject doc)
{
  Log.notice("%s Update Environment" CR, tags::sensors);
  type = static_cast<SensorType>(doc["type"] | static_cast<int>(UNDEFINED));
  String idStr;
  strlcpy(name, doc.getMember("name").as<String>().c_str(), sizeof(name));
  generateId(idStr, name, sizeof(id));
  strlcpy(id, idStr.c_str(), sizeof(id));
  strlcpy(deviceClass, doc["deviceClass"] | constantsSensor::noneClass, sizeof(deviceClass));
  dht = NULL;
  dallas = NULL;
  primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  delayRead = doc["delayRead"];
  mqttRetain = doc["mqttRetain"] | true;
  strlcpy(payloadOn, doc["payloadOn"] | "ON", sizeof(payloadOff));
  strlcpy(payloadOff, doc["payloadOff"] | "OFF", sizeof(payloadOff));
  strlcpy(family, constantsSensor::familySensor, sizeof(family));
  switch (type)
  {
  case UNDEFINED:
    Log.error("%s Invalid type" CR, tags::sensors);
    return;
    break;
  case LDR:
    //Nothing to do
    break;
  case PIR:
  case RCWL_0516:
    configPIN(primaryGpio, INPUT);
    break;
  case REED_SWITCH:
    configPIN(primaryGpio, primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
    dht = new DHT_nonblocking(primaryGpio, type);
    break;
  case DS18B20:
    dallas = new DallasTemperature(new OneWire(primaryGpio));
    break;
  }
  String mqttTopic;
  mqttTopic.reserve(sizeof(mqttStateTopic));
  mqttTopic.concat(getBaseTopic());
  mqttTopic.concat("/");
  mqttTopic.concat(family);
  mqttTopic.concat("/");
  mqttTopic.concat(id);
  mqttTopic.concat("/state");
  strlcpy(mqttStateTopic, mqttTopic.c_str(), sizeof(mqttStateTopic));
  doc["id"] = id;
  doc["mqttStateTopic"] = mqttStateTopic;
}

void loop(Sensors &sensors)
{
  for (auto &ss : sensors.items)
  {

    if (ss.primaryGpio == constantsConfig::noGPIO)
      continue;
    switch (ss.type)
    {
    case UNDEFINED:
      continue;
      break;
    case LDR:
    {
      if (ss.lastRead + ss.delayRead < millis())
      {
        ss.lastRead = millis();
        int ldrRaw = analogRead(ss.primaryGpio);
        String analogReadAsString = String(ldrRaw);
        publishOnMqtt(ss.mqttStateTopic, String("{\"ldr_raw\":" + analogReadAsString + "}").c_str(), ss.mqttRetain);
        Log.notice("%s {\"ldr_raw\": %d }" CR, tags::mqtt, ldrRaw);
      }
    }
    break;

    case PIR:
    case REED_SWITCH:
    case RCWL_0516:
    {
      bool binaryState = readPIN(ss.primaryGpio);
      if (ss.lastBinaryState != binaryState)
      {
        ss.lastBinaryState = binaryState;
        String binaryStateAsString = String(binaryState);
        publishOnMqtt(ss.mqttStateTopic, String("{\"binary_state\":" + binaryStateAsString + "}").c_str(), ss.mqttRetain);
        Log.notice("%s {\"binary_state\": %t }" CR, tags::sensors, binaryState);
      }
    }
    break;

    case DHT_11:
    case DHT_21:
    case DHT_22:
    {

      if (ss.dht->measure(&ss.temperature, &ss.humidity) == true)
      {

        if (ss.lastRead + ss.delayRead < millis())
        {
          ss.lastRead = millis();
          String temperatureAsString = String(ss.temperature);
          String humidityAsString = String(ss.humidity);
          publishOnMqtt(ss.mqttStateTopic, String("{\"temperature\":" + temperatureAsString + ",\"humidity\":" + humidityAsString + "}").c_str(), ss.mqttRetain);

          Log.notice("%s {\"temperature\": %F ,\"humidity\": %F" CR, tags::sensors, ss.temperature, ss.humidity);
        }
      }
    }
    break;
    case DS18B20:
    {
      if (ss.lastRead + ss.delayRead < millis())
      {
        ss.dallas->begin();
        ss.dallas->requestTemperatures();
        ss.lastRead = millis();
        ss.temperature = ss.dallas->getTempCByIndex(0);
        String temperatureAsString = String(ss.temperature);
        publishOnMqtt(ss.mqttStateTopic, String("{\"temperature\":" + temperatureAsString + "}").c_str(), ss.mqttRetain);
        Log.notice("%s {\"temperature\": %F " CR, tags::sensors, ss.temperature);
      }
    }
    break;
    }
  }
}
