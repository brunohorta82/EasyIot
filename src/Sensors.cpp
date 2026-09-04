#include "Sensors.h"
#include "DeviceLog.h"
#include <algorithm> // std::min, used to bound the HAN clock read
#include "HomeAssistantMqttDiscovery.h"
#include "WebServer.h"
#include "ConfigOnofre.h"
#include "Mqtt.h"
#include "CoreWiFi.h"
#include <DallasTemperature.h>
#include <PZEM004Tv30.h>
#include <PZEM004T.h>
#include "CloudIO.h"
#include <DallasTemperature.h>
#include "HanOnofre.hpp"
#include <SensirionI2cSht4x.h>
#include <LTR303.h>
#include "DHT.h"
#include <NewPing.h>
#include <vector>
#ifdef ESP32
#include <OpenTherm.h>
#include <ld2410.h>
#include "SparkFun_TMF882X_Library.h"
#endif
// OpenTherm ot(1, 2);
extern ConfigOnofre config;

namespace
{
struct DallasBus
{
  unsigned int pin;
  OneWire *oneWire;
  DallasTemperature *dallas;
};

static std::vector<DallasBus> dallasBuses;

static DallasTemperature *getDallasForPin(unsigned int pin)
{
  for (auto &bus : dallasBuses)
  {
    if (bus.pin == pin)
    {
      return bus.dallas;
    }
  }
  DallasBus bus;
  bus.pin = pin;
  bus.oneWire = new OneWire(pin);
  bus.dallas = new DallasTemperature(bus.oneWire);
  bus.dallas->begin();
  dallasBuses.push_back(bus);
  return dallasBuses.back().dallas;
}

#ifdef ESP32
struct LD2450Parser {
  uint8_t buffer[32];
  uint8_t pos = 0;
  
  struct Target {
    int16_t x = 0;
    int16_t y = 0;
    int16_t speed = 0;
    uint16_t resolution = 0;
  } targets[3];
  
  uint8_t target_count = 0;
  
  static int16_t decode_coordinate(uint8_t low_byte, uint8_t high_byte) {
    int16_t coordinate = (high_byte & 0x7F) << 8 | low_byte;
    if ((high_byte & 0x80) == 0) {
      coordinate = -coordinate;
    }
    return coordinate;  // mm
  }

  static int16_t decode_speed(uint8_t low_byte, uint8_t high_byte) {
    int16_t speed = (high_byte & 0x7F) << 8 | low_byte;
    if ((high_byte & 0x80) == 0) {
      speed = -speed;
    }
    return speed * 10;  // mm/s
  }
  
  bool read(HardwareSerial& serial) {
    bool new_data = false;
    while (serial.available()) {
      uint8_t b = serial.read();
      
      // Look for header
      if (pos == 0 && b != 0xAA) continue;
      if (pos == 1 && b != 0xFF) { pos = 0; continue; }
      if (pos == 2 && b != 0x03) { pos = 0; continue; }
      if (pos == 3 && b != 0x00) { pos = 0; continue; }
      
      buffer[pos++] = b;
      
      if (pos == 30) {
        // Verify footer
        if (buffer[28] == 0x55 && buffer[29] == 0xCC) {
          target_count = 0;
          for (int i = 0; i < 3; i++) {
            int offset = 4 + i * 8;
            targets[i].x = decode_coordinate(buffer[offset + 0], buffer[offset + 1]) / 10;
            targets[i].y = decode_coordinate(buffer[offset + 2], buffer[offset + 3]) / 10;
            targets[i].speed = decode_speed(buffer[offset + 4], buffer[offset + 5]) / 10;
            targets[i].resolution = ((buffer[offset + 7] << 8) | buffer[offset + 6]) / 10;
            
            if (targets[i].y > 0) {
              target_count++;
            }
          }
          new_data = true;
        }
        pos = 0;
      }
    }
    return new_data;
  }
};

struct LD2460Parser {
  uint8_t buffer[64];
  uint8_t pos = 0;
  
  struct Target {
    int16_t x = 0;
    int16_t y = 0;
  } targets[5];
  
  uint8_t target_count = 0;
  
  bool read(HardwareSerial& serial) {
    bool new_data = false;
    while (serial.available()) {
      uint8_t b = serial.read();
      
      // Look for header
      if (pos == 0 && b != 0xF4) continue;
      if (pos == 1 && b != 0xF3) { pos = 0; continue; }
      if (pos == 2 && b != 0xF2) { pos = 0; continue; }
      if (pos == 3 && b != 0xF1) { pos = 0; continue; }
      
      buffer[pos++] = b;
      
      if (pos >= 7) {
        uint16_t len = (buffer[6] == 0) ? buffer[5] : buffer[6];
        uint16_t total_packet_len = 7 + len;
        
        if (pos == total_packet_len) {
          if (buffer[pos - 4] == 0xF8 && buffer[pos - 3] == 0xF7 && buffer[pos - 2] == 0xF6 && buffer[pos - 1] == 0xF5) {
            uint16_t target_bytes = len - 4;
            target_count = target_bytes / 4;
            if (target_count > 5) target_count = 5;
            
            for (int i = 0; i < target_count; i++) {
              int offset = 7 + i * 4;
              int16_t raw_x = (int16_t)(buffer[offset + 0] | (buffer[offset + 1] << 8));
              int16_t raw_y = (int16_t)(buffer[offset + 2] | (buffer[offset + 3] << 8));
              
              targets[i].x = raw_x * 10;
              targets[i].y = raw_y * 10;
            }
            new_data = true;
          }
          pos = 0;
        } else if (pos >= 64) {
          pos = 0;
        }
      }
    }
    return new_data;
  }
};
#endif

} // namespace
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

namespace
{
  /**
   * The LDC1612's registers, named as the datasheet names them.
   *
   * Written out rather than pulled from a library: the whole driver is seven writes
   * and two reads, and a dependency for that would be more code to keep than the
   * code it saves.
   */
  namespace LdcWater
  {
    constexpr uint8_t dataMsbCh0 = 0x00;
    constexpr uint8_t dataLsbCh0 = 0x01;
    constexpr uint8_t rcountCh0 = 0x08;
    constexpr uint8_t settleCountCh0 = 0x10;
    constexpr uint8_t clockDividersCh0 = 0x14;
    constexpr uint8_t errorConfig = 0x19;
    constexpr uint8_t config = 0x1A;
    constexpr uint8_t muxConfig = 0x1B;
    constexpr uint8_t driveCurrentCh0 = 0x1E;
    constexpr uint8_t manufacturerId = 0x7E;
  }

  bool ldcWaterWrite(uint8_t reg, uint16_t value)
  {
    Wire.beginTransmission(Discovery::I2C_LDC1612_ADDRESS);
    Wire.write(reg);
    Wire.write(value >> 8);
    Wire.write(value & 0xFF);
    return Wire.endTransmission() == 0;
  }

  bool ldcWaterReadRegister(uint8_t reg, uint16_t &value)
  {
    Wire.beginTransmission(Discovery::I2C_LDC1612_ADDRESS);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
      return false;
    if (Wire.requestFrom(Discovery::I2C_LDC1612_ADDRESS, 2) != 2)
      return false;
    value = ((uint16_t)Wire.read() << 8) | Wire.read();
    return true;
  }
}

bool Sensor::ldcWaterBegin()
{
  uint16_t manufacturer = 0;
  if (!ldcWaterReadRegister(LdcWater::manufacturerId, manufacturer) || manufacturer != 0x5449)
    return false;

  // Resolution over speed. The target turns once per litre — under 1 Hz even at a
  // domestic meter's maximum flow — so the long conversion costs nothing and buys
  // the sensitivity needed to see a target through the meter's cover.
  ldcWaterWrite(LdcWater::config, 0x2801); // sleep while configuring, as required
  ldcWaterWrite(LdcWater::rcountCh0, 0x8000);
  ldcWaterWrite(LdcWater::settleCountCh0, 0x0080);
  ldcWaterWrite(LdcWater::clockDividersCh0, 0x1002);
  ldcWaterWrite(LdcWater::errorConfig, 0x0000);
  ldcWaterWrite(LdcWater::driveCurrentCh0, 0x9000);
  ldcWaterWrite(LdcWater::muxConfig, 0x020C);
  ldcWaterWrite(LdcWater::config, 0x1601); // wake, channel 0, continuous
  return true;
}

long Sensor::ldcWaterRead()
{
  uint16_t msb = 0, lsb = 0;
  if (!ldcWaterReadRegister(LdcWater::dataMsbCh0, msb) ||
      !ldcWaterReadRegister(LdcWater::dataLsbCh0, lsb))
    return -1;
  // The top four bits are error flags, not data: amplitude too high or low, or a
  // conversion that over-ran. Any of them makes the reading meaningless.
  if (msb & 0xF000)
    return -1;
  return ((long)(msb & 0x0FFF) << 16) | lsb;
}

void Sensor::persistWaterTotal()
{
  if (waterUnsavedTurns == 0)
    return;
  waterUnsavedTurns = 0;
  // The whole configuration file, because that is where the sensor's own fields
  // live. Deliberately rare: flash has a finite number of erases and a water meter
  // is expected to keep counting for years.
  config.save();
  deviceLog("contador agua: %d L gravados", (int)waterLiters);
}

void Sensor::publishWaterState()
{
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  state.clear();
  // Litres, because that is the unit the dial counts in and the unit Home Assistant
  // wants for a total_increasing water sensor.
  obj["liters"] = waterLiters;
  obj["flow"] = waterFlow;
  serializeJson(doc, state);
  notifyState();
}

void Sensor::loop()
{
  if (!ready)
    return;

  // Stored configurations predate strict pin-array validation. Never index a
  // malformed vector: mark the sensor failed and leave all GPIO/UART state
  // untouched until the configuration is repaired and the device restarts.
  if (!hasRuntimeInputTopology())
  {
#ifdef DEBUG_ONOFRE
    if (!error)
      Log.error("%s Invalid input topology for %s: expected %u, found %u" CR,
                tags::sensors, uniqueId,
                static_cast<unsigned int>(expectedInputCount(driver)),
                static_cast<unsigned int>(inputs.size()));
#endif
    setError();
    return;
  }

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
#ifdef ESP32
  case TMF882X:
  {
    if (lastRead + delayRead < millis())
    {
      static SparkFun_TMF882X sensor;
      static struct tmf882x_msg_meas_results myResults;
      if (!isInitialized())
      {
        if (!sensor.begin())
        {
#ifdef DEBUG_ONOFRE
          Log.info("%s Error The TMF882X failed to initialize - is the board connected?" CR, tags::sensors);
#endif
          setError();
          return;
        }
      }
      if (sensor.startMeasuring(myResults))
      {
        // print out results
        Serial.println("Measurement:");
        Serial.print("     Result Number: ");
        Serial.print(myResults.result_num);
        Serial.print("  Number of Results: ");
        Serial.println(myResults.num_results);

        for (int i = 0; i < myResults.num_results; ++i)
        {
          Serial.print("       conf: ");
          Serial.print(myResults.results[i].confidence);
          Serial.print(" distance mm: ");
          Serial.print(myResults.results[i].distance_mm);
          Serial.print(" channel: ");
          Serial.print(myResults.results[i].channel);
          Serial.print(" sub_capture: ");
          Serial.println(myResults.results[i].sub_capture);
        }
        Serial.print("     photon: ");
        Serial.print(myResults.photon_count);
        Serial.print(" ref photon: ");
        Serial.print(myResults.ref_photon_count);
        Serial.print(" ALS: ");
        Serial.println(myResults.ambient_light);
        Serial.println();
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        state.clear();
        obj["messure"] = 10;
        serializeJson(doc, state);
        doc.clear();
        notifyState();
#ifdef DEBUG_ONOFRE
        Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
      }
      Serial.println("jj:");
      lastRead = millis();
    }
  }
  break;
#endif
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
  case LDC1612:
  {
    // No delayRead gate: a turn missed is a litre lost for ever, so this reads on
    // every loop. Two 16-bit registers at 100 kHz costs about a millisecond.
    if (!isInitialized())
    {
      if (!ldcWaterBegin())
      {
        // Say so once per second rather than on every loop, and keep trying: a long
        // cable to a meter pit is exactly where an intermittent bus shows up.
        if (lastRead + 1000ul < millis())
        {
          lastRead = millis();
          reInit();
          deviceLog("contador agua: sem resposta i2c");
        }
        return;
      }
      deviceLog("contador agua: %s ligado", FeatureDrivers::LDC1612);
    }

    const long value = ldcWaterRead();
    if (value < 0)
      return;

    if (!waterPrimed)
    {
      waterSlowAverage = value;
      waterPrimed = true;
      waterWindowStart = millis();
      waterResidualMin = waterResidualMax = 0;
    }
    // ~10 s time constant at this read rate. Integer maths: no floats in a loop that
    // runs hundreds of times a second on an 80 MHz part.
    waterSlowAverage += (value - waterSlowAverage) / 256;
    const long residual = value - waterSlowAverage;

    if (residual < waterResidualMin)
      waterResidualMin = residual;
    if (residual > waterResidualMax)
      waterResidualMax = residual;

    // Three seconds: the target turns once per litre, so under 1 Hz even at this
    // meter's maximum flow. A shorter window would not contain a whole revolution.
    if (millis() - waterWindowStart > 3000ul)
    {
      waterAmplitude = waterResidualMax - waterResidualMin;
      waterResidualMin = waterResidualMax = residual;
      waterWindowStart = millis();
    }

    // Below this the window is the sensor breathing, not water. Measured in place
    // with the tap shut: 267 counts against 1,430 with it running.
    constexpr long kWaterNoiseFloor = 800;
    if (waterAmplitude > kWaterNoiseFloor)
    {
      // Trigger on the residual against its own amplitude. Comparing the raw value
      // against the slow average instead lost one turn in seven on a real meter,
      // because a shallow cycle riding on a moving average never reached a fixed
      // offset from it.
      const long trigger = waterAmplitude / 10;
      if (!waterAbove && residual > trigger)
      {
        waterAbove = true;
        const unsigned long now = millis();
        if (waterLastTurn > 0ul && now > waterLastTurn)
        {
          // Litres per minute from the gap between turns. Clamped: the first turn
          // after a long silence would otherwise read as an absurd flow.
          const double minutes = (now - waterLastTurn) / 60000.0;
          waterFlow = minutes > 0.0 ? litersPerTurn / minutes : 0.0;
          if (waterFlow > 1000.0)
            waterFlow = 0.0;
        }
        waterLastTurn = now;
        waterLiters += litersPerTurn;
        waterUnsavedTurns++;
        publishWaterState();
        // A long continuous draw — filling a pool — would otherwise reach the next
        // checkpoint only when it stopped, and lose everything to a power cut.
        if (waterUnsavedTurns >= 100)
          persistWaterTotal();
      }
      else if (waterAbove && residual < -trigger)
      {
        waterAbove = false;
      }
    }

    // No turn for twenty seconds means the tap is shut, not that the flow held.
    if (waterLastTurn > 0ul && millis() - waterLastTurn > 20000ul && waterFlow != 0.0)
    {
      waterFlow = 0.0;
      publishWaterState();
      // The end of a draw is the natural moment to write the total: once per use of
      // water rather than once per litre. A domestic day is a handful of writes.
      persistWaterTotal();
    }
  }
  break;
  case SHT4X:
  {

    if (lastRead + delayRead < millis())
    {

      static SensirionI2cSht4x sensor;
      if (!isInitialized())
      {
        sensor.begin(Wire, Discovery::I2C_SHT4X_ADDRESS);
        sensor.softReset();
        lastRead = millis();
        return;
      }
      float temperature = 0.0;
      float humidity = 0.0;
      if (!sensor.measureLowestPrecision(temperature, humidity))
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
      DallasTemperature *dallas = getDallasForPin(inputs[0]);

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
  break;

  case HAN:
  {

    if (lastRead + delayRead < millis())
    {
      static ModbusMaster *modbus;
#ifdef ESP32
      if (!isInitialized())
      {
        modbus = new ModbusMaster();
        Serial1.begin(9600, SERIAL_8N1,
                      SensorRuntimePins::HAN_RX,
                      SensorRuntimePins::HAN_TX);
        modbus->begin(1, Serial1);
      }
#endif

#ifdef ESP8266
      static SoftwareSerial softwareSerial = SoftwareSerial(inputs[1], inputs[0]);
      static SensirionI2cSht4x sensor;

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
        // This bundled ModbusMaster stores the last valid response index rather
        // than a word count. Add one, then clamp to the six-word clock buffer.
        // Using `i < available()` here drops the final word.
        std::array<uint16_t, 6> buffer{};
        const size_t words = std::min<size_t>(
            static_cast<size_t>(modbus->available()) + 1U, buffer.size());
        for (size_t i = 0; i < words; ++i)
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
        // Stop the read cycle here. Without this, the next guarded read waits for
        // another full Modbus timeout before it records the error.
        setError();
#ifdef DEBUG_ONOFRE
        Log.info("%s HAN  Error: %d. " CR, tags::sensors, rsl);
#endif
      }
      // A single register may fail without costing the rest of the payload:
      // skip that field and carry on. Two failures in a row mean the meter has
      // gone quiet, so the pass stops rather than blocking 2 s on each of the
      // remaining registers — which is what reset the device before 9.163.
      // The clock read above already counts: if it failed, the meter is halfway
      // to being declared silent and one more miss ends the pass.
      int hanMisses = error ? 1 : 0;
      auto hanRead = [&](uint16_t address, uint8_t words) {
        if (hanMisses >= 2)
          return false;
        if (modbus->readInputRegisters(address, words) == modbus->ku8MBSuccess)
        {
          hanMisses = 0;
          return true;
        }
        hanMisses++;
        return false;
      };
      if (hanRead(INSTANTANEOUS_VOLTAGE_L1, 2))
      {
        obj["voltage"] = modbus->getResponseBuffer(0) / 10.0;
        obj["current"] = modbus->getResponseBuffer(1) / 10.0;
        delay(50);
      }
      // A miss here costs only this field; hanRead() tracks it.
      if (hanRead(ACTIVE_ENERGY_IMPORT_PLUS_A, 2))
      {
        obj["powerImport"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        obj["powerExport"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        delay(50);
      }
      // A miss here costs only this field; hanRead() tracks it.
      if (hanRead(RATE_1_CONTRACT_1_ACTIVE_ENERGY_PLUS_A, 3))
      {
        obj["rate1"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        obj["rate2"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        obj["rate3"] = (modbus->getResponseBuffer(5) | modbus->getResponseBuffer(4) << 16) / 1000.0;
        delay(50);
      }
      // A miss here costs only this field; hanRead() tracks it.
      if (hanRead(INSTANTANEOUS_ACTIVE_POWER_PLUS_SUM_OF_ALL_PHASES, 3))
      {
        obj["power"] = modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16;
        obj["export"] = modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16;
        obj["powerFactor"] = modbus->getResponseBuffer(4) / 1000.0;
        delay(50);
      }
      // A miss here costs only this field; hanRead() tracks it.
      if (hanRead(INSTANTANEOUS_FREQUENCY, 1))
      {
        obj["frequency"] = modbus->getResponseBuffer(0) / 10.0;
        delay(50);
      }
      // A miss here costs only this field; hanRead() tracks it.
      if (hanRead(CURRENTLY_ACTIVE_TARIFF, 1))
      {
        obj["tarif"] = modbus->getResponseBuffer(0) >> 8;
        delay(50);
      }
      // A miss here costs only this field; hanRead() tracks it.
      if (hanRead(ACTIVE_DEMAND_CONTROL_THRESHOLD_T1, 3))
      {
        obj["demandControlT1"] = (modbus->getResponseBuffer(1) | modbus->getResponseBuffer(0) << 16) / 1000.0;
        obj["demandControlT2"] = (modbus->getResponseBuffer(3) | modbus->getResponseBuffer(2) << 16) / 1000.0;
        obj["demandControlT3"] = (modbus->getResponseBuffer(5) | modbus->getResponseBuffer(4) << 16) / 1000.0;
        delay(50);
      }
      // A miss here costs only this field; hanRead() tracks it.

      if (hanMisses < 2 && modbus->readLastProfile(0x06, 0x01) == modbus->ku8MBSuccess)
      {
        std::array<uint16_t, 6> buffer{};
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

      // Escalate once, after the pass: two misses in a row is a meter that has
      // stopped answering, and that is what should pause polling. A single bad
      // register must not, or every field after it disappears from Home
      // Assistant — which is how demandControl was lost between 9.163 and 9.168.
      if (hanMisses >= 2)
      {
        setError();
      }
      obj["errorCount"] = errorCounter;
      if (!error)
      {
        obj["status"] = "HAN OK";
      }
      obj["signal"] = WiFi.RSSI();
      obj["comm"] = "SWSERIAL_8N1";
#ifdef ESP8266
      if (!sensor.measureLowestPrecision(temperature, humidity))
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
    if (context == nullptr)
    {
      context = new ld2410();
    }
    ld2410* radar = (ld2410*)context;
    if (initialized)
    {
      radar->read();
    }
    if (lastRead + delayRead < millis())
    {
      if (!isInitialized())
      {
        Serial1.begin(256000, SERIAL_8N1, inputs[0], inputs[1]);
        delay(500);
        if (!radar->begin(Serial1))
        {
#ifdef DEBUG_ONOFRE
          Log.error("Ld2410 error " CR, tags::sensors);
#endif
          return;
        }
      }
      lastRead = millis();

      if (radar->isConnected())
      {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        state.clear();

        if (radar->presenceDetected())
        {
          lastBinaryState = true;
          obj["occupancy"] = Payloads::presenceOnPayload;
          if (radar->stationaryTargetDetected())
          {
            obj["stationaryTargetDistance"] = radar->stationaryTargetDistance();
            obj["stationaryTargetEnergy"] = radar->stationaryTargetEnergy();
          }
          else
          {
            obj["stationaryTargetDistance"] = 0;
            obj["stationaryTargetEnergy"] = 0;
          }
          if (radar->movingTargetDetected())
          {
            obj["movingTargetDistance"] = radar->movingTargetDistance();
            obj["movingTargetEnergy"] = radar->movingTargetEnergy();
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
          if (lastBinaryState == false)
            return;
          obj["stationaryTargetDistance"] = 0;
          obj["stationaryTargetEnergy"] = 0;
          obj["occupancy"] = Payloads::presenceOffPayload;
          obj["movingTargetEnergy"] = 0;
          obj["movingTargetDistance"] = 0;
          obj["motion"] = Payloads::motionOffPayload;
          lastBinaryState = false;
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
  case LD2450:
  {
    if (context == nullptr)
    {
      context = new LD2450Parser();
    }
    LD2450Parser* parser = (LD2450Parser*)context;
    
    if (initialized)
    {
      if (parser->read(Serial1))
      {
        if (lastRead + delayRead < millis())
        {
          lastRead = millis();
          JsonDocument doc;
          JsonObject obj = doc.to<JsonObject>();
          state.clear();
          
          obj["count"] = parser->target_count;
          obj["motion"] = (parser->target_count > 0) ? Payloads::motionOnPayload : Payloads::motionOffPayload;
          obj["occupancy"] = (parser->target_count > 0) ? Payloads::presenceOnPayload : Payloads::presenceOffPayload;
          
          for (int i = 0; i < 3; i++)
          {
            String prefix = "t" + String(i + 1) + "_";
            obj[prefix + "x"] = parser->targets[i].x;
            obj[prefix + "y"] = parser->targets[i].y;
            obj[prefix + "s"] = parser->targets[i].speed;
            obj[prefix + "r"] = parser->targets[i].resolution;
          }
          
          serializeJson(doc, state);
          doc.clear();
          notifyState();
#ifdef DEBUG_ONOFRE
          Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
        }
      }
    }
    
    if (!isInitialized())
    {
      Serial1.begin(256000, SERIAL_8N1, inputs[0], inputs[1]);
      delay(500);
      lastRead = millis();
    }
    break;
  }
  case LD2460:
  {
    if (context == nullptr)
    {
      context = new LD2460Parser();
    }
    LD2460Parser* parser = (LD2460Parser*)context;
    
    if (initialized)
    {
      if (parser->read(Serial1))
      {
        if (lastRead + delayRead < millis())
        {
          lastRead = millis();
          JsonDocument doc;
          JsonObject obj = doc.to<JsonObject>();
          state.clear();
          
          obj["count"] = parser->target_count;
          obj["motion"] = (parser->target_count > 0) ? Payloads::motionOnPayload : Payloads::motionOffPayload;
          obj["occupancy"] = (parser->target_count > 0) ? Payloads::presenceOnPayload : Payloads::presenceOffPayload;
          
          for (int i = 0; i < 5; i++)
          {
            String prefix = "t" + String(i + 1) + "_";
            obj[prefix + "x"] = parser->targets[i].x;
            obj[prefix + "y"] = parser->targets[i].y;
          }
          
          serializeJson(doc, state);
          doc.clear();
          notifyState();
#ifdef DEBUG_ONOFRE
          Log.notice("%s %s " CR, tags::sensors, state.c_str());
#endif
        }
      }
    }
    
    if (!isInitialized())
    {
      Serial1.begin(115200, SERIAL_8N1, inputs[0], inputs[1]);
      delay(500);
      lastRead = millis();
    }
    break;
  }
#endif
  case PZEM_004T_V03:
    if (lastRead + delayRead < millis())
    {
      static int pzemCount = 0;
#if defined(ESP8266)
      if (!isInitialized())
      {
        // A scan may append to config.sensors. Queue it so the main loop runs
        // discovery only after this range-for and its feature lease end.
        config.requestI2cDiscovery();
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
      // Serviced here because the meter is only reachable while this object owns
      // the bus; the counter reads zero from the next poll onwards.
      if (takeEnergyResetRequest())
      {
        const bool ok = pzemv03.resetEnergy();
#ifdef DEBUG_ONOFRE
        Log.notice("%s Energy reset for %s: %s" CR, tags::sensors, uniqueId, ok ? "ok" : "failed");
#else
        (void)ok;
#endif
      }
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
  case PZEM_004T_V01:

    if (lastRead + delayRead < millis())
    {

#ifdef ESP8266
      PZEM004T pzem = PZEM004T(inputs[0], inputs[1]);
#endif
#ifdef ESP32
      static PZEM004T pzem = PZEM004T(&Serial1,
                                      SensorRuntimePins::PZEM_V01_RX,
                                      SensorRuntimePins::PZEM_V01_TX);
#endif
      IPAddress ip(192, 168, 1, 1);
#if defined(ESP8266)
      if (!isInitialized())
      {
        pzem.setAddress(ip);
      }

#endif
#if defined(ESP32)

      if (!isInitialized())
      {

        pzem.setAddress(ip);
      }

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
      float e = 0.0;

      v = pzem.voltage(ip);
      if (isnan(v))
      {
        setError();
        obj["error"] = true;
      }
      else
      {
        c = pzem.current(ip);
        p = pzem.power(ip);
        e = pzem.energy(ip);
        obj["voltage"] = v;
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
          config.display->printf("%0.fV %0.fA %0.fHz", v, c, f);
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

#ifdef DEBUG_ONOFRE
      Log.notice("%s %s" CR, tags::sensors, state.c_str());
#endif
    }
    break;
#ifndef ESP32
  case TMF882X:
  case LD2410:
    // These drivers are ESP32-only and intentionally no-op on ESP8266 builds.
    break;
#endif
  default:
    break;
  }
}
