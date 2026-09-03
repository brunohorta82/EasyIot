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
  // Asked for, not done here. Writing the configuration file takes hundreds of
  // milliseconds of LittleFS, and this runs inside loopSensors() with the feature
  // lease held — every web request during the write would be answered "busy". The
  // main loop performs it once the lease is free.
  config.requestSaveConfiguration();
  deviceLog("contador agua: %d L por gravar", (int)waterLiters);
}

void Sensor::updateWaterFloor()
{
  // The quiet level is the smallest amplitude seen recently. Two buckets give a
  // sliding window of fifteen to thirty minutes in constant memory: a domestic
  // supply is idle for part of any quarter hour, and thirty minutes bounds how
  // long a wrong guess can persist.
  constexpr unsigned long kQuietPeriodMs = 900000ul;
  if (waterQuietPeriodStart == 0ul)
    waterQuietPeriodStart = millis();
  if (millis() - waterQuietPeriodStart >= kQuietPeriodMs)
  {
    waterQuietPrev = waterQuietNow;
    waterQuietNow = -1;
    waterQuietPeriodStart = millis();
  }
  // Only windows the detector treats as quiet feed the estimate, so a draw does
  // not teach the ceiling its own signal. waterFloor starts at the conservative
  // maximum, which makes everything quiet on the first pass and lets the true
  // ceiling be learnt from below within a period.
  if (waterAmplitude < waterFloor &&
      (waterQuietNow < 0 || waterAmplitude > waterQuietNow))
    waterQuietNow = waterAmplitude;

  long quiet = waterQuietNow;
  if (waterQuietPrev > quiet)
    quiet = waterQuietPrev;
  if (quiet < 0)
    return;  // Nothing measured yet: leave the conservative threshold in place.

  // Twice the quiet ceiling. Measured on an installed meter: the quiet amplitude
  // peaks around 266 counts and a cistern refilling reads 806-1098, so double the
  // ceiling sits at about 530 — clear of the loudest silence by two, and below the
  // quietest real flow by one and a half. A fixed constant cannot do that on
  // somebody else's meter, mount and coil distance, and choosing one by hand
  // already cost a litre in seven.
  long floor = quiet * 2;

  // The clamps bound what the adaptation can get wrong.
  //
  // The lower one stops an unusually still sensor from setting a threshold near
  // zero and counting its own breathing. The quiet level measured here was 130 to
  // 208, so twice it clears 200 anyway and this rarely binds.
  //
  // The upper one is chosen from the flowing amplitude, not picked round: 600 is
  // below the 806 counts measured while a cistern refilled, so the threshold can
  // never climb past a real signal. That matters because amplitude alone cannot
  // tell a steady leak from steady noise — both look like an unvarying level, and
  // over half an hour a leak would teach the sliding minimum its own amplitude.
  // The cap is what keeps the meter counting through it. A leak whose flow is so
  // steady and so weak that it sits under 600 counts remains undetectable this
  // way; separating that from noise needs periodicity, not a threshold, and
  // guessing a number here instead would be exactly the mistake that has already
  // cost a litre in seven.
  constexpr long kFloorMin = 300;
  constexpr long kFloorMax = 600;
  if (floor < kFloorMin)
    floor = kFloorMin;
  if (floor > kFloorMax)
    floor = kFloorMax;
  waterFloor = floor;
}

void Sensor::publishWaterState()
{
  waterLastPublish = millis();
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  state.clear();
  // Litres, because that is the unit the dial counts in and the unit Home Assistant
  // wants for a total_increasing water sensor.
  obj["liters"] = waterLiters;
  obj["flow"] = waterFlow;
  // The signal strength behind those numbers. Published so a count can be judged
  // rather than trusted: the noise floor was calibrated at one meter, and on a
  // bench with no target in front of the coil this sensor counted fourteen
  // phantom litres in seven minutes. Whoever installs the next one needs to see
  // the amplitude, not guess it.
  obj["amplitude"] = waterAmplitude;
  // The threshold the device chose for itself, so an installation can be judged
  // from its own numbers rather than from a constant somebody once measured.
  obj["floor"] = waterFloor;
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
    // Do not report this as a count mismatch: a driver missing from the
    // topology switch fails here with the counts matching, and a message
    // reading "expected 2, found 2" sends the reader hunting the wrong bug.
    if (!error)
      Log.error("%s Rejected input topology for %s: driver %s, %u pin(s), "
                "expected %u%s" CR,
                tags::sensors, uniqueId, driverToText().c_str(),
                static_cast<unsigned int>(inputs.size()),
                static_cast<unsigned int>(expectedInputCount(driver)),
                isSupportedOnCurrentTarget(driver)
                    ? ""
                    : " (driver not supported on this target)");
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
    // Fifty samples a second, and not one per loop iteration.
    //
    // Reading on every pass was written when the target was assumed to spin fast. It
    // does not — once per litre, so under 1 Hz even at a domestic meter's maximum —
    // and the cost of the assumption was severe: an I2C transaction every iteration
    // holds the feature lease almost continuously, so the panel answered "feature
    // configuration is busy" and the captive portal would not open at all.
    //
    // At 50 Hz there are still seventy samples per revolution, and the lease is free
    // the rest of the time.
    // 2^8 samples at fifty a second is 5.1 s.
    //
    // A ten-minute constant (shift 15) was tried and measured 5% against the
    // dial where this measures 65%: with a long constant the residual carries a
    // slow offset instead of sitting on zero, and the hysteresis below — which
    // rearms only when the residual falls past minus the trigger — then never
    // rearms. One turn per draw gets counted and the rest are lost. Lengthening
    // the constant needs the trigger referred to the residual's own local mean
    // first, which is a design change, not a constant.
    constexpr int kWaterSlowShift = 8;
    constexpr unsigned long kWaterReadInterval = 20ul;
    if (lastRead + kWaterReadInterval > millis())
      return;
    lastRead = millis();

    if (!isInitialized())
    {
      if (!ldcWaterBegin())
      {
        // Keep trying rather than giving up until the next reboot: a long cable to a
        // meter pit is exactly where an intermittent bus shows up. Complaining once a
        // minute, because the log is a small ring buffer and a dead sensor would
        // otherwise fill it and push out everything else.
        reInit();
        if (waterWindowStart == 0ul || millis() - waterWindowStart > 60000ul)
        {
          waterWindowStart = millis();
          deviceLog("contador agua: sem resposta i2c");
        }
        return;
      }
      deviceLog("contador agua: %s ligado", FeatureDrivers::LDC1612);
    }

    const long value = ldcWaterRead();
    // Zero is not a reading. Asked immediately after begin(), before the first
    // conversion has finished, the LDC1612 answers zero with no error bit set —
    // and seeding the baseline with it left the filter climbing from zero to
    // forty-four million, which at a ten-minute time constant blinded the
    // detector for the best part of an hour after every restart. A coil in front
    // of a target never reads anywhere near zero.
    if (value <= 0)
      return;

    if (!waterPrimed)
    {
      waterSlowScaled = static_cast<int64_t>(value) << kWaterSlowShift;
      waterSlowAverage = value;
      waterLongMinNow = waterLongMaxNow = 0;
      waterLongMinPrev = waterLongMaxPrev = 0;
      waterLongPeriodStart = millis();
      waterPrimed = true;
      waterPrimedAt = millis();
      waterWindowStart = millis();
      waterResidualMin = waterResidualMax = 0;
    }
    // Integer arithmetic throughout: no floats in a loop that runs fifty times a
    // second on a part without an FPU to spare.
    //
    // Ten minutes of time constant, chosen by simulation against three hours of
    // real samples taken while the house slept and the only water moving was a
    // leak: at five seconds the detector found one litre where ten had passed,
    // and at this setting it found eleven. The old five seconds came from an
    // assumption that the target span fast, which it does not.
    waterSlowScaled += value - (waterSlowScaled >> kWaterSlowShift);
    waterSlowAverage = static_cast<long>(waterSlowScaled >> kWaterSlowShift);
    const long residual = value - waterSlowAverage;

    if (residual < waterResidualMin)
      waterResidualMin = residual;
    if (residual > waterResidualMax)
      waterResidualMax = residual;

    // Eight seconds, measured and not chosen.
    //
    // Three seconds was justified by the meter's *maximum* flow and never held
    // against a real one. At 4.7 L/min a revolution takes 12.7 s, so a
    // three-second window measured 42% of the swing: the device published
    // amplitudes of 166 to 714 against its own floor of 600 while water was
    // visibly running, and counted 9 litres out of 35 — 26%.
    //
    // Full-rate bursts through one draw put the real cycle at 950 counts over
    // 12.7 s. Simulating the whole detector, hysteresis included, over five of
    // those bursts: three seconds finds 6 turns of 12, eight seconds finds 11,
    // fifteen finds 6 again — a longer window raises the amplitude, which raises
    // the trigger, which makes the rearm harder. Eight is the top of a plateau
    // that holds across every trigger fraction tried, not a fragile maximum.
    if (millis() - waterWindowStart > 8000ul)
    {
      waterAmplitude = waterResidualMax - waterResidualMin;
      waterResidualMin = waterResidualMax = residual;
      waterWindowStart = millis();
      if (waterWindowsSettled < 2)
        waterWindowsSettled++;
    }

    // The long window, twenty minutes to a bucket, so its span is twenty to
    // forty. Long enough to hold a whole revolution at three litres an hour,
    // which is the regime the three-second window cannot see at all.
    constexpr unsigned long kWaterLongPeriodMs = 1200000ul;
    if (waterLongPeriodStart == 0ul ||
        millis() - waterLongPeriodStart >= kWaterLongPeriodMs)
    {
      waterLongMinPrev = waterLongMinNow;
      waterLongMaxPrev = waterLongMaxNow;
      waterLongMinNow = waterLongMaxNow = residual;
      waterLongPeriodStart = millis();
    }
    if (residual < waterLongMinNow)
      waterLongMinNow = residual;
    if (residual > waterLongMaxNow)
      waterLongMaxNow = residual;
    const long longMin = waterLongMinPrev < waterLongMinNow ? waterLongMinPrev : waterLongMinNow;
    const long longMax = waterLongMaxPrev > waterLongMaxNow ? waterLongMaxPrev : waterLongMaxNow;
    const long longAmplitude = longMax - longMin;
    // Measured, not used. Taking the larger of the two windows inflated the
    // trigger, and with it the amount the residual had to fall by to rearm — the
    // same defect as the long time constant, from the other side. The long window
    // stays here because it is what a leak needs, and it costs nothing to keep
    // measuring while the trigger is made independent of it.
    (void)longAmplitude;

    // Three time constants of the high-pass filter. Until the slow average has
    // caught up with the raw reading there is no meaningful residual to threshold,
    // so measure the noise but never count a turn.
    constexpr unsigned long kWaterWarmupMs = 30000ul;
    if (millis() - waterPrimedAt < kWaterWarmupMs)
    {
      // Keep the window fresh so the first amplitude after warm-up describes the
      // settled signal rather than the settling.
      waterResidualMin = waterResidualMax = residual;
      waterAmplitude = 0;
      waterWindowsSettled = 0;
      return;
    }

    // Wait for one whole window before trusting an amplitude. Counting on the
    // first one after warm-up invented a litre on a real board: that window's
    // extremes were reset at the warm-up boundary, so it measured a fraction of
    // three seconds. Three seconds of caution, against a litre per restart.
    if (waterWindowsSettled < 2)
      return;

    // Below the threshold the window is the sensor breathing, not water. The
    // threshold is measured, not compiled in: see updateWaterFloor().
    updateWaterFloor();

    // An amplitude far above anything water can produce is a bus glitch or a
    // re-initialisation, not flow. Measured on an installed meter: the quiet
    // ceiling sits near 300 counts and a real draw reads 800 to 1400, so the
    // signal is about five times the silence. Fifty times is therefore an order
    // of magnitude above any genuine reading, and it catches what actually
    // happened: a window of 90,911 counts counted a litre that never flowed.
    // Expressed against the learnt ceiling rather than as a constant, because
    // the ratio holds across installations while the absolute value does not.
    if (waterQuietNow > 0 && waterAmplitude / 50 > waterQuietNow)
    {
      // Drop the window rather than the reading: the next one is measured from
      // scratch, so a single glitch cannot leave a stale amplitude behind.
      waterResidualMin = waterResidualMax = residual;
      waterAmplitude = 0;
      waterWindowStart = millis();
      return;
    }

    if (waterAmplitude > waterFloor)
    {
      // Trigger on the residual against its own amplitude. Comparing the raw value
      // against the slow average instead lost one turn in seven on a real meter,
      // because a shallow cycle riding on a moving average never reached a fixed
      // offset from it.
      const long trigger = waterAmplitude / 10;
      // A DN15 domestic meter turns once per litre and cannot pass more than its
      // overload flow, Q4 = 3.125 m3/h on this Aquadis+ — about 52 L/min, or one
      // turn every 1.15 s. Anything faster is not water, so refuse to count it.
      //
      // 1200 and not 1000: a one-second gate admits 60 L/min, which is above that
      // ceiling, and a bench board duly reported a turn at 58.88 L/min. The
      // number here has to come from the meter's rating, not from a round figure.
      constexpr unsigned long kWaterMinTurnIntervalMs = 1200ul;
      const bool tooSoon = waterLastTurn > 0ul &&
                           millis() - waterLastTurn < kWaterMinTurnIntervalMs;
      if (!waterAbove && residual > trigger && !tooSoon)
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
        //
        // Ten litres, not a hundred. A hundred was chosen to spare the flash and
        // it costs too much: whatever has not reached disk is lost on any restart,
        // and two litres went missing during a single installation simply because
        // the board was power-cycled twice. Ten litres of granularity is a couple
        // of dozen writes a day on a domestic supply, which the flash will outlive.
        if (waterUnsavedTurns >= 10)
          persistWaterTotal();
      }
      else if (waterAbove && residual < -trigger)
      {
        waterAbove = false;
      }
    }

#ifdef DEBUG_ONOFRE
    // A burst at the full read rate, once a minute. Filled one sample per pass,
    // so the lease is never held for longer than a single I2C transaction.
    if (waterBurst == nullptr)
      waterBurst = static_cast<long *>(malloc(kWaterBurstSamples * sizeof(long)));
    if (waterBurst != nullptr)
    {
      // Decimation first, on its own. Folding it into the condition below made
      // the else branch run on four passes out of five, and since its timer had
      // long expired it wiped the half-filled buffer every time — so the burst
      // never completed and nothing was ever published, not even a failure.
      if (++waterBurstDecimate >= 5)
      {
        waterBurstDecimate = 0;
        if (waterBurstCount < kWaterBurstSamples)
        {
        waterBurst[waterBurstCount++] = value;
        if (waterBurstCount == kWaterBurstSamples)
        {
          // Deltas between consecutive samples, not against the first: the step
          // between two readings twenty milliseconds apart is tens of counts,
          // where the offset from the first sample reaches hundreds. Smaller
          // numbers make a smaller payload, and payload size is why the bursts
          // that mattered never arrived — the ones published during a draw, when
          // the client was busiest, were dropped in silence.
          String payload = String("{\"t\":") + String(millis()) +
                           ",\"base\":" + String(waterBurst[0]) + ",\"s\":[";
          for (size_t i = 1; i < kWaterBurstSamples; i++)
          {
            if (i > 1)
              payload += ',';
            payload += String(waterBurst[i] - waterBurst[i - 1]);
          }
          payload += "]}";
          const bool sent = publishOnMqtt(
              String(String(readTopic) + "/burst").c_str(), payload.c_str(), false);
          if (!sent)
            deviceLog("contador agua: lote perdido (%u bytes)", payload.length());
          waterBurstArmedAt = millis();
          // Left full on purpose. Zeroing it here restarted the fill on the very
          // next pass, so the sixty-second gate below was dead code and this
          // published a kilobyte and a half every five seconds.
          waterBurstCount = kWaterBurstSamples;
        }
        }
        else if (millis() - waterBurstArmedAt >= 60000ul)
        {
          waterBurstCount = 0;
        }
      }
    }

    // Temporary instrumentation, debug builds only: raw samples every two
    // seconds on a topic of their own, so the drift of the baseline over minutes
    // can be measured before a slow channel is designed around it. The fast
    // detector cannot see flow slower than about 15 L/h — not because the signal
    // is weaker, it is the same size, but because the five-second high-pass
    // follows it and cancels it. Whether a minutes-long window is usable depends
    // entirely on how far the baseline wanders, which is a measurement, not a
    // guess. A separate topic keeps this out of the state payload, where a
    // Home Assistant value template would choke on it.
    //
    // One shared timer: this build carries a single meter, and the alternative is
    // a member that ships in release for no reason.
    static unsigned long rawPublishedAt = 0ul;
    if (millis() - rawPublishedAt >= 2000ul)
    {
      rawPublishedAt = millis();
      char payload[96];
      snprintf(payload, sizeof(payload),
               "{\"raw\":%ld,\"avg\":%ld,\"res\":%ld,\"amp\":%ld}",
               value, waterSlowAverage, residual, waterAmplitude);
      publishOnMqtt(String(String(readTopic) + "/raw").c_str(), payload, false);
    }
#endif

    // Announce the total even when nothing is running. The first publish happens
    // as soon as the sensor has a reading, so the panel never sits on a dash
    // waiting for somebody to open a tap.
    constexpr unsigned long kWaterPublishInterval = 60000ul;
    if (waterLastPublish == 0ul ||
        millis() - waterLastPublish >= kWaterPublishInterval)
      publishWaterState();

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
          lastBinaryState = true;
          obj["occupancy"] = Payloads::presenceOnPayload;
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
  }
}
