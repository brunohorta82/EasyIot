#include "Sensors.h"
#include "HomeAssistantMqttDiscovery.h"
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
#include <SHT3x.h>
extern ConfigOnofre config;
void Sensor::notifyState()
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

void Sensor::loop()
{

  switch (type)
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
      DHT_nonblocking *dht;
      if (!isInitialized())
      {
        dht = new DHT_nonblocking(inputs[0], type);
      }
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
      SHT3x sensor;
      if (!isInitialized())
      {
#ifdef ESP32
        Wire.setPins(inputs[0], inputs[1]);
#endif
#ifdef ESP8266
        Wire.pins(inputs[0], inputs[1]);
#endif
        sensor.Begin();
      }
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
      DallasTemperature *dallas;
      if (!isInitialized())
      {
        dallas = new DallasTemperature(new OneWire(inputs[0]));
        dallas->begin();
      }

      StaticJsonDocument<256> doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      dallas->requestTemperatures();
      obj["temperature"] = dallas->getTempCByIndex(0);
      serializeJson(doc, state);
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  case HAN:
  {

    if (lastRead + delayRead < millis())
    {

#ifdef ESP32
      if (!isInitialized())
      {
        modbus = new ModbusMaster();
        Serial1.begin(9600, SERIAL_8N2, inputs[0], inputs[1]);
        modbus->begin(1, Serial1);
      }
#endif
#ifdef ESP8266
      if (!isInitialized())
      {
        modbus = new ModbusMaster();
        Serial.begin(9600, SERIAL_8N2);
        Serial.pins(inputs[0], inputs[1]);
        modbus->begin(1, Serial);
      }
#endif
      lastRead = millis();
      StaticJsonDocument<300> doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      modbus->clearResponseBuffer();
      if (modbus->readInputRegisters(CLOCK, 1) == modbus->ku8MBSuccess)
      {
        std::array<uint16_t, 6> buffer;
        for (size_t i = 0; i <= modbus->available(); ++i)
        {
          buffer[i] = modbus->getResponseBuffer(i);
        }
        han_clock_t clock{.buffer = buffer};
        char strftime_buf[64];
        sprintf(strftime_buf, "%d-%02d-%02dT%02d:%02d:%02d.000", clock.year, clock.day_of_month, clock.day_of_month, clock.hour, clock.minute, clock.second);
        doc["dateTime"] = strftime_buf;
      }
      if (modbus->readInputRegisters(INSTANTANEOUS_VOLTAGE_L1, 2) == modbus->ku8MBSuccess)
      {
        doc["voltage"] = modbus->getResponseBuffer(0) / 10.0;
        doc["current"] = modbus->getResponseBuffer(1) / 10.0;
      }
      if (modbus->readInputRegisters(ACTIVE_ENERGY_IMPORT_PLUS_A, 2) == modbus->ku8MBSuccess)
      {
        doc["powerImport"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        doc["powerExport"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
      }
      if (modbus->readInputRegisters(RATE_1_CONTRACT_1_ACTIVE_ENERGY_PLUS_A, 3) == modbus->ku8MBSuccess)
      {
        doc["rate1"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        doc["rate2"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        doc["rate3"] = (modbus->getResponseBuffer(5) | modbus->getResponseBuffer(4) << 16) / 1000.0;
      }
      if (modbus->readInputRegisters(INSTANTANEOUS_ACTIVE_POWER_PLUS_SUM_OF_ALL_PHASES, 3) == modbus->ku8MBSuccess)
      {
        doc["power"] = modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16;
        doc["export"] = modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16;
        doc["powerFactor"] = modbus->getResponseBuffer(4) / 1000.0;
      }
      if (modbus->readInputRegisters(INSTANTANEOUS_FREQUENCY, 1) == modbus->ku8MBSuccess)
      {
        doc["frequency"] = modbus->getResponseBuffer(0) / 10.0;
      }
      if (modbus->readInputRegisters(CURRENTLY_ACTIVE_TARIFF, 1) == modbus->ku8MBSuccess)
      {
        doc["tarif"] = modbus->getResponseBuffer(0) >> 8;
      }
      if (modbus->readInputRegisters(ACTIVE_DEMAND_CONTROL_THRESHOLD_T1, 3) == modbus->ku8MBSuccess)
      {
        doc["demandControlT1"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        doc["demandControlT2"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        doc["demandControlT3"] = (modbus->getResponseBuffer(5) | modbus->getResponseBuffer(4) << 16) / 1000.0;
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
      PZEM004Tv30 *pzemv03;
#if defined(ESP8266)
      if (!isInitialized())
      {
        SoftwareSerial softwareSerial = SoftwareSerial(inputs[0], inputs[1]);
        pzemv03 = new PZEM004Tv30(softwareSerial);
      }
#endif
#if defined(ESP32)
      if (!isInitialized())
      {
        pzemv03 = new PZEM004Tv30(Serial2, inputs[0], inputs[1]);
      }
#endif

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
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s" CR, tags::sensors, state.c_str());
#endif
    }
    break;
  }
}
