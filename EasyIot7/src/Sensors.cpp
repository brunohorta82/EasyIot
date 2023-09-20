#include "Sensors.h"
#include "Discovery.h"
#include <ArduinoLog.h>
#include "constants.h"
#include "WebServer.h"
#include "Config.h"
#include "Mqtt.h"
#include "CoreWiFi.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include <PZEM004T.h>
#include <PZEM004Tv30.h>
#include "CloudIO.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include "WebRequests.h"
#include <Bounce2.h>
#include "Templates.h"
#define BUS_SDA 2  //-1 if you don't use display
#define BUS_SCL 13 //-1 if you don't use display
#if WITH_DISPLAY
#include <Adafruit_SSD1306.h>

#define DISPLAY_BTN 16
bool displayOn = true;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, DISPLAY_BTN);
#define OLED_RESET 16 // Reset pin # (or -1 if sharing Arduino reset pin)

void printOnDisplay(float _voltage, float _amperage, float _power, float _energy)
{
  display.clearDisplay();
  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(5, 0);
  display.println((String(_power) + "W").c_str());
  display.setCursor(5, 8);
  display.println((String(_energy) + "Kwh").c_str());
  display.setCursor(5, 16);
  display.println((String(_voltage) + "V").c_str());
  display.setCursor(5, 24);
  display.println((String(_amperage) + "A").c_str());

  display.display();
}

void setupDisplay()
{
  Wire.begin(BUS_SDA, BUS_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("BH PZEM"));

  display.display();
}
#endif
struct Sensors &getAtualSensorsConfig()
{
  static Sensors sensors;
  return sensors;
}
void Sensors::resetAll()
{
  resetSensors(getAtualSensorsConfig());
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
#ifdef DEBUG_ONOFRE
      Log.errorln("%s Invalid type", tags::sensors);
#endif
      continue;
      break;
    case LDR:
      strlcpy(item.deviceClass, "lightness", sizeof(item.deviceClass));
      break;
    case PIR:
    case RCWL_0516:
      strlcpy(item.deviceClass, "motion", sizeof(item.deviceClass));
      configPIN(item.primaryGpio, INPUT);
      break;
    case REED_SWITCH_NC:
    case REED_SWITCH_NO:
      strlcpy(item.deviceClass, "alarm", sizeof(item.deviceClass));
      configPIN(item.primaryGpio, INPUT_PULLUP);
      break;
    case DHT_11:
    case DHT_21:
    case DHT_22:
      strlcpy(item.deviceClass, "temperature", sizeof(item.deviceClass));
      item.dht = new DHT_nonblocking(item.primaryGpio, item.type);
      break;
    case DS18B20:
      strlcpy(item.deviceClass, "temperature", sizeof(item.deviceClass));
      item.dallas = new DallasTemperature(new OneWire(item.primaryGpio));
      break;

    case PZEM_004T:
    {
      strlcpy(item.deviceClass, "power", sizeof(item.deviceClass));
#if defined(ESP8266)
      item.pzem = new PZEM004T(item.primaryGpio, item.secondaryGpio);
#endif
#if defined(ESP32)
      item.pzem = new PZEM004T(&Serial2, item.primaryGpio, item.secondaryGpio);
#endif
      IPAddress ip(192, 168, 1, item.secondaryGpio);
      item.pzem->setAddress(ip);
      configPIN(item.tertiaryGpio, INPUT);
#if WITH_DISPLAY
      setupDisplay();
#endif
    }
    break;
    case PZEM_004T_V03:
    {
      strlcpy(item.deviceClass, "power", sizeof(item.deviceClass));

#if defined(ESP8266)
      // static SoftwareSerial softwareSerial = SoftwareSerial(item.primaryGpio, item.secondaryGpio);
      item.pzemv03 = new PZEM004Tv30(item.primaryGpio, item.secondaryGpio);
#endif
#if defined(ESP32)
      item.pzemv03 = new PZEM004Tv30(Serial2, item.primaryGpio, item.secondaryGpio);
#endif
      configPIN(item.tertiaryGpio, INPUT);
    }
#if WITH_DISPLAY
      setupDisplay();
#endif
      break;
    case PZEM_017:
      strlcpy(item.deviceClass, "power", sizeof(item.deviceClass));
      // TODO item.pzemModbus = new Modbus(item.primaryGpio, item.secondaryGpio);
      // TODO item.pzemModbus->Begin(9600, 2);
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
  file.read((uint8_t *)&mqttSupport, sizeof(mqttSupport));
  file.read((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
  firmware = VERSION;
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
  file.write((uint8_t *)&mqttSupport, sizeof(mqttSupport));
  file.write((uint8_t *)&cloudIOSupport, sizeof(cloudIOSupport));
}

void Sensors::save()
{
  if (!LittleFS.begin())
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s File storage can't start" CR, tags::sensors);
#endif
    return;
  }

  File file = LittleFS.open(configFilenames::sensors, "w+");
  if (!file)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s Failed to create file" CR, tags::sensors);
#endif
    return;
  }
  this->save(file);
  file.close();

#ifdef DEBUG_ONOFRE
  Log.notice("%s Config stored." CR, tags::sensors);
#endif
}
void load(Sensors &sensors)
{

  if (!LittleFS.exists(configFilenames::sensors))
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Default config loaded." CR, tags::sensors);
#endif
    loadSensorsDefaults();
    getAtualSensorsConfig().save();
  }

  File file = LittleFS.open(configFilenames::sensors, "r");
  sensors.load(file);
  file.close();

#ifdef DEBUG_ONOFRE
  Log.notice("%s Stored values was loaded." CR, tags::sensors);
#endif
}

void saveAndRefreshServices(Sensors &sensors, const SensorT &ss)
{

  sensors.save();
  if (ss.haSupport)
  {
    addToHaDiscovery(ss);
  }
}
Sensors &Sensors::updateFromJson(const String &id, JsonObject doc)
{
  for (auto &ss : items)
  {
    if (strcmp(id.c_str(), ss.id) == 0)
    {
      ss.updateFromJson(doc);
      saveAndRefreshServices(*this, ss);
      return *this;
    }
  }
  SensorT newSs;
  newSs.updateFromJson(doc);
  items.push_back(newSs);
  saveAndRefreshServices(*this, newSs);
  return *this;
}
void Sensors::toJson(JsonVariant &root)
{
  for (const auto &ss : items)
  {
    JsonVariant sdoc = root.createNestedObject();
    sdoc["id"] = ss.id;
    sdoc["name"] = ss.name;
    sdoc["family"] = ss.family;
    sdoc["type"] = static_cast<int>(ss.type);
    sdoc["deviceClass"] = ss.deviceClass;
    sdoc["primaryGpio"] = ss.primaryGpio;
    sdoc["secondaryGpio"] = ss.secondaryGpio;
    sdoc["tertiaryGpio"] = ss.tertiaryGpio;
    sdoc["mqttStateTopic"] = ss.mqttStateTopic;
    sdoc["pullup"] = ss.pullup;
    sdoc["delayRead"] = ss.delayRead;
    sdoc["lastBinaryState"] = ss.lastBinaryState;
    sdoc["haSupport"] = ss.haSupport;
    sdoc["cloudIOSupport"] = ss.cloudIOSupport;
    sdoc["mqttSupport"] = ss.mqttSupport;
    sdoc["emoncmsSupport"] = ss.emoncmsSupport;
  }
}
Sensors &Sensors::remove(const char *id)
{
  auto match = std::find_if(items.begin(), items.end(), [id](const SensorT &item)
                            { return strcmp(id, item.id) == 0; });

  if (match == items.end())
  {
    return *this;
  }
  removeFromHaDiscovery(*match);
  items.erase(match);
  save();
  return *this;
}
void reloadSensors()
{
  for (auto &ss : getAtualSensorsConfig().items)
  {
    ss.reloadMqttTopics();
  }
  getAtualSensorsConfig().save();
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
#ifdef DEBUG_ONOFRE
  Log.notice("%s Update Environment" CR, tags::sensors);
#endif
  type = static_cast<SensorType>(doc["type"] | static_cast<int>(UNDEFINED));
  String idStr;
  strlcpy(name, doc["name"], sizeof(name));
  generateId(idStr, name, static_cast<int>(type), sizeof(id));
  strlcpy(id, idStr.c_str(), sizeof(id));
  String classDevice = doc["deviceClass"] | String(constantsSensor::powerMeterClass);
  strlcpy(deviceClass, classDevice.c_str(), sizeof(deviceClass));
  dht = NULL;
  dallas = NULL;
  primaryGpio = doc["primaryGpio"] | constantsConfig::noGPIO;
  secondaryGpio = doc["secondaryGpio"] | constantsConfig::noGPIO;
  tertiaryGpio = doc["tertiaryGpio"] | constantsConfig::noGPIO;
  delayRead = doc["delayRead"] | 0;
  mqttRetain = doc["mqttRetain"] | true;
  cloudIOSupport = doc["cloudIOSupport"] | true;
  mqttSupport = doc["mqttSupport"] | false;
  haSupport = doc["haSupport"] | true;
  emoncmsSupport = doc["emoncmsSupport"] | false;
  strlcpy(payloadOn, doc["payloadOn"] | "ON", sizeof(payloadOn));
  strlcpy(payloadOff, doc["payloadOff"] | "OFF", sizeof(payloadOff));
  strlcpy(mqttPayload, "", sizeof(mqttPayload));
  switch (type)
  {
  case UNDEFINED:
#ifdef DEBUG_ONOFRE
    Log.error("%s Invalid type" CR, tags::sensors);
#endif
    return;
    break;
  case LDR:
    strlcpy(deviceClass, "lightness", sizeof(deviceClass));
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    strlcpy(mqttPayload, "{\"illuminance\":0}", sizeof(mqttPayload));
    break;
  case PIR:
  case RCWL_0516:
    strlcpy(deviceClass, "motion", sizeof(deviceClass));
    configPIN(primaryGpio, INPUT);
    strlcpy(family, constantsSensor::binarySensorFamily, sizeof(family));
    strlcpy(mqttPayload, "{\"binary_state\":false}", sizeof(mqttPayload));
    break;
  case REED_SWITCH_NC:
    strlcpy(deviceClass, "alarm", sizeof(deviceClass));
    configPIN(primaryGpio, INPUT_PULLUP);
    strlcpy(family, constantsSensor::binarySensorFamily, sizeof(family));
    strlcpy(mqttPayload, "{\"binary_state\":1}", sizeof(mqttPayload));
    break;
  case REED_SWITCH_NO:
    strlcpy(deviceClass, "alarm", sizeof(deviceClass));
    configPIN(primaryGpio, INPUT_PULLUP);
    strlcpy(family, constantsSensor::binarySensorFamily, sizeof(family));
    strlcpy(mqttPayload, "{\"binary_state\":0}", sizeof(mqttPayload));
    break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
    strlcpy(deviceClass, "temperature", sizeof(deviceClass));
    dht = new DHT_nonblocking(primaryGpio, type);
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    strlcpy(mqttPayload, "{\"humidity\":0,\"temperature\":0}", sizeof(mqttPayload));
    break;
  case DS18B20:
    strlcpy(deviceClass, "temperature", sizeof(deviceClass));
    dallas = new DallasTemperature(new OneWire(primaryGpio));
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    break;
  case PZEM_004T:
    strlcpy(deviceClass, "power", sizeof(deviceClass));
#ifdef ESP8266
    pzem = new PZEM004T(primaryGpio, secondaryGpio);
#endif
#ifdef ESP32
    pzem = new PZEM004T(&Serial2, primaryGpio, secondaryGpio);
#endif
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
    break;
  case PZEM_004T_V03:
  {
    strlcpy(deviceClass, "power", sizeof(deviceClass));
#if defined(ESP8266)
    // static SoftwareSerial softwareSerial = SoftwareSerial(primaryGpio, secondaryGpio);
    pzemv03 = new PZEM004Tv30(primaryGpio, secondaryGpio);
    ;
#endif
#if defined(ESP32)

    pzemv03 = new PZEM004Tv30(Serial2, primaryGpio, secondaryGpio);
#endif
    strlcpy(family, constantsSensor::familySensor, sizeof(family));
  }
  break;
  case PZEM_017:
    strlcpy(deviceClass, "power", sizeof(deviceClass));
    // TODO pzemModbus = new Modbus(primaryGpio, secondaryGpio);
    //  TODO pzemModbus->Begin(9600, 2);
    break;
  }
  reloadMqttTopics();
  doc["id"] = id;
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

void publishReadings(String &readings, SensorT &sensor)
{

  strlcpy(sensor.lastReading, readings.c_str(), sizeof(sensor.lastReading));
  String id = String(sensor.id);
  sendToServerEvents(id, readings.c_str());
  if (sensor.cloudIOSupport)
    notifyStateToCloudIO(sensor.mqttCloudStateTopic, readings.c_str(), readings.length());
  if (sensor.mqttSupport)
  {
    publishOnMqtt(sensor.mqttStateTopic, readings.c_str(), sensor.mqttRetain);
  }
  if (sensor.emoncmsSupport)
    publishOnEmoncms(sensor, readings);
}

void resetSensors(Sensors &sensors)
{
  for (auto &ss : sensors.items)
  {
    if (ss.pzemv03 != NULL)
    {
      ss.pzemv03->resetEnergy();
      Log.notice("%s {\"pzemv03\": %d}" CR, tags::sensors, ss.id);
    }
    else
    {
      Log.notice("%s {\"pzemv03\": null}" CR, tags::sensors);
    }
  }
}
bool initRealTimeSensors = true;
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
        auto readings = String("{\"illuminance\":" + analogReadAsString + "}");
        publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
        Log.notice("%s {\"illuminance\": %d }" CR, tags::sensors, ldrRaw);
#endif
      }
    }
    break;

    case PIR:
    case REED_SWITCH_NC:
    case RCWL_0516:
    {
      bool binaryState = readPIN(ss.primaryGpio);
      if (ss.lastBinaryState != binaryState || initRealTimeSensors)
      {
        ss.lastRead = millis();
        ss.lastBinaryState = binaryState;
        String binaryStateAsString = String(binaryState);
        auto readings = String("{\"binary_state\":" + (binaryStateAsString) + "}");
        publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
        Log.notice("%s %s" CR, tags::sensors, readings.c_str());
#endif
      }
    }
    break;
    case REED_SWITCH_NO:
    {
      bool binaryState = !readPIN(ss.primaryGpio);
      if (ss.lastBinaryState != binaryState || initRealTimeSensors)
      {
        ss.lastRead = millis();
        ss.lastBinaryState = binaryState;
        String binaryStateAsString = String(binaryState);
        auto readings = String("{\"binary_state\":" + binaryStateAsString + "}");
        publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
        Log.notice("%s %s" CR, tags::sensors, readings.c_str());
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
          publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
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
        for (int i = 0; i < ss.oneWireSensorsCount; i++)
        {
          StaticJsonDocument<256> doc;
          JsonObject obj = doc.to<JsonObject>();
          ss.dallas->requestTemperatures();
          ss.lastRead = millis();
          ss.temperature = ss.dallas->getTempCByIndex(i);
          String temperatureAsString = String("temperature_") + String(i + 1);
          obj[temperatureAsString] = ss.temperature;
          String readings = "";
          serializeJson(doc, readings);
          publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
          Log.notice("%s %s " CR, tags::sensors, readings.c_str());
#endif
        }
      }
    }

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
#ifdef DEBUG_ONOFRE
          Log.notice("%s PZEM ERROR" CR, tags::sensors);
#endif
        }
        else
        {
          auto readings = String("{\"voltage\":" + String(v) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
          printOnDisplay(v, i, p, c);
#endif
          publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
          Log.notice("%s {\"voltage\": %F,\"current\": %F,\"power\": %F \"energy\": %F }" CR, tags::sensors, v, i, p, c);
#endif
        }
      }
      break;
    case PZEM_004T_V03:
      if (ss.lastRead + ss.delayRead < millis())
      {
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char buffer[80];
        strftime(buffer, 80, "%Y%m%d", timeinfo);
        File file = LittleFS.open("/lastday.log", "r");
        String lastDate = "";
        if (file.available())
        {
          lastDate = file.readString();
          file.close();
        }
        if (lastDate.compareTo(String(buffer)) != 0)
        {
          if (ss.pzemv03->resetEnergy())
          {
            file = LittleFS.open("/lastday.log", "w");
            file.print(buffer);
            file.close();
            continue;
          }
        }

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
#ifdef DEBUG_ONOFRE
          Log.notice("%s PZEM ERROR" CR, tags::sensors);
#endif
        }
        else
        {
          auto readings = String("{\"lastreset\":" + String(buffer) + ",\"voltage\":" + String(v) + ",\"frequency\":" + String(f) + ",\"pf\":" + String(pf) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
          printOnDisplay(v, i, p, c);
#endif
          publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
          Log.notice("%s %s" CR, tags::sensors, readings.c_str());
#endif
        }
      }
      break;
    case PZEM_017:
      /* TODO if (ss.lastRead + ss.delayRead < millis())
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
#ifdef DEBUG_ONOFRE
            Log.notice("%s PZEM ERROR" CR, tags::sensors);
#endif
          }
          else
          {
            v = (float)((buffer[3] << 8) + buffer[4]) / 100.0;
            i = (float)((buffer[5] << 8) + buffer[6]) / 100.0;
            p = (float)((buffer[9] << 24) + (buffer[10] << 16) + (buffer[7] << 8) + buffer[8]) / 10.0;
            c = (float)((buffer[13] << 24) + (buffer[14] << 16) + (buffer[11] << 8) + buffer[12]);
            auto readings = String("{\"voltage\":" + String(v) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
            printOnDisplay(v, i, p, c);
#endif
            publishReadings(readings, ss);
#ifdef DEBUG_ONOFRE
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
      */
      break;
    }
  }
  initRealTimeSensors = false;
}
