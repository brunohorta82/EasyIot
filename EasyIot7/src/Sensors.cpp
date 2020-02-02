#include "Sensors.h"
#include "Discovery.h"
#include "constants.h"
#include "WebServer.h"
#include "Config.h"
#include "Mqtt.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include <PZEM004T.h>
#include <PZEM004Tv30.h>
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include "WebRequests.h"
#include <Bounce2.h>
#include <Modbus.h>

#if WITH_DISPLAY
#include "SSD1306Wire.h" //https://github.com/ThingPulse/esp8266-oled-ssd1306
#define DISPLAY_SDA 2    //-1 if you don't use display
#define DISPLAY_SCL 13   //-1 if you don't use display
#define DISPLAY_BTN 16
bool displayOn = true;
SSD1306Wire display(0x3C, DISPLAY_SDA, DISPLAY_SCL);
bool lastState = false;
Bounce debouncer = Bounce();
void printOnDisplay(float _voltage, float _amperage, float _power, float _energy)
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(5, 0, String(_power) + "W");
  display.setFont(ArialMT_Plain_10);
  display.drawString(5, 16, String(_energy) + " kWh");
  display.drawString(5, 26, String(_voltage) + " V");
  display.drawString(5, 36, String(_amperage) + " A");
  display.display();
}

void setupDisplay()
{
  pinMode(DISPLAY_BTN, INPUT_PULLDOWN_16);
  debouncer.attach(DISPLAY_BTN);
  debouncer.interval(5); // interval in ms
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5, 0, "BH PZEM");
  display.display();
}
#endif
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
#ifdef DEBUG
      Log.error("%s Invalid type" CR, tags::sensors);
#endif
      continue;
      break;
    case LDR:
      //nothing to do
      break;
    case PIR:
    case RCWL_0516:
      configPIN(item.primaryGpio, INPUT);
      break;
    case REED_SWITCH_NC:
    case REED_SWITCH_NO:
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
    case PZEM_004T:
    {
      item.pzem = new PZEM004T(item.primaryGpio, item.secondaryGpio);
      IPAddress ip(192, 168, 1, item.secondaryGpio);
      item.pzem->setAddress(ip);
      configPIN(item.tertiaryGpio, INPUT);
#if WITH_DISPLAY
      setupDisplay();
#endif
    }
    break;
    case PZEM_004T_V03:
      item.pzemv03 = new PZEM004Tv30(item.primaryGpio, item.secondaryGpio);
      configPIN(item.tertiaryGpio, INPUT);
#if WITH_DISPLAY
      setupDisplay();
#endif
      break;
    case PZEM_017:
      item.pzemModbus = new Modbus(item.primaryGpio, item.secondaryGpio);
      item.pzemModbus->Begin(9600, 2);
#if WITH_DISPLAY
      setupDisplay();
#endif
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
  file.read((uint8_t *)&firmware, sizeof(firmware));
  file.read((uint8_t *)id, sizeof(id));
  file.read((uint8_t *)name, sizeof(name));
  file.read((uint8_t *)family, sizeof(family));
  file.read((uint8_t *)&type, sizeof(type));
  file.read((uint8_t *)deviceClass, sizeof(deviceClass));
  file.read((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.read((uint8_t *)&mqttRetain, sizeof(mqttRetain));
  file.read((uint8_t *)&haSupport, sizeof(haSupport));
  file.read((uint8_t *)&emoncmsSupport, sizeof(emoncmsSupport));
  file.read((uint8_t *)&primaryGpio, sizeof(primaryGpio));
  file.read((uint8_t *)&secondaryGpio, sizeof(secondaryGpio));
  file.read((uint8_t *)&tertiaryGpio, sizeof(tertiaryGpio));
  file.read((uint8_t *)&pullup, sizeof(pullup));
  file.read((uint8_t *)&delayRead, sizeof(delayRead));
  file.read((uint8_t *)&lastBinaryState, sizeof(lastBinaryState));
  file.read((uint8_t *)payloadOff, sizeof(payloadOff));
  file.read((uint8_t *)payloadOn, sizeof(payloadOn));
  file.read((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.read((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.read((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));
  if (firmware < VERSION)
  {
#ifdef DEBUG
    Log.notice("%s Migrate Firmware from %F to %F" CR, tags::sensors, firmware, VERSION);
#endif
    firmware = VERSION;
  }
}
void SensorT::save(File &file) const
{
  file.write((uint8_t *)&firmware, sizeof(firmware));
  file.write((uint8_t *)id, sizeof(id));
  file.write((uint8_t *)name, sizeof(name));
  file.write((uint8_t *)family, sizeof(family));
  file.write((uint8_t *)&type, sizeof(type));
  file.write((uint8_t *)deviceClass, sizeof(deviceClass));
  file.write((uint8_t *)mqttStateTopic, sizeof(mqttStateTopic));
  file.write((uint8_t *)&mqttRetain, sizeof(mqttRetain));
  file.write((uint8_t *)&haSupport, sizeof(haSupport));
  file.write((uint8_t *)&emoncmsSupport, sizeof(emoncmsSupport));
  file.write((uint8_t *)&primaryGpio, sizeof(primaryGpio));
  file.write((uint8_t *)&secondaryGpio, sizeof(secondaryGpio));
  file.write((uint8_t *)&tertiaryGpio, sizeof(tertiaryGpio));
  file.write((uint8_t *)&pullup, sizeof(pullup));
  file.write((uint8_t *)&delayRead, sizeof(delayRead));
  file.write((uint8_t *)&lastBinaryState, sizeof(lastBinaryState));
  file.write((uint8_t *)payloadOff, sizeof(payloadOff));
  file.write((uint8_t *)payloadOn, sizeof(payloadOn));
  file.write((uint8_t *)&knxLevelOne, sizeof(knxLevelOne));
  file.write((uint8_t *)&knxLevelTwo, sizeof(knxLevelTwo));
  file.write((uint8_t *)&knxLevelThree, sizeof(knxLevelThree));
}

void save(Sensors &sensors)
{
  if (!SPIFFS.begin())
  {
#ifdef DEBUG
    Log.error("%s File storage can't start" CR, tags::sensors);
#endif
    return;
  }

  File file = SPIFFS.open(configFilenames::sensors, "w+");
  if (!file)
  {
#ifdef DEBUG
    Log.error("%s Failed to create file" CR, tags::sensors);
#endif
    return;
  }
  sensors.save(file);
  file.close();
  SPIFFS.end();
#ifdef DEBUG
  Log.notice("%s Config stored." CR, tags::sensors);
#endif
}
void load(Sensors &sensors)
{
  if (!SPIFFS.begin())
  {
#ifdef DEBUG
    Log.error("%s File storage can't start" CR, tags::sensors);
#endif
    return;
  }

  if (!SPIFFS.exists(configFilenames::sensors))
  {
#ifdef DEBUG
    Log.notice("%s Default config loaded." CR, tags::sensors);
#endif
#if defined BHPZEM_004T || BHPZEM_004T_2_0 || BHPZEM_004T_V03 || BHPZEM_004T_V03_2_0 || BHPZEM_017
    SensorT pzem;
    strlcpy(pzem.name, "Consumo", sizeof(pzem.name));
    String idStr;
    generateId(idStr, pzem.name, sizeof(pzem.id));
    strlcpy(pzem.id, idStr.c_str(), sizeof(pzem.id));
    strlcpy(pzem.family, constantsSensor::familySensor, sizeof(pzem.name));
#if defined BHPZEM_004T_2_0 || BHPZEM_004T
    pzem.type = PZEM_004T;
#endif
#if defined BHPZEM_017
    pzem.type = PZEM_017;
#endif
#if defined BHPZEM_004T_V03 || BHPZEM_004T_V03_2_0
    pzem.type = PZEM_004T_V03;
#endif
    pzem.knxLevelOne = 3;
    pzem.knxLevelTwo = 1;
    pzem.knxLevelThree = 1;
#if defined BHPZEM_004T || BHPZEM_004T_V03
    pzem.primaryGpio = 4;
    pzem.secondaryGpio = 5;
#endif
#if defined BHPZEM_004T_2_0 || BHPZEM_004T_V03_2_0 || BHPZEM_017
    pzem.primaryGpio = 3;
    pzem.secondaryGpio = 1;
#endif

    pzem.tertiaryGpio = constantsConfig::noGPIO;
    pzem.mqttRetain = true;
    pzem.haSupport = true;
    pzem.emoncmsSupport = true;
    pzem.delayRead = 5000;
    String mqttTopic;
    mqttTopic.reserve(sizeof(pzem.mqttStateTopic));
    mqttTopic.concat(getBaseTopic());
    mqttTopic.concat("/");
    mqttTopic.concat(pzem.family);
    mqttTopic.concat("/");
    mqttTopic.concat(pzem.id);
    mqttTopic.concat("/state");
    strlcpy(pzem.mqttStateTopic, mqttTopic.c_str(), sizeof(pzem.mqttStateTopic));
    strlcpy(pzem.payloadOn, "ON", sizeof(pzem.payloadOn));
    strlcpy(pzem.payloadOff, "OFF", sizeof(pzem.payloadOff));
    strlcpy(pzem.mqttPayload, "", sizeof(pzem.mqttPayload));
    strlcpy(pzem.deviceClass, constantsSensor::noneClass, sizeof(pzem.deviceClass));
    sensors.items.push_back(pzem);
    SPIFFS.end();
    save(sensors);
    load(sensors);
    return;
#endif
  }

  File file = SPIFFS.open(configFilenames::sensors, "r+");
  sensors.load(file);
  file.close();
  SPIFFS.end();
#ifdef DEBUG
  Log.notice("%s Stored values was loaded." CR, tags::sensors);
#endif
}
void remove(Sensors &sensors, const char *id)
{
  if (sensors.remove(id))
    save(sensors);
}
void saveAndRefreshServices(Sensors &sensors, const SensorT &ss)
{

  save(sensors);
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
  const size_t CAPACITY = JSON_ARRAY_SIZE(items.size()) + items.size() * (JSON_OBJECT_SIZE(24) + sizeof(SensorT));
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
    sdoc["secondaryGpio"] = ss.secondaryGpio;
    sdoc["tertiaryGpio"] = ss.tertiaryGpio;
    sdoc["pullup"] = ss.pullup;
    sdoc["delayRead"] = ss.delayRead;
    sdoc["lastBinaryState"] = ss.lastBinaryState;
    sdoc["temperature"] = ss.temperature;
    sdoc["humidity"] = ss.humidity;
    sdoc["payloadOff"] = ss.payloadOff;
    sdoc["payloadOn"] = ss.payloadOn;
    sdoc["haSupport"] = ss.haSupport;
    sdoc["emoncmsSupport"] = ss.emoncmsSupport;
    sdoc["knxLevelOne"] = ss.knxLevelOne;
    sdoc["knxLevelTwo"] = ss.knxLevelTwo;
    sdoc["knxLevelThree"] = ss.knxLevelThree;
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
void reloadSensors()
{
  for (auto &ss : getAtualSensorsConfig().items)
  {
    ss.reloadMqttTopics();
  }
  save(getAtualSensorsConfig());
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
  removeFromHaDiscovery(*this);
#ifdef DEBUG
  Log.notice("%s Update Environment" CR, tags::sensors);
#endif
  type = static_cast<SensorType>(doc["type"] | static_cast<int>(UNDEFINED));
  String idStr;
  strlcpy(name, doc.getMember("name").as<String>().c_str(), sizeof(name));
  generateId(idStr, name, sizeof(id));
  strlcpy(id, idStr.c_str(), sizeof(id));
  strlcpy(deviceClass, doc["deviceClass"] | constantsSensor::noneClass, sizeof(deviceClass));
  dht = NULL;
  dallas = NULL;
  primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  secondaryGpio = doc["secondaryGpio"] | constantsConfig::noGPIO;
  tertiaryGpio = doc["tertiaryGpio"] | constantsConfig::noGPIO;
  delayRead = doc["delayRead"];
  mqttRetain = doc["mqttRetain"] | true;
  haSupport = doc["haSupport"] | true;
  knxLevelOne = doc["knxLevelOne"] | 0;
  knxLevelTwo = doc["knxLevelTwo"] | 0;
  knxLevelThree = doc["knxLevelThree"] | 0;
  emoncmsSupport = doc["emoncmsSupport"] | false;
  strlcpy(payloadOn, doc["payloadOn"] | "ON", sizeof(payloadOn));
  strlcpy(payloadOff, doc["payloadOff"] | "OFF", sizeof(payloadOff));
  strlcpy(mqttPayload, "", sizeof(mqttPayload));
  switch (type)
  {
  case UNDEFINED:
#ifdef DEBUG
    Log.error("%s Invalid type" CR, tags::sensors);
#endif
    return;
    break;
  case LDR:
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    strlcpy(mqttPayload, "{\"illuminance\":0}", sizeof(mqttPayload));
    break;
  case PIR:
  case RCWL_0516:
    configPIN(primaryGpio, INPUT);
    strlcpy(family, constantsSensor::binarySensorFamily, sizeof(family));
    strlcpy(mqttPayload, "{\"binary_state\":false}", sizeof(mqttPayload));
    break;
  case REED_SWITCH_NC:
    configPIN(primaryGpio, primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    strlcpy(family, constantsSensor::binarySensorFamily, sizeof(family));
    strlcpy(mqttPayload, "{\"binary_state\":1}", sizeof(mqttPayload));
    break;
  case REED_SWITCH_NO:
    configPIN(primaryGpio, primaryGpio == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    strlcpy(family, constantsSensor::binarySensorFamily, sizeof(family));
    strlcpy(mqttPayload, "{\"binary_state\":0}", sizeof(mqttPayload));
    break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
    dht = new DHT_nonblocking(primaryGpio, type);
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    strlcpy(mqttPayload, "{\"humidity\":0,\"temperature\":0}", sizeof(mqttPayload));
    break;
  case DS18B20:
    dallas = new DallasTemperature(new OneWire(primaryGpio));
    strlcpy(family, constantsSensor::familySensor, sizeof(family));

    break;
  case PZEM_004T:
    pzem = new PZEM004T(primaryGpio, secondaryGpio);
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    break;
  case PZEM_004T_V03:
    pzemv03 = new PZEM004Tv30(primaryGpio, secondaryGpio);
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    break;
  case PZEM_017:
    pzemModbus = new Modbus(primaryGpio, secondaryGpio);
    pzemModbus->Begin(9600, 2);
    break;
  }
  reloadMqttTopics();
  doc["id"] = id;
  doc["mqttStateTopic"] = mqttStateTopic;
}
void SensorT::reloadMqttTopics()
{
  String mqttTopic;
  mqttTopic.reserve(sizeof(mqttStateTopic));
  mqttTopic.concat(getBaseTopic());
  mqttTopic.concat("/");
  mqttTopic.concat(family);
  mqttTopic.concat("/");
  mqttTopic.concat(id);
  mqttTopic.concat("/state");
  strlcpy(mqttStateTopic, mqttTopic.c_str(), sizeof(mqttStateTopic));
}
void loop(Sensors &sensors)
{
#if WITH_DISPLAY
  debouncer.update();
  int value = debouncer.read();
  if (lastState != value)
  {
    lastState = value;
    if (value)
    {

      if (displayOn)
      {
        display.displayOff();
        displayOn = false;
      }
      else
      {
        display.displayOn();
        displayOn = true;
      }
    }
  }
#endif
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
        auto readings = String("{\"illuminance\":" + analogReadAsString + "}");
        publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
        sendToServerEvents("sensors", readings.c_str());
        publishOnEmoncms(ss, readings);
#ifdef DEBUG
        Log.notice("%s {\"illuminance\": %d }" CR, tags::mqtt, ldrRaw);
#endif
      }
    }
    break;

    case PIR:
    case REED_SWITCH_NC:
    case RCWL_0516:
    {
      bool binaryState = readPIN(ss.primaryGpio);
      if (ss.lastBinaryState != binaryState)
      {
        ss.lastBinaryState = binaryState;
        String binaryStateAsString = String(binaryState);
        auto readings = String("{\"binary_state\":" + binaryStateAsString + "}");
        publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
        sendToServerEvents("sensors", readings.c_str());
#ifdef DEBUG
        Log.notice("%s {\"binary_state\": %t }" CR, tags::sensors, binaryState);
#endif
      }
    }
    break;
    case REED_SWITCH_NO:
    {
      bool binaryState = !readPIN(ss.primaryGpio);
      if (ss.lastBinaryState != binaryState)
      {
        ss.lastBinaryState = binaryState;
        String binaryStateAsString = String(binaryState);
        auto readings = String("{\"binary_state\":" + binaryStateAsString + "}");
        publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
        sendToServerEvents("sensors", readings.c_str());
#ifdef DEBUG
        Log.notice("%s {\"binary_state\": %t }" CR, tags::sensors, binaryState);
#endif
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
          auto readings = String("{\"temperature\":" + temperatureAsString + ",\"humidity\":" + humidityAsString + "}");
          publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
          sendToServerEvents("sensors", readings.c_str());
#ifdef DEBUG
          Log.notice("%s {\"temperature\": %F ,\"humidity\": %F}" CR, tags::sensors, ss.temperature, ss.humidity);
#endif
        }
      }
    }
    break;
    case DS18B20:
    {

      if (ss.lastRead + ss.delayRead < millis())
      {
        ss.dallas->begin();
        ss.oneWireSensorsCount = ss.dallas->getDeviceCount();
        StaticJsonDocument<256> doc;
        JsonObject obj = doc.to<JsonObject>();
        for (int i = 0; i < ss.oneWireSensorsCount; i++)
        {
          ss.dallas->requestTemperatures();
          ss.lastRead = millis();
          ss.temperature = ss.dallas->getTempCByIndex(i);
          String temperatureAsString = String("temperature_") + String(i + 1);
          obj[temperatureAsString] = trunc(ss.temperature);
        }
        String readings = "";
        serializeJson(doc, readings);
        publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
        sendToServerEvents("sensors", readings.c_str());
#ifdef DEBUG
        Log.notice("%s %s " CR, tags::sensors, readings.c_str());
#endif
      }
    }
    break;
    case PZEM_004T:
      if (ss.lastRead + ss.delayRead < millis())
      {
        IPAddress ip(192, 168, 1, ss.secondaryGpio);
        ss.lastRead = millis();
        float v = ss.pzem->voltage(ip);

        float i = ss.pzem->current(ip);

        float p = ss.pzem->power(ip);

        float c = ss.pzem->energy(ip);

        if (ss.tertiaryGpio != constantsConfig::noGPIO)
        {
          if (digitalRead(ss.tertiaryGpio))
          {
            p = p * -1;
          }
        }

        if (v < 0.0)
        {
#ifdef DEBUG
          Log.notice("%s PZEM ERROR" CR, tags::sensors);
#endif
        }
        else
        {
          auto readings = String("{\"pzem\":" + String(ss.secondaryGpio) + ",\"voltage\":" + String(v) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
          printOnDisplay(v, i, p, c);
#endif
          publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
          sendToServerEvents("sensors", readings.c_str());
          publishOnEmoncms(ss, readings);
#ifdef DEBUG
          Log.notice("%s {\"voltage\": %F,\"current\": %F,\"power\": %F \"energy\": %F }" CR, tags::sensors, v, i, p, c);
#endif
        }
      }
      break;
    case PZEM_004T_V03:
      if (ss.lastRead + ss.delayRead < millis())
      {
        ss.lastRead = millis();
        float v = ss.pzemv03->voltage();
        float i = ss.pzemv03->current();
        float p = ss.pzemv03->power();
        float c = ss.pzemv03->energy();
        float f = ss.pzemv03->frequency();
        float pf = ss.pzemv03->pf();
        if (ss.tertiaryGpio != constantsConfig::noGPIO)
        {
          if (digitalRead(ss.tertiaryGpio))
          {
            p = p * -1;
          }
        }

        if (isnan(v))
        {
#ifdef DEBUG
          Log.notice("%s PZEM ERROR" CR, tags::sensors);
#endif
        }
        else
        {
          auto readings = String("{\"pzem\":" + String(ss.secondaryGpio) + ",\"voltage\":" + String(v) + ",\"frequency\":" + String(f) + ",\"pf\":" + String(pf) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
          printOnDisplay(v, i, p, c);
#endif
          publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
          sendToServerEvents("sensors", readings.c_str());
          publishOnEmoncms(ss, readings);
#ifdef DEBUG
          Log.notice("%s %s" CR, tags::sensors, readings.c_str());
#endif
        }
      }
      break;
    case PZEM_017:
      if (ss.lastRead + ss.delayRead < millis())
      {

        ss.lastRead = millis();
        float v = -1;
        float i = -1;
        float p = -1;
        float c = -1;
        static uint8_t send_retry = 0;
        bool data_ready = ss.pzemModbus->ReceiveReady();
        if (data_ready)
        {

          uint8_t buffer[22];
          uint8_t error = ss.pzemModbus->ReceiveBuffer(buffer, 8);
          if (error)
          {
#ifdef DEBUG
            Log.notice("%s PZEM ERROR" CR, tags::sensors);
#endif
          }
          else
          {
            v = (float)((buffer[3] << 8) + buffer[4]) / 100.0;
            i = (float)((buffer[5] << 8) + buffer[6]) / 100.0;
            p = (float)((buffer[9] << 24) + (buffer[10] << 16) + (buffer[7] << 8) + buffer[8]) / 10.0;
            c = (float)((buffer[13] << 24) + (buffer[14] << 16) + (buffer[11] << 8) + buffer[12]);
            auto readings = String("{\"pzem\":" + String(ss.secondaryGpio) + ",\"voltage\":" + String(v) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
            printOnDisplay(v, i, p, c);
#endif
            publishOnMqtt(ss.mqttStateTopic, readings.c_str(), ss.mqttRetain);
            sendToServerEvents("sensors", readings.c_str());
            publishOnEmoncms(ss, readings);
#ifdef DEBUG
            Log.notice("%s %s" CR, tags::sensors, readings.c_str());
#endif
          }
        }

        if (0 == send_retry || data_ready)
        {
          send_retry = 5;
          ss.pzemModbus->Send(0x01, 0x04, 0, 8);
        }
        else
        {

          send_retry--;
        }
      }
      break;
    }
  }
}
