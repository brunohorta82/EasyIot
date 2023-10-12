#include "Sensors.h"
#include "HomeAssistantMqttDiscovery.h"
#include <ArduinoLog.h>
#include "WebServer.h"
#include "ConfigOnofre.h"
#include "Mqtt.h"
#include "CoreWiFi.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include <PZEM004Tv30.h>
#include "CloudIO.h"
#include <DallasTemperature.h>
#include <dht_nonblocking.h>
#include "HanOnofre.hpp"
#include <ModbusMaster.h>
#include <Wire.h>
#include <SHT3x.h>
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
SHT3x sensor;
void SensorT::setup()
{
  switch (interface)
  {
  case SHT3x_SENSOR:
    Wire.setPins(13, 14);
    sensor.Begin();
    break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
    dht = new DHT_nonblocking(inputs[0], interface);
    break;
  case DS18B20:
    dallas = new DallasTemperature(new OneWire(inputs[0]));
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
  case PZEM_004T_V03:
  {
#if defined(ESP8266)
    SoftwareSerial softwareSerial = SoftwareSerial(primaryGpio, secondaryGpio);
    pzemv03 = new PZEM004Tv30(softwareSerial);
#endif
#if defined(ESP32)
    pzemv03 = new PZEM004Tv30(Serial2, inputs[0], inputs[1]);
#endif
  }
#if WITH_DISPLAY
    setupDisplay();
#endif
    break;
  }
}
typedef union
{
  struct
  {
    uint16_t year;
    uint8_t day_of_month;
    uint8_t month;
    uint8_t hour;
    uint8_t week_day;
    uint8_t second;
    uint8_t minute;
    int8_t deviation_lsb;
    uint8_t hundreds_of_second;
    uint8_t clock_status;
    int8_t deviation_msb;
  };
  std::array<std::uint16_t, 6> buffer;
} __attribute__((packed)) han_clock_t;

void SensorT::loop()
{

  switch (interface)
  {
  case LDR:
  {
    if (lastRead + delayRead < millis())
    {
      StaticJsonDocument<256> doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      obj["illuminance"] = analogRead(inputs[0]);
      serializeJson(doc, state);
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s LDR %s}" CR, tags::sensors, state);
#endif
    }
  }
  break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
  {
    if (lastRead + delayRead < millis())
    {
      float temperature;
      float humidity;
      StaticJsonDocument<256> doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      if (dht->measure(&temperature, &humidity) == true)
      {
        obj["temperature"] = temperature;
        obj["humidity"] = humidity;
        serializeJson(doc, state);
        notifyState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s DHT %s}" CR, tags::sensors, state);
#endif
      }
    }
  }
  break;
  case SHT3x_SENSOR:
  {

    if (lastRead + delayRead < millis())
    {
      sensor.UpdateData();
      StaticJsonDocument<256> doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      obj["temperature"] = sensor.GetTemperature();
      obj["humidity"] = sensor.GetRelHumidity();
      serializeJson(doc, state);
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
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
        state.clear();
        lastRead = millis();
        dallas->requestTemperatures();
        obj["temperature"] = dallas->getTempCByIndex(i);
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
      StaticJsonDocument<300> doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      node.clearResponseBuffer();
      if (node.readInputRegisters(CLOCK, 1) == node.ku8MBSuccess)
      {
        std::array<uint16_t, 6> buffer;
        for (size_t i = 0; i <= node.available(); ++i)
        {
          buffer[i] = node.getResponseBuffer(i);
        }
        han_clock_t clock{.buffer = buffer};
        char strftime_buf[64];
        sprintf(strftime_buf, "%d-%02d-%02dT%02d:%02d:%02d.000", clock.year, clock.day_of_month, clock.day_of_month, clock.hour, clock.minute, clock.second);
        doc["dateTime"] = strftime_buf;
      }
      if (node.readInputRegisters(INSTANTANEOUS_VOLTAGE_L1, 2) == node.ku8MBSuccess)
      {
        doc["voltage"] = node.getResponseBuffer(0) / 10.0;
        doc["current"] = node.getResponseBuffer(1) / 10.0;
      }
      if (node.readInputRegisters(ACTIVE_ENERGY_IMPORT_PLUS_A, 2) == node.ku8MBSuccess)
      {
        doc["powerImport"] = (node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16) / 1000.0;
        doc["powerExport"] = (node.getResponseBuffer(3) | node.getResponseBuffer(2) << 16) / 1000.0;
      }
      if (node.readInputRegisters(RATE_1_CONTRACT_1_ACTIVE_ENERGY_PLUS_A, 3) == node.ku8MBSuccess)
      {
        doc["rate1"] = (node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16) / 1000.0;
        doc["rate2"] = (node.getResponseBuffer(3) | node.getResponseBuffer(2) << 16) / 1000.0;
        doc["rate3"] = (node.getResponseBuffer(5) | node.getResponseBuffer(4) << 16) / 1000.0;
      }
      if (node.readInputRegisters(INSTANTANEOUS_ACTIVE_POWER_PLUS_SUM_OF_ALL_PHASES, 3) == node.ku8MBSuccess)
      {
        doc["power"] = node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16;
        doc["export"] = node.getResponseBuffer(3) | node.getResponseBuffer(2) << 16;
        doc["powerFactor"] = node.getResponseBuffer(4) / 1000.0;
      }
      if (node.readInputRegisters(INSTANTANEOUS_FREQUENCY, 1) == node.ku8MBSuccess)
      {
        doc["frequency"] = node.getResponseBuffer(0) / 10.0;
      }
      if (node.readInputRegisters(CURRENTLY_ACTIVE_TARIFF, 1) == node.ku8MBSuccess)
      {
        doc["tarif"] = node.getResponseBuffer(0) >> 8;
      }
      if (node.readInputRegisters(ACTIVE_DEMAND_CONTROL_THRESHOLD_T1, 3) == node.ku8MBSuccess)
      {
        doc["demandControlT1"] = (node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16) / 1000.0;
        doc["demandControlT2"] = (node.getResponseBuffer(3) | node.getResponseBuffer(2) << 16) / 1000.0;
        doc["demandControlT3"] = (node.getResponseBuffer(5) | node.getResponseBuffer(4) << 16) / 1000.0;
      }
      serializeJson(doc, state);
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  case PZEM_004T_V03:
    if (lastRead + delayRead < millis())
    {
      lastRead = millis();
      StaticJsonDocument<256> doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      float v = pzemv03->voltage();
      if (isnan(v))
        return;
      obj["voltage"] = v;
      obj["frequency"] = pzemv03->frequency();
      obj["powerFactor"] = pzemv03->pf();
      obj["current"] = pzemv03->current();
      obj["power"] = pzemv03->power();
      obj["energy"] = pzemv03->energy();
      serializeJson(doc, state);
      notifyState();
#if WITH_DISPLAY
      printOnDisplay(v, i, p, c);
#endif
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s" CR, tags::sensors, state.c_str());
#endif
    }
    break;
  }
}
