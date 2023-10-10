#include "Sensors.h"
#include "HomeAssistantMqttDiscovery.h"
#include <ArduinoLog.h>
#include "WebServer.h"
#include "ConfigOnofre.h"
#include "Mqtt.h"
#include "CoreWiFi.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include <PZEM004T.h>
#include <PZEM004Tv30.h>
#include "CloudIO.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include "HanOnofre.hpp"
#include <ModbusMaster.h>
extern ConfigOnofre config;
void SensorT::notifyState()
{
  // Notify by MQTT/Homeassistant
  if (mqttConnected())
  {
    publishOnMqtt(readTopic, state, false);
  }
  // Notify by MQTT OnofreCloud
  if (cloudIOConnected())
  {
    notifyStateToCloudIO(cloudIOreadTopic, state.c_str());
  }
  // Notify by SSW Webpanel
  sendToServerEvents(uniqueId, state.c_str());
}
ModbusMaster node;
void SensorT::setup()
{
  switch (interface)
  {
  case UNDEFINED:
#ifdef DEBUG_ONOFRE
    Log.errorln("%s Invalid type", tags::sensors);
#endif
    return;
    break;
  case LDR:
    strlcpy(family, "LIGHTNESS", sizeof(family));
    break;
  case PIR:
  case RCWL_0516:
    strlcpy(family, "MOTION", sizeof(family));
    configPIN(primaryGpio, INPUT);
    break;
  case REED_SWITCH_NC:
  case REED_SWITCH_NO:
    strlcpy(family, "ALARM", sizeof(family));
    configPIN(primaryGpio, INPUT_PULLUP);
    break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
    strlcpy(family, "TEMPERATURE", sizeof(family));
    dht = new DHT_nonblocking(primaryGpio, interface);
    break;
  case DS18B20:
    strlcpy(family, "TEMPERATURE", sizeof(family));
    dallas = new DallasTemperature(new OneWire(primaryGpio));
    break;
  case HAN:
  {
#ifdef ESP32
    Serial1.begin(9600, SERIAL_8N2, 13, 14);
    node.begin(1, Serial1);
#endif
#ifdef ESP8266
    Serial.begin(9600, SERIAL_8N2);
    Serial.pins(primaryGpio, secondaryGpio);
    node.begin(1, Serial);
#endif
  }
  break;
  case PZEM_004T:
  {
    strlcpy(family, "POWER", sizeof(family));
#if defined(ESP8266)
    pzem = new PZEM004T(primaryGpio, secondaryGpio);
#endif
#if defined(ESP32)
    pzem = new PZEM004T(&Serial2, primaryGpio, secondaryGpio);
#endif
    IPAddress ip(192, 168, 1, secondaryGpio);
    pzem->setAddress(ip);
    configPIN(tertiaryGpio, INPUT);
  }
  break;
  case PZEM_004T_V03:
  {
    strlcpy(family, "POWER", sizeof(family));

#if defined(ESP8266)
    SoftwareSerial softwareSerial = SoftwareSerial(primaryGpio, secondaryGpio);
    pzemv03 = new PZEM004Tv30(softwareSerial);
#endif
#if defined(ESP32)
    pzemv03 = new PZEM004Tv30(Serial2, primaryGpio, secondaryGpio);
#endif
    configPIN(tertiaryGpio, INPUT);
  }
#if WITH_DISPLAY
    setupDisplay();
#endif
    break;
  }
}

void SensorT::loop()
{
  state.clear();
  switch (interface)
  {
  case UNDEFINED:
    return;
    break;
  case LDR:
  {
    if (lastRead + delayRead < millis())
    {
      lastRead = millis();
      int ldrRaw = analogRead(primaryGpio);
      String analogReadAsString = String(ldrRaw);
      state = String("{\"illuminance\":" + analogReadAsString + "}");
      notifyState();
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
    bool binaryState = readPIN(primaryGpio);
    if (lastBinaryState != binaryState)
    {
      lastRead = millis();
      lastBinaryState = binaryState;
      String binaryStateAsString = String(binaryState);
      state = String("{\"binary_state\":" + (binaryStateAsString) + "}");
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s" CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;
  case REED_SWITCH_NO:
  {
    bool binaryState = !readPIN(primaryGpio);
    if (lastBinaryState != binaryState)
    {
      lastRead = millis();
      lastBinaryState = binaryState;
      String binaryStateAsString = String(binaryState);
      state = String("{\"binary_state\":" + binaryStateAsString + "}");
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s" CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;

  case DHT_11:
  case DHT_21:
  case DHT_22:
  {
    float temperature;
    float humidity;
    if (dht->measure(&temperature, &humidity) == true)
    {
      if (lastRead + delayRead < millis())
      {
        lastRead = millis();
        String temperatureAsString = String(temperature);
        String humidityAsString = String(humidity);
        state = String("{\"temperature\":" + temperatureAsString + ",\"humidity\":" + humidityAsString + "}");
        notifyState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s {\"temperature\": %F ,\"humidity\": %F}" CR, tags::sensors, temperature, humidity);
#endif
      }
    }
  }
  break;
  case DS18B20:
  {

    if (lastRead + delayRead < millis())
    {
      dallas->begin();
      oneWireSensorsCount = dallas->getDeviceCount();
      for (int i = 0; i < oneWireSensorsCount; i++)
      {
        StaticJsonDocument<256> doc;
        JsonObject obj = doc.to<JsonObject>();
        dallas->requestTemperatures();
        lastRead = millis();
        float temperature = dallas->getTempCByIndex(i);
        String temperatureAsString = String("temperature_") + String(i + 1);
        obj[temperatureAsString] = temperature;
        serializeJson(doc, state);
        notifyState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
      }
    }
  }
  case HAN:
  {

    if (lastRead + delayRead < millis())
    {
      lastRead = millis();
      StaticJsonDocument<256>
          doc;
      JsonObject obj = doc.to<JsonObject>();
      node.clearResponseBuffer();
      if (node.readInputRegisters(INSTANTANEOUS_VOLTAGE_L1, 2) == node.ku8MBSuccess)
      {
        doc["voltage"] = node.getResponseBuffer(0) / 10.0;
        doc["current"] = node.getResponseBuffer(1) / 10.0;
      }
      if (node.readInputRegisters(INSTANTANEOUS_ACTIVE_POWER_PLUS_SUM_OF_ALL_PHASES, 1) == node.ku8MBSuccess)
      {
        doc["power"] = node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16;
      }
      if (node.readInputRegisters(INSTANTANEOUS_FREQUENCY, 1) == node.ku8MBSuccess)
      {
        doc["frequency"] = node.getResponseBuffer(0) / 10.0;
      }
      serializeJson(doc, state);
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  case PZEM_004T:
    if (lastRead + delayRead < millis())
    {
      IPAddress ip(192, 168, 1, secondaryGpio);
      lastRead = millis();
      float v = pzem->voltage(ip);

      float i = pzem->current(ip);

      float p = pzem->power(ip);

      float c = pzem->energy(ip);

      if (tertiaryGpio != constantsConfig::noGPIO)
      {
        if (digitalRead(tertiaryGpio))
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
        state = String("{\"voltage\":" + String(v) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
        printOnDisplay(v, i, p, c);
#endif
        notifyState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s {\"voltage\": %F,\"current\": %F,\"power\": %F \"energy\": %F }" CR, tags::sensors, v, i, p, c);
#endif
      }
    }
    break;
  case PZEM_004T_V03:
    if (lastRead + delayRead < millis())
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
        if (pzemv03->resetEnergy())
        {
          file = LittleFS.open("/lastday.log", "w");
          file.print(buffer);
          file.close();
          return;
        }
      }

      lastRead = millis();
      float v = pzemv03->voltage();
      float i = pzemv03->current();
      float p = pzemv03->power();
      float c = pzemv03->energy();
      float f = pzemv03->frequency();
      float pf = pzemv03->pf();
      if (tertiaryGpio != constantsConfig::noGPIO)
      {
        if (digitalRead(tertiaryGpio))
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
        state = String("{\"lastreset\":" + String(buffer) + ",\"voltage\":" + String(v) + ",\"frequency\":" + String(f) + ",\"pf\":" + String(pf) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(c) + "}");
#if WITH_DISPLAY
        printOnDisplay(v, i, p, c);
#endif
        notifyState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s %s" CR, tags::sensors, state.c_str());
#endif
      }
    }
    break;
  }
}
