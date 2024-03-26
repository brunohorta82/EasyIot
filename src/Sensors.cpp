#include "Sensors.h"
#include "HomeAssistantMqttDiscovery.h"
#include "WebServer.h"
#include "ConfigOnofre.h"
#include "Mqtt.h"
#include "CoreWiFi.h"
#include <DallasTemperature.h>
#include <PZEM004Tv30.h>
#include "CloudIO.h"
#include <DallasTemperature.h>
#include "HanOnofre.hpp"
#include <SensirionI2CSht4x.h>
#include <LTR303.h>
#include "DHT.h"
#include <NewPing.h>
#ifdef ESP32
#include <OpenTherm.h>
#include <ld2410.h>
#endif
// OpenTherm ot(1, 2);
extern ConfigOnofre config;
void Sensor::notifyState()
{
  // Notify by MQTT/Homeassistant
  if (mqttConnected())
  {
    publishOnMqtt(readTopic, state.c_str(), false);
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
  if (!wifiConnected())
  {
    return;
  }
  if (error)
  {
    if (lastErrorTimestamp + constantsConfig::DEFAULT_TIME_SENSOR_ERROR_CLEAR < millis())
    {
      error = false;
#ifdef HAN_MODE
      if (errorCounter >= 5)
      {
        config.requestRestart();
        return;
      }
#endif
#ifdef DEBUG_ONOFRE
      Log.info("%s Error automatically cleaned. System will try again." CR, tags::sensors);
#endif
    }
    return;
  }

  switch (driver)
  {
  case INVALID_SENSOR:
    return;
  case TMF882X:
  {
  }
  break;
  case DHT_11:
  case DHT_21:
  case DHT_22:
  {
    if (lastRead + delayRead < millis())
    {
      static DHT *dht;
      if (!isInitialized())
      {
        dht = new DHT(inputs[0], driver - 100);
        dht->begin();
      }
      float temperature = dht->readTemperature();
      float humidity = dht->readHumidity();
      lastRead = millis();
      if (isnan(humidity) || isnan(temperature))
      {
#ifdef DEBUG_ONOFRE
        Log.notice("%s DHT ERROR " CR, tags::sensors);
#endif
        return;
      }
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      obj["temperature"] = temperature;
      obj["humidity"] = humidity;
      serializeJson(doc, state);
      doc.clear();
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;
  case PIR:
  {
    if (lastRead + delayRead < millis())
    {
      if (!isInitialized())
      {
        configPIN(inputs[0], INPUT);
      }
      lastRead = millis();
      int currentState = readPINToInt(inputs[0]);
      if (lastBinaryState == currentState)
        return;
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      obj["motion"] = currentState ? Payloads::motionOnPayload : Payloads::motionOffPayload;
      serializeJson(doc, state);
      doc.clear();
      notifyState();
      lastBinaryState = currentState;
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;
  case RAIN:
  {
    if (lastRead + delayRead < millis())
    {
      if (!isInitialized())
      {
        configPIN(inputs[0], INPUT_PULLUP);
      }

      lastRead = millis();
      int currentState = readPINToInt(inputs[0]);
      if (lastBinaryState == currentState)
        return;
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      obj["rain"] = currentState ? Payloads::rainOnPayload : Payloads::rainOffPayload;
      serializeJson(doc, state);
      doc.clear();
      notifyState();
      lastBinaryState = currentState;

#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;
  case DOOR:
  case WINDOW:
  {
    if (lastRead + delayRead < millis())
    {
      if (!isInitialized())
      {
        configPIN(inputs[0], INPUT_PULLUP);
      }
      lastRead = millis();
      int currentState = readPINToInt(inputs[0]);
      if (lastBinaryState == currentState)
        return;
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      obj["state"] = currentState ? Payloads::windowDoornOnPayload : Payloads::windowDoornOffPayload;
      serializeJson(doc, state);
      doc.clear();
      notifyState();
      lastBinaryState = currentState;
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;
  case SHT4X:
  {

    if (lastRead + delayRead < millis())
    {

      static SensirionI2CSht4x sensor;
      if (!isInitialized())
      {
        sensor.begin(Wire, Discovery::I2C_SHT4X_ADDRESS);
        sensor.softReset();
        lastRead = millis();
        return;
      }
      float temperature = 0.0;
      float humidity = 0.0;
      if (sensor.measureLowestPrecision(temperature, humidity) == NO_ERROR)
      {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        state.clear();
        obj["temperature"] = temperature;
        obj["humidity"] = humidity;
        serializeJson(doc, state);
        doc.clear();
        notifyState();
        lastRead = millis();
#ifdef DEBUG_ONOFRE
        Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
      }
    }
  }
  break;
  case LTR303X:
  {
    if (lastRead + delayRead < millis())
    {
      lastRead = millis();
      static int gain = 6;
      static LTR303 light;
      if (!isInitialized())
      {
        light.begin();
        light.setControl(gain, false, false);
        light.setMeasurementRate(1, 3);
        light.setPowerUp();
        return;
      }

      unsigned int data0, data1;
      double lux;
      if (light.getData(data0, data1))
      {
        if (light.getLux(gain, 0, data0, data1, lux))
        {
          if (millis() < 25000)
          {
#ifdef DEBUG_ONOFRE
            Log.notice("%s LTR303 Calibration " CR, tags::sensors);
#endif
            return;
          }
          JsonDocument doc;
          JsonObject obj = doc.to<JsonObject>();
          state.clear();
          obj["gain"] = gain;
          obj["ch0"] = data0;
          obj["ch1"] = data1;
          obj["lux"] = lux;
          serializeJson(doc, state);
          doc.clear();
          notifyState();
#ifdef DEBUG_ONOFRE
          Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
        }
      }
    }
  }
  break;
  case DS18B20:
  {
    if (lastRead + delayRead < millis())
    {
      static DallasTemperature *dallas;
      if (!isInitialized())
      {
        dallas = new DallasTemperature(new OneWire(inputs[0]));
        dallas->begin();
      }

      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      dallas->requestTemperatures();
      obj["temperature"] = dallas->getTempCByIndex(0);
      serializeJson(doc, state);
      doc.clear();
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;
  case HCSR04:
  {
    if (lastRead + delayRead < millis())
    {
      NewPing sonar(inputs[0], inputs[1], 350);

      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      unsigned int distance = sonar.ping_cm();
      obj["distance"] = distance;
      serializeJson(doc, state);
      doc.clear();
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }

  case HAN:
  case HAN_8N2:
  {

    if (lastRead + delayRead < millis())
    {
      static ModbusMaster *modbus;
#ifdef ESP32
      if (!isInitialized())
      {
        modbus = new ModbusMaster();
        Serial1.begin(9600, SERIAL_8N1, 5, 6);
        modbus->begin(1, Serial1);
      }
#endif

#ifdef ESP8266
      static SoftwareSerial softwareSerial = SoftwareSerial(inputs[1], inputs[0]);
      static SensirionI2CSht4x sensor;

      float temperature = 0.0;
      float humidity = 0.0;

      if (!isInitialized())
      {
        modbus = new ModbusMaster();
        Wire.begin(DefaultPins::SDA, DefaultPins::SCL);
        sensor.begin(Wire, Discovery::I2C_SHT4X_ADDRESS);
        sensor.softReset();
        softwareSerial.begin(9600, SWSERIAL_8N1);
        modbus->begin(1, softwareSerial);
      }
#endif
      lastRead = millis();
      JsonDocument
          doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      modbus->clearResponseBuffer();
      uint8_t rsl = modbus->readInputRegisters(CLOCK, 1);
      if (rsl == modbus->ku8MBSuccess)
      {
        std::array<uint16_t, 6> buffer;
        for (size_t i = 0; i <= modbus->available(); ++i)
        {
          buffer[i] = modbus->getResponseBuffer(i);
        }
        han_clock_t clock{.buffer = buffer};
        char strftime_buf[64];
        sprintf(strftime_buf, "%d-%02d-%02dT%02d:%02d:%02d", clock.year, clock.month, clock.day_of_month, clock.hour, clock.minute, clock.second);
        obj["dateTime"] = strftime_buf;
        delay(100);
      }
      else
      {
        if (rsl == 0x81)
        {
          obj["status"] = "Acesso negado contacte a E-Redes";
        }
        else if (rsl == 0xE2)
        {
          obj["status"] = "Problemas na comunicação";
        }
        else if (rsl == 0xE1)
        {
          obj["status"] = "Função Inválida";
        }
        else if (rsl == 0xE0)
        {
          obj["status"] = "Endereço Inválido";
        }
        else
        {
          obj["status"] = "Erro desconhecido";
        }
        //  setError();
#ifdef DEBUG_ONOFRE
        Log.info("%s HAN  Error: %d. " CR, tags::sensors, rsl);
#endif
      }
      if (!error && modbus->readInputRegisters(INSTANTANEOUS_VOLTAGE_L1, 2) == modbus->ku8MBSuccess)
      {
        obj["voltage"] = modbus->getResponseBuffer(0) / 10.0;
        obj["current"] = modbus->getResponseBuffer(1) / 10.0;
        delay(50);
      }
      else
      {
        setError();
      }
      if (!error && modbus->readInputRegisters(ACTIVE_ENERGY_IMPORT_PLUS_A, 2) == modbus->ku8MBSuccess)
      {
        obj["powerImport"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        obj["powerExport"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        delay(50);
      }
      else
      {
        setError();
      }
      if (!error && modbus->readInputRegisters(RATE_1_CONTRACT_1_ACTIVE_ENERGY_PLUS_A, 3) == modbus->ku8MBSuccess)
      {
        obj["rate1"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        obj["rate2"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        obj["rate3"] = (modbus->getResponseBuffer(5) | modbus->getResponseBuffer(4) << 16) / 1000.0;
        delay(50);
      }
      else
      {
        setError();
      }
      if (!error && modbus->readInputRegisters(INSTANTANEOUS_ACTIVE_POWER_PLUS_SUM_OF_ALL_PHASES, 3) == modbus->ku8MBSuccess)
      {
        obj["power"] = modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16;
        obj["export"] = modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16;
        obj["powerFactor"] = modbus->getResponseBuffer(4) / 1000.0;
        delay(50);
      }
      else
      {
        setError();
      }
      if (!error && modbus->readInputRegisters(INSTANTANEOUS_FREQUENCY, 1) == modbus->ku8MBSuccess)
      {
        obj["frequency"] = modbus->getResponseBuffer(0) / 10.0;
        delay(50);
      }
      else
      {
        setError();
      }
      if (!error && modbus->readInputRegisters(CURRENTLY_ACTIVE_TARIFF, 1) == modbus->ku8MBSuccess)
      {
        obj["tarif"] = modbus->getResponseBuffer(0) >> 8;
        delay(50);
      }
      else
      {
        setError();
      }
      if (!error && modbus->readInputRegisters(ACTIVE_DEMAND_CONTROL_THRESHOLD_T1, 3) == modbus->ku8MBSuccess)
      {
        obj["demandControlT1"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        obj["demandControlT2"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        obj["demandControlT3"] = (modbus->getResponseBuffer(5) | modbus->getResponseBuffer(4) << 16) / 1000.0;
        delay(50);
      }
      else
      {
        setError();
      }

      if (modbus->readLastProfile(0x06, 0x01) == modbus->ku8MBSuccess)
      {
        std::array<uint16_t, 6> buffer;
        for (size_t i = 0; i <= 3; ++i)
        {
          buffer[i] = modbus->getResponseBuffer(i);
        }
        han_clock_t clock{.buffer = buffer};
        char strftime_buf[64];
        sprintf(strftime_buf, "%d-%02d-%02dT%02d:%02d:%02d", clock.year, clock.month, clock.day_of_month, clock.hour, clock.minute, clock.second);
        obj["dateProfile"] = strftime_buf;
        obj["amr"] = modbus->getResponseBuffer(6);
        obj["activeEnergyImport"] = modbus->getResponseBuffer(8) |
                                    modbus->getResponseBuffer(7) << 16;
        obj["reactiveEnergyRC"] = modbus->getResponseBuffer(10) |
                                  modbus->getResponseBuffer(9) << 16;
        obj["reactiveEnergyRI)"] = modbus->getResponseBuffer(12) |
                                   modbus->getResponseBuffer(11) << 16;
        obj["activeEnergyExport)"] = modbus->getResponseBuffer(14) |
                                     modbus->getResponseBuffer(13) << 16;
      }

      obj["errorCount"] = errorCounter;
      if (!error)
      {
        obj["status"] = "HAN OK";
      }
      obj["signal"] = WiFi.RSSI();
      obj["comm"] = "SWSERIAL_8N1";
#ifdef ESP8266
      if (sensor.measureLowestPrecision(temperature, humidity) == NO_ERROR)
      {
        obj["temperature"] = round(temperature);
        obj["humidity"] = round(humidity);
      }
#endif
      serializeJson(obj, state);
      doc.clear();
      notifyState();
#ifdef DEBUG_ONOFRE
      Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
    }
  }
  break;

#ifdef ESP32
  case LD2410:
  {
    static ld2410 radar;
    if (initialized)
    {
      radar.read();
    }
    if (lastRead + delayRead < millis())
    {
      if (!isInitialized())
      {
        Serial1.begin(256000, SERIAL_8N1, inputs[0], inputs[1]);
        delay(500);
        if (radar.begin(Serial1))
        {
#ifdef DEBUG_ONOFRE
          Log.error("Ld2410 error " CR, tags::sensors);
#endif
          return;
        }
      }
      lastRead = millis();

      if (radar.isConnected())
      {
        JsonDocument
            doc;
        JsonObject obj = doc.to<JsonObject>();
        state.clear();
        if (radar.presenceDetected())
        {
          bool currentState = readPINToInt(inputs[0]);
          if (lastBinaryState == currentState)
            return;
          lastBinaryState = currentState;
          obj["presence"] = Payloads::presenceOnPayload;
          if (radar.stationaryTargetDetected())
          {
            obj["stationaryTargetDistance"] = radar.stationaryTargetDistance();
            obj["stationaryTargetEnergy"] = radar.stationaryTargetEnergy();
          }
          else
          {
            obj["stationaryTargetDistance"] = 0;
            obj["stationaryTargetEnergy"] = 0;
          }
          if (radar.movingTargetDetected())
          {
            obj["movingTargetDistance"] = radar.movingTargetDistance();
            obj["movingTargetEnergy"] = radar.movingTargetEnergy();
            obj["motion"] = Payloads::motionOnPayload;
          }
          else
          {
            obj["movingTargetEnergy"] = 0;
            obj["movingTargetDistance"] = 0;
            obj["motion"] = Payloads::motionOffPayload;
          }
        }
        else
        {
          obj["stationaryTargetDistance"] = 0;
          obj["stationaryTargetEnergy"] = 0;
          obj["presence"] = Payloads::presenceOffPayload;
          obj["movingTargetEnergy"] = 0;
          obj["movingTargetDistance"] = 0;
          obj["motion"] = Payloads::motionOffPayload;
        }
        serializeJson(doc, state);
        doc.clear();
        notifyState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
      }
    }
    break;
  }

#else
  case LD2410:
    break;
#endif
  case PZEM_004T_V03:
    if (lastRead + delayRead < millis())
    {
      static int pzemCount = 0;
#if defined(ESP8266)
      if (!isInitialized())
      {
        config.i2cDiscovery();
      }
      SoftwareSerial softwareSerial = SoftwareSerial(inputs[0], inputs[1]);
      PZEM004Tv30 pzemv03(softwareSerial);
#endif
#if defined(ESP32)
      if (!isInitialized())
      {
      }
      PZEM004Tv30 pzemv03(Serial1, inputs[0], inputs[1], hwAddress);
#endif
      lastRead = millis();
      JsonDocument doc;
      JsonObject obj = doc.to<JsonObject>();
      state.clear();
      lastRead = millis();
      float v = 0.0;
      float f = 0.0;
      float c = 0.0;
      float p = 0.0;
      float pf = 0.0;
      float e = 0.0;

      v = pzemv03.voltage();
      if (isnan(v))
      {
        setError();
        obj["error"] = true;
      }
      else
      {
        f = pzemv03.frequency();
        pf = pzemv03.pf();
        c = pzemv03.current();
        p = pzemv03.power();
        e = pzemv03.energy();
        obj["addr"] = pzemv03.getAddress();
        obj["voltage"] = v;
        obj["frequency"] = f;
        obj["powerFactor"] = pf;
        obj["current"] = c;
        obj["power"] = p;
        obj["energy"] = e;
      }
      serializeJson(doc, state);
      doc.clear();
      notifyState();
      if (config.display != NULL)
      {
        config.display->clearDisplay();
        config.display->setTextSize(1);              // Normal 1:1 pixel scale
        config.display->setTextColor(SSD1306_WHITE); // Draw white text
        config.display->setCursor(0, 0);
        if (error)
        {
          config.display->printf("ERROR");
          config.display->display();
        }
        else
        {

          if (pzemCount == 0 || pzemCount > 1)
          {
            int cl = 16;
            int offset = 56 - cl;
            config.display->setCursor(0, 9);
            config.display->print("V");
            config.display->setCursor(0, 18);
            config.display->print("A");
            config.display->setCursor(0, 27);
            config.display->print("W ");
            config.display->setCursor(0, 36);
            config.display->print("PF");
            config.display->setCursor(0, 45);
            config.display->print("Hz");
            config.display->setCursor(0, 54);
            config.display->print("E");
            for (auto s : config.sensors)
            {
              if (s.driver != SensorDriver::PZEM_004T_V03)
                continue;
              pzemCount++;
              DeserializationError error = deserializeJson(doc, s.state);
              if (error != DeserializationError::Ok)
                continue;
              if (!(doc["error"] | false))
              {
                config.display->setCursor(cl, 0);
                config.display->printf("CT%d", doc["addr"] | 0);
                config.display->setCursor(cl, 9);
                config.display->printf("%0.f", doc["voltage"] | 0.0);
                config.display->setCursor(cl, 18);
                config.display->printf("%0.f", doc["current"] | 0.0);
                config.display->setCursor(cl, 27);
                config.display->printf("%.0f ", doc["power"] | 0.0);
                config.display->setCursor(cl, 36);
                config.display->printf("%0.2f", doc["powerFactor"] | 0.0);
                config.display->setCursor(cl, 45);
                config.display->printf("%0.f", doc["frequency"] | 0.0);
                config.display->setCursor(cl, 54);
                config.display->printf("%.0f", doc["energy"] | 0.0);
              }
              cl = cl + offset;
            }
            config.display->display();
            doc.clear();
          }
          else
          {
            config.display->printf("%0.fV %0.fA %0.2fPF %0.fHz", v, c, pf, f);
            config.display->setCursor(0, 46);
            config.display->printf("%.0fKwh", e);
            config.display->setTextSize(2);
            int16_t x1;
            int16_t y1;
            uint16_t width;
            uint16_t height;
            String power = String(p) + "W";
            config.display->getTextBounds(power.c_str(), 0, 0, &x1, &y1, &width, &height);
            config.display->setCursor((128 - width) / 2, ((64 - height) / 2) - 4);
            config.display->println(power.c_str());
            config.display->display();
          }
        }
      }

#ifdef DEBUG_ONOFRE
      Log.notice("%s %s" CR, tags::sensors, state.c_str());
#endif
    }
    break;
  }
}
