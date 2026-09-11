#pragma once
#include <Arduino.h>
#include <vector>
#include "Constants.h"
#include <ModbusMaster.h>
#include <Wire.h>
enum SensorDriver
{
  INVALID_SENSOR = 999,
  DHT_11 = 111,
  DHT_21 = 121,
  DHT_22 = 122,
  PZEM_004T_V03 = 71,
  PZEM_004T_V01 = 72,
  HAN = 80,
  LTR303X = 81,
  PIR = 82,
  RAIN = 83,
  DOOR = 84,
  WINDOW = 85,
  DS18B20 = 90,
  SHT4X = 91,
  TMF882X = 92,
  HCSR04 = 93,
  LD2410 = 94,
  LDC1612 = 95,
  LD2450 = 96,
  LD2460 = 97

};

namespace SensorRuntimePins
{
#ifdef ESP32
// These are the pins the legacy ESP32 implementations actually open. Keep
// ownership validation and the runtime constructors on the same constants.
constexpr unsigned int HAN_RX{21u};
constexpr unsigned int HAN_TX{7u};
constexpr unsigned int PZEM_V01_RX{27u};
constexpr unsigned int PZEM_V01_TX{26u};
#endif
} // namespace SensorRuntimePins

class Sensor
{
public:
  // Configuration topology is part of the driver contract. Keep it in one
  // place so live updates cannot validate a different shape from the runtime
  // reader. A zero count means the driver is unknown/invalid, not pinless.
  static size_t expectedInputCount(SensorDriver sensorDriver)
  {
    switch (sensorDriver)
    {
    case DHT_11:
    case DHT_21:
    case DHT_22:
    case PIR:
    case RAIN:
    case DOOR:
    case WINDOW:
    case DS18B20:
      return 1;
    case PZEM_004T_V03:
    case PZEM_004T_V01:
    case HAN:
    case LTR303X:
    case SHT4X:
    case TMF882X:
    case HCSR04:
    case LD2410:
    case LDC1612:
      return 2;
    case INVALID_SENSOR:
    default:
      return 0;
    }
  }

  // Some drivers compile on a target even though their runtime wiring cannot
  // exist there. Fail closed before touching those fixed pins. In particular,
  // PZEM v1's ESP32 implementation is hard-wired to GPIO27/26, which are not
  // usable application pins on the C3/C6 variants.
  static bool isSupportedOnCurrentTarget(SensorDriver sensorDriver)
  {
    switch (sensorDriver)
    {
    case DHT_11:
    case DHT_21:
    case DHT_22:
    case PZEM_004T_V03:
    case PZEM_004T_V01:
    case HAN:
    case LTR303X:
    case PIR:
    case RAIN:
    case DOOR:
    case WINDOW:
    case DS18B20:
    case SHT4X:
    case TMF882X:
    case HCSR04:
    case LD2410:
    case LDC1612:
      break;
    case INVALID_SENSOR:
    default:
      return false;
    }
#ifdef ESP8266
    if (sensorDriver == TMF882X || sensorDriver == LD2410)
      return false;
#endif
#if defined(ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32C3)
    if (sensorDriver == PZEM_004T_V01)
      return false;
#endif
    return true;
  }

  // Return the pins the driver actually uses when its stored input array is
  // descriptive only. Claims and conflict checks must follow runtime hardware,
  // not a legacy JSON array that the implementation ignores.
  static bool fixedRuntimeInputs(SensorDriver sensorDriver,
                                 std::vector<unsigned int> &runtimeInputs)
  {
    runtimeInputs.clear();
    switch (sensorDriver)
    {
    case LTR303X:
    case SHT4X:
    case TMF882X:
    case LDC1612:
      runtimeInputs = {static_cast<unsigned int>(DefaultPins::SDA),
                       static_cast<unsigned int>(DefaultPins::SCL)};
      return true;
#ifdef ESP32
    case HAN:
      // Sensor::loop() opens Serial1 as RX=21, TX=7 on every ESP32-family
      // build. Keep that canonical order here even though the older
      // DefaultPins HAN names describe a different pair.
      runtimeInputs = {SensorRuntimePins::HAN_RX,
                       SensorRuntimePins::HAN_TX};
      return true;
    case PZEM_004T_V01:
      runtimeInputs = {SensorRuntimePins::PZEM_V01_RX,
                       SensorRuntimePins::PZEM_V01_TX};
      return true;
#endif
    default:
      return false;
    }
  }

  // CONFIG
  char uniqueId[24]{};

  char name[24] = {0};
  int hwAddress{0x10};
  SensorDriver driver;
  String state = "";
  // MQTT
  char readTopic[128];

  // CLOUDIO
  char cloudIOreadTopic[128]{};

  // GPIOS INPUT
  std::vector<unsigned int> inputs;

  // CONTROL VARIABLES
  int lastBinaryState = -1;
  unsigned long delayRead = 5000ul;

  /**
   * Water-meter pickup state (LDC1612 only).
   *
   * The count is what the meter's own register says, in litres, and it is seeded by
   * hand from the dial — the coil counts revolutions of a target and has no idea how
   * much water preceded it. Kept as a double because a domestic meter reaches six
   * digits of litres and the value has to survive being written and read as JSON.
   */
  double waterLiters = 0.0;
  double litersPerTurn = 1.0;
  /** Litres per minute, from the interval between the last turns. */
  double waterFlow = 0.0;
  /** Slow average subtracted from every sample so the thermal drift, which is
   *  larger than the signal, does not swamp it. Measured on an Itron Aquadis+:
   *  84,000 counts of drift over two seconds against a 1,400-count signal. */
  long waterSlowAverage = 0;
  bool waterPrimed = false;
  long waterResidualMin = 0;
  long waterResidualMax = 0;
  long waterAmplitude = 0;
  unsigned long waterWindowStart = 0ul;
  unsigned long waterLastTurn = 0ul;
  bool waterAbove = false;
  /** Turns since the last time the total was written to flash. */
  unsigned int waterUnsavedTurns = 0;
  unsigned long lastRead = 0ul;
  bool initialized = false;
  // A pin-map change is applied by reboot. Once the old driver is quiesced,
  // keep it from touching either the old or new pins in the response window.
  bool ready = true;
  bool error = false;
  int errorCounter = 0;
  unsigned long lastErrorTimestamp = 0ul;
  int id = 0;
  void* context = nullptr;
  String

  familyToText()
  {
    switch (driver)
    {
    case DS18B20:
    case SHT4X:
    case DHT_11:
    case DHT_21:
    case DHT_22:
      return Family::CLIMATE;
    case RAIN:
      return Family::GARDEN;
    case PZEM_004T_V03:
    case PZEM_004T_V01:
    case HAN:
      return Family::ENERGY;
    case PIR:
    case DOOR:
    case WINDOW:
    case LD2410:
    case LD2450:
    case LD2460:
      return Family::SECURITY;
    case LTR303X:
    case HCSR04:
    case LDC1612:
      return Family::LEVEL_METER;
    case INVALID_SENSOR:
      return Family::NONE;
    default:
      return Family::NONE;
    }

    return Family::NONE;
  };
  String driverToText()
  {
    switch (driver)
    {
    case DS18B20:
      return FeatureDrivers::DS18B20;
    case LTR303X:
      return FeatureDrivers::LTR303;
    case SHT4X:
      return FeatureDrivers::SHT4X;
    case DHT_11:
      return FeatureDrivers::DHT_11;
    case DHT_21:
      return FeatureDrivers::DHT_21;
    case DHT_22:
      return FeatureDrivers::DHT_22;
    case PZEM_004T_V03:
      return FeatureDrivers::PZEM_004T_V03;
    case PZEM_004T_V01:
      return FeatureDrivers::PZEM_004T_V01;
    case HAN:
      return FeatureDrivers::HAN;
    case RAIN:
      return FeatureDrivers::RAIN;
    case DOOR:
      return FeatureDrivers::DOOR;
    case WINDOW:
      return FeatureDrivers::WINDOW;
    case PIR:
      return FeatureDrivers::PIR;
    case HCSR04:
      return FeatureDrivers::HCSR04;
    case LD2410:
      return FeatureDrivers::LD2410;
    case LD2450:
      return FeatureDrivers::LD2450;
    case LD2460:
      return FeatureDrivers::LD2460;
    case TMF882X:
      return FeatureDrivers::TMF882X;
    case LDC1612:
      return FeatureDrivers::LDC1612;
    case INVALID_SENSOR:
      return FeatureDrivers::INVALID;
    }
    return FeatureDrivers::INVALID;
  };
  /** True once the LDC1612 answers and has been configured. */
  bool ldcWaterBegin();
  /** The 28-bit conversion, or -1 when the channel reports an error. */
  long ldcWaterRead();
  /** Publishes litres and flow as this sensor's state. */
  void publishWaterState();
  /** Writes the running total to flash. Rare on purpose. */
  void persistWaterTotal();

  const bool isInitialized()
  {
    if (!initialized)
    {
      initialized = true;
      return false;
    }
    return initialized;
  };
  const void reInit()
  {
    initialized = false;
  };
  void deactivateForConfigUpdate()
  {
    ready = false;
  }

  // Zeroing a PZEM's accumulated energy talks to it over the same serial bus the
  // reader owns, so the request is queued here and carried out inside the
  // sensor's own loop rather than from the web handler's context.
  // Inverted flag semantics let GCC's atomic-flag primitives provide a
  // copyable cross-task handoff without making Sensor non-copyable.
  bool energyResetRequestConsumed = true;
  bool supportsEnergyReset() const
  {
    return driver == SensorDriver::PZEM_004T_V03;
  };
  void requestEnergyReset()
  {
    __atomic_clear(&energyResetRequestConsumed, __ATOMIC_RELEASE);
  };
  bool takeEnergyResetRequest()
  {
    return !__atomic_test_and_set(&energyResetRequestConsumed, __ATOMIC_ACQ_REL);
  };

  const void clearError()
  {
    error = false;
    lastErrorTimestamp = 0ul;
  };
  const void setError()
  {
    if (error)
      return;
    error = true;
    lastErrorTimestamp = millis();
    errorCounter++;
  };
  void loop();
  void notifyState();

private:
  bool hasRuntimeInputTopology() const
  {
    if (!isSupportedOnCurrentTarget(driver))
      return false;

    switch (driver)
    {
    // These implementations use a global/default I2C bus rather than
    // indexing the stored pin vector. Preserve existing field configurations
    // while still requiring two pins when a new array is explicitly supplied.
    case LTR303X:
    case SHT4X:
    case TMF882X:
      return true;
    // ESP32 HAN uses fixed UART pins. ESP8266 HAN indexes RX/TX from inputs.
    case HAN:
#ifdef ESP8266
      return inputs.size() == 2;
#else
      return true;
#endif
    // ESP32 PZEM v1 uses a fixed UART pair and does not index the stored array.
    case PZEM_004T_V01:
#ifdef ESP32
      return true;
#else
      return inputs.size() == expectedInputCount(driver);
#endif
    // LD2410 is implemented only on ESP32. On that target it indexes RX/TX.
    case LD2410:
#ifdef ESP32
      return inputs.size() == 2;
#else
      return true;
#endif
    case INVALID_SENSOR:
      return true;
    case DHT_11:
    case DHT_21:
    case DHT_22:
    case PZEM_004T_V03:
    case PIR:
    case RAIN:
    case DOOR:
    case WINDOW:
    case DS18B20:
    case HCSR04:
      return inputs.size() == expectedInputCount(driver);
    default:
      return false;
    }
  }
};
