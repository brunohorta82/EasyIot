#include "ConfigOnofre.h"
#include "CoreWiFi.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "WebServer.h"
#include "Templates.h"
#include "LittleFS.h"
#include "Wire.h"
#include "Images.hpp"
#include <PZEM004Tv30.h>
#include "HomeAssistantMqttDiscovery.h"
#include "CloudIO.h"
#include "DeviceClock.h"
#include "Irrigation.h"
#include "Persistence.h"
#include <algorithm>

static constexpr const char *kFirmwareBuildDate = __DATE__ " " __TIME__;
static constexpr uint32_t kFeatureAccessYieldMs = 100;

static const char *currentMcuName()
{
#ifdef ESP32
#ifdef ESP32C6
  return "ESP32-C6";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  return "ESP32-C3";
#else
  return "ESP32";
#endif
#else
#ifdef HAN_MODE
  return "ESP8266-HAN";
#else
  return "ESP8266";
#endif
#endif
}

static bool featureAccessYieldActive(uint32_t deadline)
{
  return deadline != 0 &&
         static_cast<int32_t>(deadline - millis()) > 0;
}

void actuatoresCallback(message_t const &msg, void *arg);

// Kept out of any debug guard: the web panel shows this in release builds, where it
// is the first thing worth knowing when a device has been misbehaving.
String deviceResetReason()
{
#ifdef ESP8266
  return ESP.getResetReason();
#else
  switch (esp_reset_reason())
  {
  case ESP_RST_POWERON:   return "Power-on";
  case ESP_RST_EXT:       return "External reset";
  case ESP_RST_SW:        return "Software reset";
  case ESP_RST_PANIC:     return "Exception/Panic";
  case ESP_RST_INT_WDT:   return "Interrupt watchdog";
  case ESP_RST_TASK_WDT:  return "Task watchdog";
  case ESP_RST_WDT:       return "Other watchdog";
  case ESP_RST_DEEPSLEEP: return "Deep sleep wake";
  case ESP_RST_BROWNOUT:  return "Brownout";
  case ESP_RST_SDIO:      return "SDIO reset";
  default:                return "Unknown";
  }
#endif
}

void ConfigOnofre::generateId(String &id, const String &name, int familyCode, int io, size_t maxSize)
{
  id.reserve(maxSize);
  id.concat(chipId);
  id.concat(name);
  id.concat(familyCode);
  id.concat(io);
  id.toLowerCase();
  normalize(id);
}
ConfigOnofre &ConfigOnofre::init()
{
#ifdef ESP8266
  strlcpy(chipId, String(ESP.getChipId()).c_str(), sizeof(chipId));
#endif
#ifdef ESP32
  uint32_t chipIdHex = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipIdHex |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  strlcpy(chipId, String(chipIdHex).c_str(), sizeof(chipId));
#endif
  strlcpy(nodeId, chipId, sizeof(nodeId));
  mqttPort = constantsMqtt::defaultPort;
  dhcp = true;
  strlcpy(accessPointPassword, constantsConfig::apSecret, sizeof(accessPointPassword));
  strlcpy(apiUser, constantsConfig::apiUser, sizeof(apiUser));
  strlcpy(apiPassword, constantsConfig::apiPassword, sizeof(apiPassword));
#ifdef WIFI_SSID
  strlcpy(wifiSSID, WIFI_SSID, sizeof(wifiSSID));
#endif
#ifdef WIFI_SECRET
  strlcpy(wifiSecret, WIFI_SECRET, sizeof(wifiSecret));
#endif
#ifdef DEBUG_ONOFRE
  Log.notice("%s Default config loaded." CR, tags::config);
#endif
#ifdef TEST_TEMPLATE
  templateSelect(Template::DUAL_LIGHT);
#endif
#ifdef HAN_MODE
  templateSelect(Template::HAN_MODULE);
#endif
#ifdef IRRIGATION_MODE
  // A dedicated irrigation board should come up as one, the same way HAN_MODE
  // works: five zones and the rain sensor, without anyone picking a template.
  templateSelect(Template::GARDEN);
#endif
  return save();
}
bool ConfigOnofre::isSensorExists(int hwAddress)
{
  for (auto f : sensors)
  {
    if (f.hwAddress == hwAddress)
      return true;
  }
  return false;
}
#ifdef ESP32
void ConfigOnofre::pzemDiscovery()
{

  bool found = false;
  bool needsSave = false;
  for (int i = 0; i < 3; i++)
  {
    PZEM004Tv30 pzem(Serial1, DefaultPins::PZEM_TX, DefaultPins::PZEM_RX, Discovery::MODBUS_PZEM_ADDRESS_START + i);
    delay(200);
    float voltageOne = pzem.voltage();
    if (!isnan(voltageOne))
    {
#ifdef DEBUG_ONOFRE
      Log.info("%sPzem found with address: 0x%x " CR, tags::discovery, pzem.getAddress());
#endif
      found = voltageOne > 0;
      if (found && !isSensorExists(pzem.getAddress()))
      {
        preparePzem(String(I18N::ENERGY) + String(pzem.getAddress()), DefaultPins::PZEM_TX, DefaultPins::PZEM_RX, pzem.getAddress(), SensorDriver::PZEM_004T_V03);
        needsSave = true;
      }
    }
  }

  if (!found)
  {
    PZEM004Tv30 pzem(Serial1, DefaultPins::PZEM_TX, DefaultPins::PZEM_RX);

    delay(200);
    float voltageOne = pzem.voltage();
    if (!isnan(voltageOne))
    {
      found = voltageOne > 0;
      if (found && !isSensorExists(pzem.getAddress()))
      {
        preparePzem(String(I18N::ENERGY) + String(pzem.getAddress()), DefaultPins::PZEM_TX, DefaultPins::PZEM_RX, pzem.getAddress(), SensorDriver::PZEM_004T_V03);
        needsSave = true;
      }
    }
  }

  if (needsSave)
  {
    save();
  }
}
#endif
void ConfigOnofre::i2cDiscovery()
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Smart Bus Started." CR, tags::config);
#endif
  Wire.begin(DefaultPins::SDA, DefaultPins::SCL);

  bool needsSave = false;
  byte error, address;
  for (address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
#ifdef DEBUG_ONOFRE
      Log.notice("%s Device Found. 0x%x" CR, tags::config, address);
#endif
      if (address == Discovery::I2C_SHT4X_ADDRESS)
      {
        if (!isSensorExists(address))
        {
          prepareSHT4X(address);
          needsSave = true;
        }
      }
      else if (address == Discovery::I2C_SSD1306_ADDRESS)
      {
        // Discovery can be requested again by more than one PZEM sensor. Keep
        // the one live display instance rather than leaking a new allocation
        // on every scan.
        if (display == nullptr)
        {
          Adafruit_SSD1306 *candidate = new Adafruit_SSD1306(128, 64, &Wire, -1);
          if (candidate != nullptr &&
              candidate->begin(SSD1306_SWITCHCAPVCC,
                               Discovery::I2C_SSD1306_ADDRESS))
          {
            display = candidate;
            display->clearDisplay();
            display->drawBitmap(
                (display->width() - LOGO_WIDTH) / 2,
                0,
                logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
            display->setTextSize(2);
            display->setTextColor(SSD1306_WHITE);
            display->setCursor(30, LOGO_HEIGHT);
            display->println(F("ONOFRE"));
            display->display();
          }
          else
          {
            delete candidate;
          }
        }
      }
      else if (address == Discovery::I2C_LTR303_ADDRESS)
      {
        if (!isSensorExists(address))
        {
          prepareLTR303(address);
          needsSave = true;
        }
      }
      else if (address == Discovery::I2C_TMF880X_ADDRESS)
      {
        if (!isSensorExists(address))
        {
          prepareTMF882X(address);
          needsSave = true;
        }
      }
    }
  }
  if (needsSave)
    save();
#ifdef DEBUG_ONOFRE
  Log.notice("%s Smart Bus Done." CR, tags::config);
#endif
}
void ConfigOnofre::requestI2cDiscovery()
{
#ifdef ESP32
  i2cDiscoveryRequested.store(true, std::memory_order_release);
#else
  i2cDiscoveryRequested = true;
#endif
}
void ConfigOnofre::serviceDeferredI2cDiscovery()
{
#ifdef ESP32
  if (!i2cDiscoveryRequested.exchange(false, std::memory_order_acq_rel))
    return;
#else
  if (!i2cDiscoveryRequested)
    return;
  i2cDiscoveryRequested = false;
#endif

  // This method is called only after loopSensors() has released its lease.
  // Reacquire it for the scan because discovery may append sensors and save the
  // complete vectors. If another owner won the lease, coalesce another retry.
  if (!tryBeginFeatureAccess())
  {
    requestI2cDiscovery();
    return;
  }
  i2cDiscovery();
  endFeatureAccess();
}
bool ConfigOnofre::tryBeginFeatureAccess()
{
#ifdef ESP32
  bool expected = false;
  if (!featureAccessInProgress.compare_exchange_strong(
          expected, true, std::memory_order_acquire, std::memory_order_relaxed))
  {
    uint32_t deadline = millis() + kFeatureAccessYieldMs;
    if (deadline == 0)
      deadline = 1;
    featureAccessYieldUntilMs.store(deadline, std::memory_order_release);
    return false;
  }
  featureAccessYieldUntilMs.store(0, std::memory_order_release);
  return true;
#else
  // Do not wait or yield here: ESP8266 networking and feature work share the
  // same cooperative execution context.
  if (featureAccessInProgress)
  {
    featureAccessYieldUntilMs = millis() + kFeatureAccessYieldMs;
    if (featureAccessYieldUntilMs == 0)
      featureAccessYieldUntilMs = 1;
    return false;
  }
  featureAccessInProgress = true;
  featureAccessYieldUntilMs = 0;
  return true;
#endif
}
bool ConfigOnofre::tryBeginFeatureLoopAccess()
{
#ifdef ESP32
  if (featureAccessYieldActive(
          featureAccessYieldUntilMs.load(std::memory_order_acquire)))
    return false;
  bool expected = false;
  if (!featureAccessInProgress.compare_exchange_strong(
          expected, true, std::memory_order_acquire, std::memory_order_relaxed))
    return false;
  // Close the race where a foreground waiter arrived between the first check
  // and this loop acquiring the lease.
  if (featureAccessYieldActive(
          featureAccessYieldUntilMs.load(std::memory_order_acquire)))
  {
    featureAccessInProgress.store(false, std::memory_order_release);
    return false;
  }
  return true;
#else
  if (featureAccessInProgress ||
      featureAccessYieldActive(featureAccessYieldUntilMs))
    return false;
  featureAccessInProgress = true;
  if (featureAccessYieldActive(featureAccessYieldUntilMs))
  {
    featureAccessInProgress = false;
    return false;
  }
  return true;
#endif
}
void ConfigOnofre::endFeatureAccess()
{
#ifdef ESP32
  featureAccessInProgress.store(false, std::memory_order_release);
#else
  featureAccessInProgress = false;
#endif
}
bool ConfigOnofre::tryBeginConfigUpdate()
{
  return tryBeginFeatureAccess();
}
void ConfigOnofre::endConfigUpdate()
{
  endFeatureAccess();
}
bool ConfigOnofre::requestTemplateChange(int templateId)
{
#ifdef ESP32
  int expected = Template::NO_TEMPLATE;
  return requestedTemplateId.compare_exchange_strong(
      expected, templateId, std::memory_order_release, std::memory_order_relaxed);
#else
  if (requestedTemplateId != Template::NO_TEMPLATE)
    return false;
  requestedTemplateId = templateId;
  return true;
#endif
}
int ConfigOnofre::peekTemplateChangeRequest() const
{
#ifdef ESP32
  return requestedTemplateId.load(std::memory_order_acquire);
#else
  return requestedTemplateId;
#endif
}
void ConfigOnofre::clearTemplateChangeRequest(int templateId)
{
#ifdef ESP32
  int expected = templateId;
  requestedTemplateId.compare_exchange_strong(
      expected, Template::NO_TEMPLATE,
      std::memory_order_acq_rel, std::memory_order_acquire);
#else
  if (requestedTemplateId == templateId)
    requestedTemplateId = Template::NO_TEMPLATE;
#endif
}
ConfigOnofre &ConfigOnofre::load()
{
#ifdef ESP32
  pinMode(DefaultPins::OUTPUT_TWO, OUTPUT);
#endif
  knxIdRegister = knx.callback_register("Actuatores", actuatoresCallback);
  if (!LittleFS.exists(configFilenames::config))
  {
    return init();
  }

  File file = LittleFS.open(configFilenames::config, "r+");
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error)
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Failed to read file, using default configuration." CR, tags::config);
    serializeJson(doc, Serial);
#endif
    doc.clear();
    return init();
  }

#ifdef ESP8266
  strlcpy(chipId, String(ESP.getChipId()).c_str(), sizeof(chipId));
#endif
#ifdef ESP32
  uint32_t chipIdHex = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipIdHex |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  strlcpy(chipId, String(chipIdHex).c_str(), sizeof(chipId));
  sprintf(provisionId, "ONOFRE%s", chipId);

#endif
  templateId = doc["templateId"] | 0;
  strlcpy(nodeId,
          doc["nodeId"] | chipId,
          sizeof(nodeId));
  // CLOUDIO
  strlcpy(cloudIOUsername, doc["cloudIOUsername"] | "", sizeof(cloudIOUsername));
  strlcpy(cloudIOPassword, doc["cloudIOPassword"] | "", sizeof(cloudIOPassword));
  sprintf(cloudIOhealthTopic, "%s/%s/available", cloudIOUsername, chipId);

  // MQTT
  strlcpy(mqttIpDns, doc["mqttIpDns"] | "", sizeof(mqttIpDns));
  mqttPort = doc["mqttPort"] | 1883;
  strlcpy(mqttUsername, doc["mqttUsername"] | "", sizeof(mqttUsername));
  strlcpy(mqttPassword, doc["mqttPassword"] | "", sizeof(mqttPassword));
  sprintf(healthTopic, "onofre/%s/available", chipId);
  sprintf(irrigationStateTopic, "onofre/%s/irrigation/state", chipId);
  sprintf(irrigationWriteTopic, "onofre/%s/irrigation/set", chipId);

  // WIFI
  strlcpy(wifiSSID, doc["wifiSSID"] | "", sizeof(wifiSSID));
  strlcpy(wifiSecret, doc["wifiSecret"] | "", sizeof(wifiSecret));
  dhcp = doc["dhcp"] | true;
  strlcpy(wifiIp, doc["wifiIp"] | "", sizeof(wifiIp));
  strlcpy(wifiMask, doc["wifiMask"] | "", sizeof(wifiMask));
  strlcpy(wifiGw, doc["wifiGw"] | "", sizeof(wifiGw));
  // ACCESS POINT AND PANNEL ADMIN
  strlcpy(accessPointPassword, doc["accessPointPassword"] | constantsConfig::apSecret, sizeof(accessPointPassword));
  strlcpy(apiUser, doc["apiUser"] | constantsConfig::apiUser, sizeof(apiUser));
  strlcpy(apiPassword, doc["apiPassword"] | constantsConfig::apiPassword, sizeof(apiPassword));
  JsonArray features = doc["features"];

  for (auto d : features)
  {
    if (strcmp(d["group"] | "", "ACTUATOR") == 0)
    {
      Actuator actuator;
      strlcpy(actuator.uniqueId, d["id"] | "", sizeof(actuator.uniqueId));
      strlcpy(actuator.name, d["name"] | "", sizeof(actuator.name));
      actuator.driver = d["driver"];
      actuator.typeControl = d["typeControl"];
      actuator.knxAddress[0] = d["area"] | 0;
      actuator.knxAddress[1] = d["line"] | 0;
      actuator.knxAddress[2] = d["member"] | 0;
      actuator.upCourseTime = d["upCourseTime"] | constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;
      actuator.downCourseTime = d["downCourseTime"] | constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;
      actuator.state = d["state"] | 0;
      actuator.autoOff = d["autoOff"] | 0ul;
      String family = actuator.familyToText();
      family.toLowerCase();
      sprintf(actuator.readTopic, "onofre/%s/%s/%s/state", chipId, family.c_str(), actuator.uniqueId);
      sprintf(actuator.writeTopic, "onofre/%s/%s/%s/set", chipId, family.c_str(), actuator.uniqueId);
      JsonArray outputs = d["outputs"];
      for (auto out : outputs)
      {
        actuator.outputs.push_back(out);
      }
      JsonArray inputs = d["inputs"];
      for (auto in : inputs)
      {
        actuator.inputs.push_back(in);
      }

      // Stored configurations predate the live-update validator. Fail closed
      // before setup() can drive a pin that is reserved on the current target
      // (for example the ESP32-C6 Smart Bus on GPIO6/7).
      bool storedPinsValid = true;
      for (auto output : actuator.outputs)
        storedPinsValid = storedPinsValid && validOutputPin(output);
      for (auto input : actuator.inputs)
        storedPinsValid = storedPinsValid && validInputPin(input);
      if (!storedPinsValid)
      {
        // Guarded like every other log here: a release build has no ArduinoLog,
        // so an unguarded call breaks the four release targets while the debug
        // ones keep compiling.
#ifdef DEBUG_ONOFRE
        Log.error("%s Stored actuator has an invalid pin mapping; leaving it inactive." CR,
                  tags::config);
#endif
        continue;
      }

      actuator.setup();
      actuatores.push_back(actuator);
    }
    else if (strcmp(d["group"] | "", "SENSOR") == 0)
    {
      Sensor sensor;
      strlcpy(sensor.uniqueId, d["id"] | "", sizeof(sensor.uniqueId));
      strlcpy(sensor.name, d["name"] | "", sizeof(sensor.name));
      sensor.delayRead = d["delayRead"];
      sensor.driver = d["driver"];
      sensor.hwAddress = d["hwAddress"] | 0x10;
      sensor.id = featureIds++;
      String family = sensor.familyToText();
      family.toLowerCase();
      sprintf(sensor.readTopic, "onofre/%s/%s/%s/metrics", chipId, family.c_str(), sensor.uniqueId);
      sensor.state = "";
      JsonArray inputs = d["inputs"];
      for (auto in : inputs)
      {
        sensor.inputs.push_back(in);
      }
      sensors.push_back(sensor);
    }
  }
  doc.clear();
  // After the features, never before: a program naming a valve this device does
  // not have is dropped, which can only be decided once the valves are known.
  irrigation.load();
#ifdef DEBUG_ONOFRE
  Log.notice("%s Stored config loaded." CR, tags::config);

#endif
  return *this;
}
bool ConfigOnofre::loadTemplate(int templateId)
{
  if (!templateSelect((Template)templateId))
    return false;
  this->templateId = templateId;
  return true;
}
bool ConfigOnofre::persist()
{
  JsonDocument doc;
  doc["templateId"] = templateId;
  if (!String(nodeId).isEmpty())
    doc["nodeId"] = nodeId;
  // MQTT
  if (!String(mqttIpDns).isEmpty())
    doc["mqttIpDns"] = mqttIpDns;
  doc["mqttPort"] = mqttPort;
  if (!String(mqttUsername).isEmpty())
    doc["mqttUsername"] = mqttUsername;
  if (!String(mqttPassword).isEmpty())
    doc["mqttPassword"] = mqttPassword;
  // CLOUDIO
  if (!String(cloudIOUsername).isEmpty())
    doc["cloudIOUsername"] = cloudIOUsername;
  if (!String(cloudIOPassword).isEmpty())
    doc["cloudIOPassword"] = cloudIOPassword;
  // WIFI
  if (!String(wifiSSID).isEmpty())
    doc["wifiSSID"] = wifiSSID;
  if (!String(wifiSecret).isEmpty())
    doc["wifiSecret"] = wifiSecret;
  doc["dhcp"] = dhcp;
  if (!String(wifiIp).isEmpty())
    doc["wifiIp"] = wifiIp;
  if (!String(wifiMask).isEmpty())
    doc["wifiMask"] = wifiMask;
  if (!String(wifiGw).isEmpty())
    doc["wifiGw"] = wifiGw;
  // ACCESS POINT AND PANNEL ADMIN
  doc["accessPointPassword"] = accessPointPassword;
  doc["apiUser"] = apiUser;
  doc["apiPassword"] = apiPassword;

  JsonArray features = doc["features"].to<JsonArray>();
  for (auto s : actuatores)
  {
    // Build directly inside the parent document. If any allocation below
    // fails, the parent's overflow flag makes the atomic writer reject the
    // complete save instead of hiding a partial child document.
    JsonObject a = features.add<JsonObject>();
    a["group"] = "ACTUATOR";
    a["driver"] = s.driver;
    a["id"] = s.uniqueId;
    a["name"] = s.name;
    a["typeControl"] = s.typeControl;
    a["upCourseTime"] = s.upCourseTime;
    a["downCourseTime"] = s.downCourseTime;
    a["area"] = s.knxAddress[0];
    a["line"] = s.knxAddress[1];
    a["member"] = s.knxAddress[2];
    a["autoOff"] = s.autoOff;
    // A garden valve's live state is never a boot command. Persisting ON here
    // can reopen water after an unrelated save and power cycle.
    a["state"] = s.isGardenValve() ? ActuatorState::OFF_OPEN : s.state;
    JsonArray outputs = a["outputs"].to<JsonArray>();
    for (auto out : s.outputs)
    {
      outputs.add(out);
    }
    JsonArray inputs = a["inputs"].to<JsonArray>();
    for (auto in : s.inputs)
    {
      inputs.add(in);
    }
  }
  for (auto ss : sensors)
  {
    JsonObject a = features.add<JsonObject>();
    a["group"] = "SENSOR";
    a["id"] = ss.uniqueId;
    a["name"] = ss.name;
    a["driver"] = ss.driver;
    a["hwAddress"] = ss.hwAddress;
    a["delayRead"] = ss.delayRead;
    JsonArray inputs = a["inputs"].to<JsonArray>();
    for (auto in : ss.inputs)
    {
      inputs.add(in);
    }
  }

  if (!persistJsonAtomically(configFilenames::config,
                             configFilenames::configTemporary, doc))
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s Failed to store configuration atomically." CR, tags::config);
#endif
    doc.clear();
    return false;
  }
#ifdef DEBUG_ONOFRE
  Log.notice("%s Config Onofre stored." CR, tags::config);
#endif
  doc.clear();
  return true;
}

void ConfigOnofre::backup(JsonVariant &root)
{
  root["format"] = "easyiot-backup";
  root["version"] = 1;
  JsonObject target = root["target"].to<JsonObject>();
  target["mcu"] = currentMcuName();
  target["firmware"] = String(VERSION);

  JsonObject stored = root["configuration"].to<JsonObject>();
  stored["templateId"] = templateId;
  stored["nodeId"] = nodeId;
  stored["mqttIpDns"] = mqttIpDns;
  stored["mqttPort"] = mqttPort;
  stored["mqttUsername"] = mqttUsername;
  stored["cloudIOUsername"] = cloudIOUsername;
  stored["wifiSSID"] = wifiSSID;
  stored["dhcp"] = dhcp;
  stored["wifiIp"] = wifiIp;
  stored["wifiMask"] = wifiMask;
  stored["wifiGw"] = wifiGw;
  stored["apiUser"] = apiUser;

  JsonArray features = stored["features"].to<JsonArray>();
  for (const auto &actuator : actuatores)
  {
    JsonObject item = features.add<JsonObject>();
    item["group"] = "ACTUATOR";
    item["id"] = actuator.uniqueId;
    item["name"] = actuator.name;
    item["driver"] = actuator.driver;
    item["typeControl"] = actuator.typeControl;
    item["upCourseTime"] = actuator.upCourseTime;
    item["downCourseTime"] = actuator.downCourseTime;
    item["area"] = actuator.knxAddress[0];
    item["line"] = actuator.knxAddress[1];
    item["member"] = actuator.knxAddress[2];
    item["autoOff"] = actuator.autoOff;
    item["state"] = actuator.driver == ActuatorDriver::GARDEN_VALVE
                        ? ActuatorState::OFF_OPEN
                        : actuator.state;
    JsonArray outputs = item["outputs"].to<JsonArray>();
    for (auto pin : actuator.outputs)
      outputs.add(pin);
    JsonArray inputs = item["inputs"].to<JsonArray>();
    for (auto pin : actuator.inputs)
      inputs.add(pin);
  }
  for (const auto &sensor : sensors)
  {
    JsonObject item = features.add<JsonObject>();
    item["group"] = "SENSOR";
    item["id"] = sensor.uniqueId;
    item["name"] = sensor.name;
    item["driver"] = sensor.driver;
    item["hwAddress"] = sensor.hwAddress;
    item["delayRead"] = sensor.delayRead;
    JsonArray inputs = item["inputs"].to<JsonArray>();
    for (auto pin : sensor.inputs)
      inputs.add(pin);
  }

  JsonVariant irrigationRoot = root["irrigation"].to<JsonObject>();
  irrigation.jsonBody(irrigationRoot);
  irrigationRoot.remove("running");
}

ConfigOnofre &ConfigOnofre::save()
{
  persist();
  return *this;
}

ConfigOnofre &ConfigOnofre::reloadFeatures()
{
  for (auto &actuator : actuatores)
  {
    String family = actuator.familyToText();
    family.toLowerCase();
    sprintf(actuator.readTopic, "onofre/%s/%s/%s/state", chipId, family.c_str(), actuator.uniqueId);
    sprintf(actuator.writeTopic, "onofre/%s/%s/%s/set", chipId, family.c_str(), actuator.uniqueId);
  }
  for (auto &sensor : sensors)
  {
    String family = sensor.familyToText();
    family.toLowerCase();
    sprintf(sensor.readTopic, "onofre/%s/%s/%s/metrics", chipId, family.c_str(), sensor.uniqueId);
    sensor.state = "";
  }
  initHomeAssistantDiscovery();
  return *this;
}
void ConfigOnofre::controlFeature(StateOrigin origin, JsonObject &action, JsonVariant &result)
{
  controlFeature(origin, action["id"] | "0", action["state"] | 0);
}
void ConfigOnofre::controlFeature(StateOrigin origin, String topic, String payload)
{
  for (auto &a : actuatores)
  {
    if (strcmp(a.writeTopic, topic.c_str()) == 0 || strcmp(a.cloudIOwriteTopic, topic.c_str()) == 0)
    {
      controlFeature(origin, a.uniqueId, payload.toInt());
      return;
    }
  }
}
void ConfigOnofre::controlFeature(StateOrigin origin, String uniqueId, int state)
{
  for (auto &a : actuatores)
  {
    if (a.ready && uniqueId.equals(a.uniqueId))
    {
      if (state == ActuatorState::TOGGLE)
      {
        state = a.state == ActuatorState::ON_CLOSE ? ActuatorState::OFF_OPEN : ActuatorState::ON_CLOSE;
      }
      a.changeState(origin, state);
      this->save();
      return;
    }
  }
}

namespace
{
constexpr size_t maxRestoreFeatures{32};
constexpr size_t maxRestorePrograms{8};
constexpr uint16_t maxRestoreZoneMinutes{240};

enum class FeatureKind : uint8_t
{
  ACTUATOR,
  SENSOR
};

enum class PinRole : uint8_t
{
  INPUT_PIN,
  OUTPUT_PIN
};

enum class PinArrayRead : uint8_t
{
  ABSENT,
  VALID,
  INVALID
};

struct FeaturePinPlan
{
  String id;
  FeatureKind kind = FeatureKind::SENSOR;
  ActuatorDriver driver = ActuatorDriver::INVALID;
  SensorDriver sensorDriver = SensorDriver::INVALID_SENSOR;
  std::vector<unsigned int> oldInputs;
  std::vector<unsigned int> oldOutputs;
  std::vector<unsigned int> inputs;
  std::vector<unsigned int> outputs;
  bool inputsProvided = false;
  bool outputsProvided = false;
  bool removed = false;
  bool seen = false;
  bool restartRequired = false;
};

struct PinClaim
{
  String id;
  FeatureKind kind = FeatureKind::SENSOR;
  SensorDriver sensorDriver = SensorDriver::INVALID_SENSOR;
  PinRole role = PinRole::INPUT_PIN;
  size_t slot = 0;
  unsigned int pin = 0;
};

FeaturePinPlan *findPlan(std::vector<FeaturePinPlan> &plans, const String &id,
                        FeatureKind kind)
{
  for (auto &plan : plans)
    if (plan.kind == kind && plan.id.equals(id))
      return &plan;
  return nullptr;
}

bool planIdExists(const std::vector<FeaturePinPlan> &plans, const String &id)
{
  for (const auto &plan : plans)
    if (plan.id.equals(id))
      return true;
  return false;
}

bool supportedActuatorDriver(ActuatorDriver driver)
{
  switch (driver)
  {
  case ActuatorDriver::SWITCH_PUSH:
  case ActuatorDriver::SWITCH_LATCH:
  case ActuatorDriver::COVER_SINGLE_PUSH:
  case ActuatorDriver::COVER_DUAL_PUSH:
  case ActuatorDriver::COVER_DUAL_LATCH:
  case ActuatorDriver::LIGHT_PUSH:
  case ActuatorDriver::LIGHT_LATCH:
  case ActuatorDriver::GARAGE_PUSH:
  case ActuatorDriver::GARDEN_VALVE:
    return true;
  case ActuatorDriver::INVALID:
  default:
    return false;
  }
}

bool boundedString(JsonObjectConst object, const char *key, size_t capacity,
                   const char *&value)
{
  JsonVariantConst field = object[key];
  if (!field.is<const char *>())
    return false;
  value = field.as<const char *>();
  return value != nullptr && strlen(value) < capacity;
}

bool isI2cRestoreDriver(SensorDriver driver)
{
  return driver == SensorDriver::LTR303X || driver == SensorDriver::SHT4X ||
         driver == SensorDriver::TMF882X;
}

bool restoreClaimsMayShare(const PinClaim &left, const PinClaim &right)
{
  return left.kind == FeatureKind::SENSOR && right.kind == FeatureKind::SENSOR &&
         isI2cRestoreDriver(left.sensorDriver) &&
         isI2cRestoreDriver(right.sensorDriver) && left.role == right.role &&
         left.slot == right.slot;
}

bool isInputModeSupported(Actuator &actuator, unsigned int mode)
{
  if (actuator.isLight() || actuator.isSwitch())
    return mode == static_cast<unsigned int>(ActuatorInputMode::PUSH) ||
           mode == static_cast<unsigned int>(ActuatorInputMode::LATCH);
  if (actuator.isCover())
    return mode <= static_cast<unsigned int>(ActuatorInputMode::ROTATE);
  if (actuator.isGarage() || actuator.isGardenValve())
    return mode == static_cast<unsigned int>(ActuatorInputMode::PUSH);
  return false;
}

ConfigUpdateResult validateActuatorTopology(const Actuator &actuator,
                                            const FeaturePinPlan &plan)
{
  if (plan.driver == ActuatorDriver::COVER_SINGLE_PUSH &&
      plan.inputs.size() != 1)
    return ConfigUpdateResult::PIN_COUNT_MISMATCH;
  if ((plan.driver == ActuatorDriver::COVER_DUAL_PUSH ||
       plan.driver == ActuatorDriver::COVER_DUAL_LATCH) &&
      plan.inputs.size() != 2)
    return ConfigUpdateResult::PIN_COUNT_MISMATCH;

  if (actuator.typeControl == ActuatorControlType::GPIO_OUTPUT)
  {
    if ((plan.driver == ActuatorDriver::COVER_SINGLE_PUSH ||
         plan.driver == ActuatorDriver::COVER_DUAL_PUSH ||
         plan.driver == ActuatorDriver::COVER_DUAL_LATCH) &&
        plan.outputs.size() != 2)
      return ConfigUpdateResult::PIN_COUNT_MISMATCH;
    if (plan.driver == ActuatorDriver::GARAGE_PUSH && plan.inputs.empty())
      return ConfigUpdateResult::PIN_COUNT_MISMATCH;
  }
  return ConfigUpdateResult::OK;
}

PinArrayRead readPinArrayStrict(JsonVariantConst feature, const char *key,
                                std::vector<unsigned int> &out)
{
  JsonVariantConst value = feature[key];
  if (value.isUnbound())
    return PinArrayRead::ABSENT;
  if (!value.is<JsonArrayConst>())
    return PinArrayRead::INVALID;

  out.clear();
  for (JsonVariantConst pin : value.as<JsonArrayConst>())
  {
    // Do not let strings, null, negative, fractional, or overflowing values
    // silently coerce to GPIO0.
    if (!pin.is<unsigned int>())
      return PinArrayRead::INVALID;
    out.push_back(pin.as<unsigned int>());
  }
  return PinArrayRead::VALID;
}

bool validateConfigScalarTypes(JsonObjectConst root)
{
  static constexpr const char *stringFields[] = {
      "nodeId", "mqttIpDns", "mqttUsername", "mqttPassword",
      "wifiSSID", "wifiSecret", "wifiIp", "wifiMask", "wifiGw",
      "accessPointPassword", "apiUser", "apiPassword"};
  for (const char *field : stringFields)
  {
    JsonVariantConst value = root[field];
    if (!value.isUnbound() && !value.is<const char *>())
      return false;
  }

  JsonVariantConst dhcpValue = root["dhcp"];
  if (!dhcpValue.isUnbound() && !dhcpValue.is<bool>())
    return false;

  JsonVariantConst mqttPortValue = root["mqttPort"];
  if (!mqttPortValue.isUnbound())
  {
    if (!mqttPortValue.is<unsigned int>())
      return false;
    const unsigned int port = mqttPortValue.as<unsigned int>();
    if (port == 0 || port > 65535)
      return false;
  }

  JsonVariantConst backupValue = root["backup"];
  if (!backupValue.isUnbound() && !backupValue.is<bool>())
    return false;
  return true;
}

void addClaims(const FeaturePinPlan &plan, bool proposed,
               std::vector<PinClaim> &claims)
{
  const std::vector<unsigned int> &inputs = proposed ? plan.inputs : plan.oldInputs;
  const std::vector<unsigned int> &outputs = proposed ? plan.outputs : plan.oldOutputs;
  for (size_t slot = 0; slot < inputs.size(); slot++)
  {
    PinClaim claim;
    claim.id = plan.id;
    claim.kind = plan.kind;
    claim.sensorDriver = plan.sensorDriver;
    claim.role = PinRole::INPUT_PIN;
    claim.slot = slot;
    claim.pin = inputs[slot];
    claims.push_back(claim);
  }
  for (size_t slot = 0; slot < outputs.size(); slot++)
  {
    PinClaim claim;
    claim.id = plan.id;
    claim.kind = plan.kind;
    claim.sensorDriver = plan.sensorDriver;
    claim.role = PinRole::OUTPUT_PIN;
    claim.slot = slot;
    claim.pin = outputs[slot];
    claims.push_back(claim);
  }
}

bool sameClaim(const PinClaim &left, const PinClaim &right)
{
  return left.kind == right.kind && left.role == right.role &&
         left.slot == right.slot && left.pin == right.pin &&
         left.id.equals(right.id);
}

bool containsClaim(const std::vector<PinClaim> &claims, const PinClaim &wanted)
{
  for (const auto &claim : claims)
    if (sameClaim(claim, wanted))
      return true;
  return false;
}

bool pinHasUnchangedOutput(const std::vector<PinClaim> &before,
                           const std::vector<PinClaim> &after,
                           unsigned int pin)
{
  for (const auto &claim : after)
    if (claim.pin == pin && claim.role == PinRole::OUTPUT_PIN &&
        containsClaim(before, claim))
      return true;
  return false;
}

bool validPinForRole(const ConfigOnofre &cfg, const PinClaim &claim)
{
  if (claim.kind == FeatureKind::SENSOR)
    return cfg.validSensorPin(claim.sensorDriver, claim.slot, claim.pin);
  if (claim.role == PinRole::INPUT_PIN)
    return cfg.validInputPin(claim.pin);
  return cfg.validOutputPin(claim.pin);
}

ConfigUpdateResult preparePinUpdate(ConfigOnofre &cfg, JsonObject &root,
                                    std::vector<FeaturePinPlan> &plans,
                                    std::vector<PinClaim> &before,
                                    std::vector<PinClaim> &after)
{
  plans.reserve(cfg.actuatores.size() + cfg.sensors.size());
  for (const auto &actuator : cfg.actuatores)
  {
    FeaturePinPlan plan;
    plan.id = actuator.uniqueId;
    plan.kind = FeatureKind::ACTUATOR;
    plan.driver = actuator.driver;
    plan.oldInputs = actuator.inputs;
    plan.oldOutputs = actuator.outputs;
    plan.inputs = actuator.inputs;
    plan.outputs = actuator.outputs;
    if (plan.id.length() == 0 || planIdExists(plans, plan.id))
      return ConfigUpdateResult::INVALID_REQUEST;
    plans.push_back(plan);
  }
  for (const auto &sensor : cfg.sensors)
  {
    FeaturePinPlan plan;
    plan.id = sensor.uniqueId;
    plan.kind = FeatureKind::SENSOR;
    plan.sensorDriver = sensor.driver;
    // I2C and fixed-UART drivers ignore their stored JSON array at runtime.
    // Validate the physical map they really open, otherwise another feature
    // could be accepted on top of an already-owned bus pin.
    if (!Sensor::fixedRuntimeInputs(sensor.driver, plan.oldInputs))
      plan.oldInputs = sensor.inputs;
    plan.inputs = plan.oldInputs;
    if (plan.id.length() == 0 || planIdExists(plans, plan.id))
      return ConfigUpdateResult::INVALID_REQUEST;
    plans.push_back(plan);
  }

  JsonVariantConst removeValue = root["featuresToRemove"];
  if (!removeValue.isUnbound())
  {
    if (!removeValue.is<JsonArrayConst>())
      return ConfigUpdateResult::INVALID_REQUEST;
    for (JsonVariantConst item : removeValue.as<JsonArrayConst>())
    {
      if (!item.is<const char *>())
        return ConfigUpdateResult::INVALID_REQUEST;
      const String id = item.as<const char *>();

      // Match the mutation below exactly: remove the first actuator with this
      // ID, or a sensor only when no actuator matches. Repeated IDs therefore
      // remove the same sequence that the apply phase will remove.
      FeaturePinPlan *removedPlan = nullptr;
      for (auto &plan : plans)
        if (!plan.removed && plan.kind == FeatureKind::ACTUATOR &&
            plan.id.equals(id))
        {
          removedPlan = &plan;
          break;
        }
      if (removedPlan == nullptr)
        for (auto &plan : plans)
          if (!plan.removed && plan.kind == FeatureKind::SENSOR &&
              plan.id.equals(id))
          {
            removedPlan = &plan;
            break;
          }
      if (removedPlan != nullptr)
        removedPlan->removed = true;
    }
  }

  // A legacy unsupported sensor must not make itself impossible to remove.
  // Reject it only after processing removals, while every surviving driver is
  // still validated before any GPIO or vector mutation.
  for (const auto &plan : plans)
    if (!plan.removed && plan.kind == FeatureKind::SENSOR &&
        !Sensor::isSupportedOnCurrentTarget(plan.sensorDriver))
      return ConfigUpdateResult::INVALID_REQUEST;

  JsonVariantConst featuresValue = root["features"];
  if (!featuresValue.isUnbound())
  {
    if (!featuresValue.is<JsonArrayConst>())
      return ConfigUpdateResult::INVALID_REQUEST;
    for (JsonVariantConst feature : featuresValue.as<JsonArrayConst>())
    {
      if (!feature.is<JsonObjectConst>())
        return ConfigUpdateResult::INVALID_REQUEST;

      const String group = feature["group"] | "";
      FeatureKind kind;
      if (group.equals("ACTUATOR"))
        kind = FeatureKind::ACTUATOR;
      else if (group.equals("SENSOR"))
        kind = FeatureKind::SENSOR;
      else
        continue; // Preserve compatibility with fields/groups this build ignores.

      const String id = feature["id"] | "";
      if (id.length() == 0)
        return ConfigUpdateResult::INVALID_REQUEST;
      FeaturePinPlan *plan = findPlan(plans, id, kind);
      if (plan == nullptr || plan->removed)
        continue; // Unknown and explicitly removed IDs were historically ignored.
      if (plan->seen)
        return ConfigUpdateResult::INVALID_REQUEST;
      plan->seen = true;

      std::vector<unsigned int> pins;
      const PinArrayRead inputs = readPinArrayStrict(feature, "inputs", pins);
      if (inputs == PinArrayRead::INVALID)
        return ConfigUpdateResult::INVALID_REQUEST;
      if (inputs == PinArrayRead::VALID)
      {
        if (kind != FeatureKind::SENSOR)
        {
          if (pins.size() != plan->oldInputs.size())
            return ConfigUpdateResult::PIN_COUNT_MISMATCH;
        }
        else
        {
          const size_t expectedInputs =
              Sensor::expectedInputCount(plan->sensorDriver);
          if (expectedInputs == 0)
            return ConfigUpdateResult::INVALID_REQUEST;
          if (pins.size() != expectedInputs)
            return ConfigUpdateResult::PIN_COUNT_MISMATCH;

          std::vector<unsigned int> fixedInputs;
          if (Sensor::fixedRuntimeInputs(plan->sensorDriver, fixedInputs) &&
              pins != fixedInputs)
            return ConfigUpdateResult::INVALID_PIN;
        }
        plan->inputs = pins;
        plan->inputsProvided = true;
      }

      if (kind == FeatureKind::ACTUATOR)
      {
        auto actuator = std::find_if(cfg.actuatores.begin(), cfg.actuatores.end(),
                                     [id](const Actuator &item)
                                     { return id.equals(item.uniqueId); });
        if (actuator == cfg.actuatores.end())
          return ConfigUpdateResult::INVALID_REQUEST;

        JsonVariantConst inputModeValue = feature["inputMode"];
        if (!inputModeValue.isUnbound())
        {
          if (!inputModeValue.is<unsigned int>())
            return ConfigUpdateResult::INVALID_REQUEST;
          const unsigned int inputMode = inputModeValue.as<unsigned int>();
          if (inputMode > static_cast<unsigned int>(ActuatorInputMode::ROTATE) ||
              !isInputModeSupported(*actuator, inputMode))
            return ConfigUpdateResult::INVALID_REQUEST;
          plan->driver = actuator->findDriver(static_cast<ActuatorInputMode>(inputMode));
          if (plan->driver == ActuatorDriver::INVALID)
            return ConfigUpdateResult::INVALID_REQUEST;
        }

        const PinArrayRead outputs = readPinArrayStrict(feature, "outputs", pins);
        if (outputs == PinArrayRead::INVALID)
          return ConfigUpdateResult::INVALID_REQUEST;
        if (outputs == PinArrayRead::VALID)
        {
          if (pins.size() != plan->oldOutputs.size())
            return ConfigUpdateResult::PIN_COUNT_MISMATCH;
          plan->outputs = pins;
          plan->outputsProvided = true;
        }

        const ConfigUpdateResult topology = validateActuatorTopology(*actuator, *plan);
        if (topology != ConfigUpdateResult::OK)
          return topology;

        const unsigned long nextUpCourseTime =
            feature["upCourseTime"] | actuator->upCourseTime;
        const unsigned long nextDownCourseTime =
            feature["downCourseTime"] | actuator->downCourseTime;
        const uint8_t nextKnxArea = feature["area"] | actuator->knxAddress[0];
        const uint8_t nextKnxLine = feature["line"] | actuator->knxAddress[1];
        const uint8_t nextKnxMember = feature["member"] | actuator->knxAddress[2];
        plan->restartRequired = plan->oldOutputs != plan->outputs ||
                                actuator->upCourseTime != nextUpCourseTime ||
                                actuator->downCourseTime != nextDownCourseTime ||
                                actuator->knxAddress[0] != nextKnxArea ||
                                actuator->knxAddress[1] != nextKnxLine ||
                                actuator->knxAddress[2] != nextKnxMember;
      }
    }

    for (auto &plan : plans)
      if (plan.kind == FeatureKind::SENSOR)
        plan.restartRequired = plan.oldInputs != plan.inputs;
  }

  for (const auto &plan : plans)
  {
    addClaims(plan, false, before);
    if (!plan.removed)
      addClaims(plan, true, after);
  }

  // A historical configuration may contain several logical owners for the
  // same output. Grandfather it only while every owner remains unchanged. If
  // one owner is removed or remapped, there is no safe physical level that can
  // be handed to the survivor without knowing the external wiring.
  for (const auto &oldClaim : before)
    if (oldClaim.role == PinRole::OUTPUT_PIN &&
        !containsClaim(after, oldClaim) &&
        pinHasUnchangedOutput(before, after, oldClaim.pin))
      return ConfigUpdateResult::PIN_CONFLICT;

  // Grandfather exact unchanged claims. Field devices legitimately share I2C
  // or addressed serial bus pins, and older firmware also allowed some maps we
  // would reject today. Only a newly introduced claim must be valid and unique
  // in the final configuration.
  for (size_t i = 0; i < after.size(); i++)
  {
    if (containsClaim(before, after[i]))
      continue;
    if (!validPinForRole(cfg, after[i]))
      return ConfigUpdateResult::INVALID_PIN;
    for (size_t j = 0; j < after.size(); j++)
      if (i != j && after[i].pin == after[j].pin)
        return ConfigUpdateResult::PIN_CONFLICT;
  }
  return ConfigUpdateResult::OK;
}

bool pinHasRole(const std::vector<PinClaim> &claims, unsigned int pin, PinRole role)
{
  for (const auto &claim : claims)
    if (claim.pin == pin && claim.role == role)
      return true;
  return false;
}

bool pinAlreadyReleased(const std::vector<unsigned int> &released, unsigned int pin)
{
  for (auto item : released)
    if (item == pin)
      return true;
  return false;
}

void parkOutput(unsigned int pin, bool remainsAnInput)
{
  if (!remainsAnInput)
  {
    configPIN(pin, OUTPUT);
    writeToPIN(pin, 0);
  }
  configPIN(pin, INPUT);
}

void releaseChangedOutputs(ConfigOnofre &cfg,
                           const std::vector<FeaturePinPlan> &plans,
                           const std::vector<PinClaim> &before,
                           const std::vector<PinClaim> &after)
{
  std::vector<unsigned int> released;
  for (const auto &plan : plans)
  {
    if (plan.kind != FeatureKind::ACTUATOR)
      continue;
    for (size_t slot = 0; slot < plan.oldOutputs.size(); slot++)
    {
      PinClaim oldClaim;
      oldClaim.id = plan.id;
      oldClaim.kind = plan.kind;
      oldClaim.role = PinRole::OUTPUT_PIN;
      oldClaim.slot = slot;
      oldClaim.pin = plan.oldOutputs[slot];
      if (containsClaim(after, oldClaim) ||
          pinHasUnchangedOutput(before, after, oldClaim.pin) ||
          !cfg.validOutputPin(oldClaim.pin) ||
          pinAlreadyReleased(released, oldClaim.pin))
        continue;
      const bool remainsAnInput = pinHasRole(after, oldClaim.pin, PinRole::INPUT_PIN);
      parkOutput(oldClaim.pin, remainsAnInput);
      released.push_back(oldClaim.pin);
    }
  }
}
} // namespace

ConfigUpdateResult ConfigOnofre::stageRestore(JsonObject &root)
{
  if (strcmp(root["format"] | "", "easyiot-backup") != 0 ||
      (root["version"] | 0u) != 1u ||
      !root["target"].is<JsonObject>() ||
      !root["configuration"].is<JsonObject>() ||
      !root["irrigation"].is<JsonObject>())
    return ConfigUpdateResult::INVALID_REQUEST;

  JsonObjectConst target = root["target"].as<JsonObjectConst>();
  if (strcmp(target["mcu"] | "", currentMcuName()) != 0)
    return ConfigUpdateResult::INVALID_REQUEST;

  JsonObjectConst submitted = root["configuration"].as<JsonObjectConst>();
  static constexpr const char *forbiddenCredentialFields[] = {
      "mqttPassword", "wifiSecret", "accessPointPassword", "apiPassword",
      "cloudIOPassword"};
  for (const char *field : forbiddenCredentialFields)
    if (!submitted[field].isUnbound())
      return ConfigUpdateResult::INVALID_REQUEST;

  const char *restoredNodeId = nullptr;
  const char *restoredMqttHost = nullptr;
  const char *restoredMqttUsername = nullptr;
  const char *restoredCloudIOUsername = nullptr;
  const char *restoredWifiSsid = nullptr;
  const char *restoredWifiIp = nullptr;
  const char *restoredWifiMask = nullptr;
  const char *restoredWifiGw = nullptr;
  const char *restoredApiUser = nullptr;
  if (!boundedString(submitted, "nodeId", sizeof(nodeId), restoredNodeId) ||
      !boundedString(submitted, "mqttIpDns", sizeof(mqttIpDns), restoredMqttHost) ||
      !boundedString(submitted, "mqttUsername", sizeof(mqttUsername), restoredMqttUsername) ||
      !boundedString(submitted, "cloudIOUsername", sizeof(cloudIOUsername),
                     restoredCloudIOUsername) ||
      !boundedString(submitted, "wifiSSID", sizeof(wifiSSID), restoredWifiSsid) ||
      !boundedString(submitted, "wifiIp", sizeof(wifiIp), restoredWifiIp) ||
      !boundedString(submitted, "wifiMask", sizeof(wifiMask), restoredWifiMask) ||
      !boundedString(submitted, "wifiGw", sizeof(wifiGw), restoredWifiGw) ||
      !boundedString(submitted, "apiUser", sizeof(apiUser), restoredApiUser))
    return ConfigUpdateResult::INVALID_REQUEST;

  JsonVariantConst templateValue = submitted["templateId"];
  JsonVariantConst mqttPortValue = submitted["mqttPort"];
  JsonVariantConst dhcpValue = submitted["dhcp"];
  JsonVariantConst featuresValue = submitted["features"];
  if (!templateValue.is<unsigned int>() ||
      templateValue.as<unsigned int>() > static_cast<unsigned int>(Template::GARDEN) ||
      !mqttPortValue.is<unsigned int>() || mqttPortValue.as<unsigned int>() == 0 ||
      mqttPortValue.as<unsigned int>() > 65535u || !dhcpValue.is<bool>() ||
      !featuresValue.is<JsonArrayConst>() ||
      featuresValue.as<JsonArrayConst>().size() > maxRestoreFeatures)
    return ConfigUpdateResult::INVALID_REQUEST;

  JsonDocument transaction;
  transaction["format"] = "easyiot-restore-transaction";
  transaction["version"] = 1;
  transaction["committed"] = false;
  transaction["hadConfig"] = LittleFS.exists(configFilenames::config);
  transaction["hadIrrigation"] = LittleFS.exists(configFilenames::irrigation);
  JsonObject restoredConfig = transaction["config"].to<JsonObject>();
  restoredConfig["templateId"] = templateValue.as<unsigned int>();
  restoredConfig["nodeId"] = restoredNodeId;
  restoredConfig["mqttIpDns"] = restoredMqttHost;
  restoredConfig["mqttPort"] = mqttPortValue.as<unsigned int>();
  restoredConfig["mqttUsername"] = restoredMqttUsername;
  if (mqttPassword[0])
    restoredConfig["mqttPassword"] = mqttPassword;
  restoredConfig["cloudIOUsername"] = restoredCloudIOUsername;
  if (cloudIOPassword[0])
    restoredConfig["cloudIOPassword"] = cloudIOPassword;
  restoredConfig["wifiSSID"] = restoredWifiSsid;
  if (wifiSecret[0])
    restoredConfig["wifiSecret"] = wifiSecret;
  restoredConfig["dhcp"] = dhcpValue.as<bool>();
  restoredConfig["wifiIp"] = restoredWifiIp;
  restoredConfig["wifiMask"] = restoredWifiMask;
  restoredConfig["wifiGw"] = restoredWifiGw;
  restoredConfig["accessPointPassword"] = accessPointPassword;
  restoredConfig["apiUser"] = restoredApiUser;
  restoredConfig["apiPassword"] = apiPassword;

  std::vector<FeaturePinPlan> plans;
  std::vector<String> valveIds;
  JsonArray storedFeatures = restoredConfig["features"].to<JsonArray>();
  for (JsonVariantConst featureValue : featuresValue.as<JsonArrayConst>())
  {
    if (!featureValue.is<JsonObjectConst>())
      return ConfigUpdateResult::INVALID_REQUEST;
    JsonObjectConst feature = featureValue.as<JsonObjectConst>();
    const char *id = nullptr;
    const char *name = nullptr;
    if (!boundedString(feature, "id", sizeof(Actuator{}.uniqueId), id) || !id[0] ||
        !boundedString(feature, "name", sizeof(Actuator{}.name), name) || !name[0])
      return ConfigUpdateResult::INVALID_REQUEST;
    for (const auto &plan : plans)
      if (plan.id.equals(id))
        return ConfigUpdateResult::INVALID_REQUEST;

    const String group = feature["group"] | "";
    JsonVariantConst driverValue = feature["driver"];
    if (!driverValue.is<unsigned int>())
      return ConfigUpdateResult::INVALID_REQUEST;

    FeaturePinPlan plan;
    plan.id = id;
    std::vector<unsigned int> pins;
    if (readPinArrayStrict(feature, "inputs", pins) != PinArrayRead::VALID ||
        pins.size() > 4)
      return ConfigUpdateResult::INVALID_REQUEST;
    plan.inputs = pins;

    JsonObject storedFeature = storedFeatures.add<JsonObject>();
    storedFeature["group"] = group;
    storedFeature["id"] = id;
    storedFeature["name"] = name;

    if (group.equals("ACTUATOR"))
    {
      plan.kind = FeatureKind::ACTUATOR;
      plan.driver = static_cast<ActuatorDriver>(driverValue.as<unsigned int>());
      JsonVariantConst controlValue = feature["typeControl"];
      if (!supportedActuatorDriver(plan.driver) ||
          !controlValue.is<unsigned int>() ||
          (controlValue.as<unsigned int>() != ActuatorControlType::GPIO_OUTPUT &&
           controlValue.as<unsigned int>() != ActuatorControlType::VIRTUAL) ||
          readPinArrayStrict(feature, "outputs", pins) != PinArrayRead::VALID ||
          pins.size() > 2)
        return ConfigUpdateResult::INVALID_REQUEST;
      plan.outputs = pins;

      const bool dualCover = plan.driver == ActuatorDriver::COVER_DUAL_PUSH ||
                             plan.driver == ActuatorDriver::COVER_DUAL_LATCH;
      const bool singleCover = plan.driver == ActuatorDriver::COVER_SINGLE_PUSH;
      const bool garage = plan.driver == ActuatorDriver::GARAGE_PUSH;
      const bool virtualControl =
          controlValue.as<unsigned int>() == ActuatorControlType::VIRTUAL;
      const size_t expectedInputs = dualCover ? 2u : singleCover ? 1u : garage ? 2u : 1u;
      if ((!virtualControl && plan.outputs.size() != (dualCover || singleCover || garage ? 2u : 1u)) ||
          (virtualControl && !plan.outputs.empty()) ||
          (plan.driver == ActuatorDriver::GARDEN_VALVE && plan.inputs.size() > 1) ||
          (plan.driver != ActuatorDriver::GARDEN_VALVE && plan.inputs.size() != expectedInputs))
        return ConfigUpdateResult::PIN_COUNT_MISMATCH;

      JsonVariantConst upValue = feature["upCourseTime"];
      JsonVariantConst downValue = feature["downCourseTime"];
      JsonVariantConst autoOffValue = feature["autoOff"];
      JsonVariantConst stateValue = feature["state"];
      JsonVariantConst areaValue = feature["area"];
      JsonVariantConst lineValue = feature["line"];
      JsonVariantConst memberValue = feature["member"];
      if (!upValue.is<unsigned long>() || !downValue.is<unsigned long>() ||
          !autoOffValue.is<unsigned long>() || !stateValue.is<unsigned int>() ||
          stateValue.as<unsigned int>() > ActuatorState::STOP ||
          !areaValue.is<unsigned int>() || areaValue.as<unsigned int>() > 255u ||
          !lineValue.is<unsigned int>() || lineValue.as<unsigned int>() > 255u ||
          !memberValue.is<unsigned int>() || memberValue.as<unsigned int>() > 255u)
        return ConfigUpdateResult::INVALID_REQUEST;

      storedFeature["driver"] = plan.driver;
      storedFeature["typeControl"] = controlValue.as<unsigned int>();
      storedFeature["upCourseTime"] = upValue.as<unsigned long>();
      storedFeature["downCourseTime"] = downValue.as<unsigned long>();
      storedFeature["area"] = areaValue.as<unsigned int>();
      storedFeature["line"] = lineValue.as<unsigned int>();
      storedFeature["member"] = memberValue.as<unsigned int>();
      storedFeature["autoOff"] = autoOffValue.as<unsigned long>();
      storedFeature["state"] = plan.driver == ActuatorDriver::GARDEN_VALVE
                                   ? ActuatorState::OFF_OPEN
                                   : stateValue.as<unsigned int>();
      if (plan.driver == ActuatorDriver::GARDEN_VALVE)
        valveIds.push_back(id);
    }
    else if (group.equals("SENSOR"))
    {
      plan.kind = FeatureKind::SENSOR;
      plan.sensorDriver = static_cast<SensorDriver>(driverValue.as<unsigned int>());
      if (!Sensor::isSupportedOnCurrentTarget(plan.sensorDriver) ||
          plan.inputs.size() != Sensor::expectedInputCount(plan.sensorDriver))
        return ConfigUpdateResult::PIN_COUNT_MISMATCH;
      std::vector<unsigned int> fixedInputs;
      if (Sensor::fixedRuntimeInputs(plan.sensorDriver, fixedInputs) &&
          plan.inputs != fixedInputs)
        return ConfigUpdateResult::INVALID_PIN;
      JsonVariantConst addressValue = feature["hwAddress"];
      JsonVariantConst delayValue = feature["delayRead"];
      if (!addressValue.is<unsigned int>() || addressValue.as<unsigned int>() > 255u ||
          !delayValue.is<unsigned long>() || delayValue.as<unsigned long>() == 0)
        return ConfigUpdateResult::INVALID_REQUEST;
      storedFeature["driver"] = plan.sensorDriver;
      storedFeature["hwAddress"] = addressValue.as<unsigned int>();
      storedFeature["delayRead"] = delayValue.as<unsigned long>();
    }
    else
    {
      return ConfigUpdateResult::INVALID_REQUEST;
    }

    JsonArray storedInputs = storedFeature["inputs"].to<JsonArray>();
    for (auto pin : plan.inputs)
      storedInputs.add(pin);
    if (plan.kind == FeatureKind::ACTUATOR)
    {
      JsonArray storedOutputs = storedFeature["outputs"].to<JsonArray>();
      for (auto pin : plan.outputs)
        storedOutputs.add(pin);
    }
    plans.push_back(plan);
  }

  std::vector<PinClaim> claims;
  for (const auto &plan : plans)
    addClaims(plan, true, claims);
  for (size_t i = 0; i < claims.size(); i++)
  {
    if (!validPinForRole(*this, claims[i]))
      return ConfigUpdateResult::INVALID_PIN;
    for (size_t j = i + 1; j < claims.size(); j++)
      if (claims[i].pin == claims[j].pin &&
          !restoreClaimsMayShare(claims[i], claims[j]))
        return ConfigUpdateResult::PIN_CONFLICT;
  }

  JsonObjectConst submittedIrrigation = root["irrigation"].as<JsonObjectConst>();
  JsonVariantConst irrEnabled = submittedIrrigation["enabled"];
  JsonVariantConst skipOnRain = submittedIrrigation["skipOnRain"];
  JsonVariantConst programsValue = submittedIrrigation["programs"];
  if (!irrEnabled.is<bool>() || !skipOnRain.is<bool>() ||
      !programsValue.is<JsonArrayConst>() ||
      programsValue.as<JsonArrayConst>().size() > maxRestorePrograms)
    return ConfigUpdateResult::INVALID_REQUEST;

  // A backup carries the concurrency limit; one taken before the setting existed
  // does not. Restoring must not quietly put the installation back to one zone —
  // the whole point of a restore is that nothing else changes — so an absent
  // value keeps what this device is set to.
  JsonVariantConst submittedMaxZones = submittedIrrigation["maxConcurrentZones"];
  uint8_t restoredMaxZones = irrigation.openZoneLimit();
  if (!submittedMaxZones.isNull())
  {
    if (!submittedMaxZones.is<unsigned int>())
      return ConfigUpdateResult::INVALID_REQUEST;
    const unsigned int wantedMaxZones = submittedMaxZones.as<unsigned int>();
    if (wantedMaxZones < 1u || wantedMaxZones > kMaxConcurrentZones)
      return ConfigUpdateResult::INVALID_REQUEST;
    restoredMaxZones = static_cast<uint8_t>(wantedMaxZones);
  }

  JsonObject restoredIrrigation = transaction["irrigation"].to<JsonObject>();
  restoredIrrigation["enabled"] = irrEnabled.as<bool>();
  restoredIrrigation["skipOnRain"] = skipOnRain.as<bool>();
  restoredIrrigation["maxConcurrentZones"] = restoredMaxZones;
  JsonArray storedPrograms = restoredIrrigation["programs"].to<JsonArray>();
  std::vector<unsigned int> programIds;
  for (JsonVariantConst programValue : programsValue.as<JsonArrayConst>())
  {
    if (!programValue.is<JsonObjectConst>())
      return ConfigUpdateResult::INVALID_REQUEST;
    JsonObjectConst program = programValue.as<JsonObjectConst>();
    JsonVariantConst idValue = program["id"];
    JsonVariantConst enabledValue = program["enabled"];
    JsonVariantConst startValue = program["startMinute"];
    JsonVariantConst weekdaysValue = program["weekdays"];
    JsonVariantConst zonesValue = program["zones"];
    if (!idValue.is<unsigned int>() || idValue.as<unsigned int>() == 0 ||
        idValue.as<unsigned int>() > 255u || !enabledValue.is<bool>() ||
        !startValue.is<unsigned int>() || startValue.as<unsigned int>() > 1439u ||
        !weekdaysValue.is<unsigned int>() || weekdaysValue.as<unsigned int>() > 0x7fu ||
        !zonesValue.is<JsonArrayConst>())
      return ConfigUpdateResult::INVALID_REQUEST;
    for (auto priorId : programIds)
      if (priorId == idValue.as<unsigned int>())
        return ConfigUpdateResult::INVALID_REQUEST;
    programIds.push_back(idValue.as<unsigned int>());

    JsonObject storedProgram = storedPrograms.add<JsonObject>();
    storedProgram["id"] = idValue.as<unsigned int>();
    storedProgram["enabled"] = enabledValue.as<bool>();
    storedProgram["startMinute"] = startValue.as<unsigned int>();
    storedProgram["weekdays"] = weekdaysValue.as<unsigned int>();
    JsonArray storedZones = storedProgram["zones"].to<JsonArray>();
    std::vector<String> seenZones;
    for (JsonVariantConst zoneValue : zonesValue.as<JsonArrayConst>())
    {
      if (!zoneValue.is<JsonObjectConst>())
        return ConfigUpdateResult::INVALID_REQUEST;
      JsonObjectConst zone = zoneValue.as<JsonObjectConst>();
      const char *zoneId = nullptr;
      JsonVariantConst minutesValue = zone["minutes"];
      if (!boundedString(zone, "uniqueId", sizeof(IrrigationZone{}.uniqueId), zoneId) ||
          !zoneId[0] || !minutesValue.is<unsigned int>() ||
          minutesValue.as<unsigned int>() == 0 ||
          minutesValue.as<unsigned int>() > maxRestoreZoneMinutes)
        return ConfigUpdateResult::INVALID_REQUEST;
      bool knownValve = false;
      for (const auto &valveId : valveIds)
        knownValve = knownValve || valveId.equals(zoneId);
      for (const auto &seen : seenZones)
        if (seen.equals(zoneId))
          return ConfigUpdateResult::INVALID_REQUEST;
      if (!knownValve)
        return ConfigUpdateResult::INVALID_REQUEST;
      seenZones.push_back(zoneId);
      JsonObject storedZone = storedZones.add<JsonObject>();
      storedZone["uniqueId"] = zoneId;
      storedZone["minutes"] = minutesValue.as<unsigned int>();
    }
  }

  if (transaction.overflowed())
    return ConfigUpdateResult::PERSISTENCE_FAILED;
  if (!persistJsonAtomically(configFilenames::restore,
                             configFilenames::restoreTemporary, transaction))
    return ConfigUpdateResult::PERSISTENCE_FAILED;
  return ConfigUpdateResult::OK;
}

ConfigUpdateResult ConfigOnofre::update(JsonObject &root, JsonVariant &responseRoot)
{
  if (root.isNull() || !validateConfigScalarTypes(root))
    return ConfigUpdateResult::INVALID_REQUEST;

  // The legacy restore path is intentionally unreachable. It bypasses the pin
  // preflight, drops sensors, regenerates IDs and cannot restore the credentials
  // omitted by GET /config. Keep the old parser only as migration reference
  // until a versioned candidate/rollback design replaces it.
  JsonVariantConst backupValue = root["backup"];
  if (!backupValue.isUnbound() && backupValue.as<bool>())
    return ConfigUpdateResult::INVALID_REQUEST;
  if (!tryBeginConfigUpdate())
    return ConfigUpdateResult::BUSY;

  // PubSubClient keeps a pointer to the configured broker string and an
  // established session does not automatically reconnect when credentials
  // change. Remember the previous values while this exclusive lease is held;
  // if any differ after the validated apply, disconnect and reconfigure the
  // synchronous client before releasing the lease.
  char previousMqttHost[sizeof(mqttIpDns)] = {};
  char previousMqttUsername[sizeof(mqttUsername)] = {};
  char previousMqttPassword[sizeof(mqttPassword)] = {};
  strlcpy(previousMqttHost, mqttIpDns, sizeof(previousMqttHost));
  strlcpy(previousMqttUsername, mqttUsername, sizeof(previousMqttUsername));
  strlcpy(previousMqttPassword, mqttPassword, sizeof(previousMqttPassword));
  const int previousMqttPort = mqttPort;

  // Network identity and addressing are consumed by Wi-Fi, mDNS and MQTT
  // outside this request. Reboot after the response whenever those persisted
  // values change so the live services cannot keep using an old snapshot.
  char previousNodeId[sizeof(nodeId)] = {};
  char previousWifiSSID[sizeof(wifiSSID)] = {};
  char previousWifiSecret[sizeof(wifiSecret)] = {};
  char previousWifiIp[sizeof(wifiIp)] = {};
  char previousWifiMask[sizeof(wifiMask)] = {};
  char previousWifiGw[sizeof(wifiGw)] = {};
  char previousAccessPointPassword[sizeof(accessPointPassword)] = {};
  char previousApiUser[sizeof(apiUser)] = {};
  char previousApiPassword[sizeof(apiPassword)] = {};
  strlcpy(previousNodeId, nodeId, sizeof(previousNodeId));
  strlcpy(previousWifiSSID, wifiSSID, sizeof(previousWifiSSID));
  strlcpy(previousWifiSecret, wifiSecret, sizeof(previousWifiSecret));
  strlcpy(previousWifiIp, wifiIp, sizeof(previousWifiIp));
  strlcpy(previousWifiMask, wifiMask, sizeof(previousWifiMask));
  strlcpy(previousWifiGw, wifiGw, sizeof(previousWifiGw));
  strlcpy(previousAccessPointPassword, accessPointPassword,
          sizeof(previousAccessPointPassword));
  strlcpy(previousApiUser, apiUser, sizeof(previousApiUser));
  strlcpy(previousApiPassword, apiPassword, sizeof(previousApiPassword));
  const bool previousDhcp = dhcp;

  const bool restore = false;
  std::vector<FeaturePinPlan> pinPlans;
  std::vector<PinClaim> oldClaims;
  std::vector<PinClaim> newClaims;
  if (!restore)
  {
    const ConfigUpdateResult result = preparePinUpdate(*this, root, pinPlans,
                                                       oldClaims, newClaims);
    if (result != ConfigUpdateResult::OK)
    {
      endConfigUpdate();
      return result;
    }


    bool deactivateActuators = false;
    for (const auto &plan : pinPlans)
      if (plan.kind == FeatureKind::ACTUATOR &&
          (plan.removed || plan.restartRequired))
      {
        deactivateActuators = true;
        break;
      }

    // A running irrigation cycle must close its valve while the old actuator
    // map is still live. Then make removed/restart-pending actuators inert;
    // covers synchronously halt both relays before any GPIO ownership changes.
    if (deactivateActuators && irrigation.isRunning())
      irrigation.stop();
    for (const auto &plan : pinPlans)
    {
      if (plan.kind != FeatureKind::ACTUATOR ||
          (!plan.removed && !plan.restartRequired))
        continue;
      auto actuator = std::find_if(actuatores.begin(), actuatores.end(),
                                   [&plan](const Actuator &item)
                                   { return plan.id.equals(item.uniqueId); });
      if (actuator != actuatores.end())
        actuator->deactivateForConfigUpdate();
    }
    for (const auto &plan : pinPlans)
    {
      if (plan.kind != FeatureKind::SENSOR || plan.removed ||
          !plan.restartRequired)
        continue;
      auto sensor = std::find_if(sensors.begin(), sensors.end(),
                                 [&plan](const Sensor &item)
                                 { return plan.id.equals(item.uniqueId); });
      if (sensor != sensors.end())
        sensor->deactivateForConfigUpdate();
    }

    // Finish every hand-off before any feature is erased or configured on its
    // new pins. Inputs and shared buses are never driven LOW here.
    releaseChangedOutputs(*this, pinPlans, oldClaims, newClaims);
  }

  if (restore)
  {
    actuatores.clear();
    sensors.clear();
  }
  JsonVariantConst dhcpValue = root["dhcp"];
  if (!dhcpValue.isUnbound())
    dhcp = dhcpValue.as<bool>();

  JsonVariantConst mqttPasswordValue = root["mqttPassword"];
  if (!mqttPasswordValue.isUnbound() &&
      strcmp(mqttPasswordValue.as<const char *>(), constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(mqttPassword, mqttPasswordValue.as<const char *>(), sizeof(mqttPassword));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Mqtt Password changed." CR, tags::config);
#endif
  }

  JsonVariantConst wifiSecretValue = root["wifiSecret"];
  if (!wifiSecretValue.isUnbound() &&
      strcmp(wifiSecretValue.as<const char *>(), constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(wifiSecret, wifiSecretValue.as<const char *>(), sizeof(wifiSecret));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Wifi Password changed." CR, tags::config);
#endif
  }

  JsonVariantConst accessPointPasswordValue = root["accessPointPassword"];
  if (!accessPointPasswordValue.isUnbound() &&
      strcmp(accessPointPasswordValue.as<const char *>(), constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(accessPointPassword, accessPointPasswordValue.as<const char *>(),
            sizeof(accessPointPassword));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Access Point Password changed." CR, tags::config);
#endif
  }

  JsonVariantConst apiPasswordValue = root["apiPassword"];
  if (!apiPasswordValue.isUnbound() &&
      strcmp(apiPasswordValue.as<const char *>(), constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(apiPassword, apiPasswordValue.as<const char *>(), sizeof(apiPassword));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Api Password changed." CR, tags::config);
#endif
  }

  JsonVariantConst nodeIdValue = root["nodeId"];
  if (!nodeIdValue.isUnbound())
  {
    String normalizedName = nodeIdValue.as<const char *>();
    normalize(normalizedName);
    if (normalizedName.isEmpty())
      normalizedName = chipId;
    strlcpy(nodeId, normalizedName.c_str(), sizeof(nodeId));
  }
  JsonVariantConst mqttHostValue = root["mqttIpDns"];
  if (!mqttHostValue.isUnbound())
    strlcpy(mqttIpDns, mqttHostValue.as<const char *>(), sizeof(mqttIpDns));
  JsonVariantConst mqttPortValue = root["mqttPort"];
  if (!mqttPortValue.isUnbound())
    mqttPort = mqttPortValue.as<unsigned int>();
  JsonVariantConst mqttUsernameValue = root["mqttUsername"];
  if (!mqttUsernameValue.isUnbound())
    strlcpy(mqttUsername, mqttUsernameValue.as<const char *>(), sizeof(mqttUsername));
  JsonVariantConst wifiSsidValue = root["wifiSSID"];
  if (!wifiSsidValue.isUnbound())
    strlcpy(wifiSSID, wifiSsidValue.as<const char *>(), sizeof(wifiSSID));
  JsonVariantConst wifiIpValue = root["wifiIp"];
  if (!dhcp && !wifiIpValue.isUnbound())
    strlcpy(wifiIp, wifiIpValue.as<const char *>(), sizeof(wifiIp));
  JsonVariantConst wifiMaskValue = root["wifiMask"];
  if (!dhcp && !wifiMaskValue.isUnbound())
    strlcpy(wifiMask, wifiMaskValue.as<const char *>(), sizeof(wifiMask));
  JsonVariantConst wifiGwValue = root["wifiGw"];
  if (!dhcp && !wifiGwValue.isUnbound())
    strlcpy(wifiGw, wifiGwValue.as<const char *>(), sizeof(wifiGw));
  JsonVariantConst apiUserValue = root["apiUser"];
  if (!apiUserValue.isUnbound())
    strlcpy(apiUser, apiUserValue.as<const char *>(), sizeof(apiUser));
  JsonArray featuresToRemove = root["featuresToRemove"];
  for (String id : featuresToRemove)
  {
    auto match = std::find_if(actuatores.begin(), actuatores.end(), [id](const Actuator &item)
                              { return id.equals(item.uniqueId); });
    if (match != actuatores.end())
    {
      removeFromHomeAssistant("light", id);
      removeFromHomeAssistant("switch", id);
      removeFromHomeAssistant("cover", id);
      actuatores.erase(match);
    }
    else
    {
      auto match = std::find_if(sensors.begin(), sensors.end(), [id](const Sensor &item)
                                { return id.equals(item.uniqueId); });
      if (match != sensors.end())
      {
        removeFromHomeAssistant("binary_sensor", id.c_str());
        removeFromHomeAssistant("sensor", id.c_str());
        sensors.erase(match);
      }
    }
  }
  JsonArray features = root["features"];
  int counter = 0;
  bool restartRequired = false;
  for (auto feature : features)
  {
    counter++;
    String id = feature["id"] | "";
    if (String("ACTUATOR").equals(feature["group"] | ""))
    {
      if (restore)
      {
        Actuator actuator;
        if (strlen(feature["name"] | I18N::NO_NAME) > 0)
          strlcpy(actuator.name, feature["name"] | I18N::NO_NAME, sizeof(actuator.name));
        actuator.driver = actuator.findDriverFromName(feature["driver"] | "INVALID");
        String idStr;
        generateId(idStr, actuator.name, actuator.driver, counter, sizeof(actuator.uniqueId));
        strlcpy(actuator.uniqueId, idStr.c_str(), sizeof(actuator.uniqueId));
        actuator.upCourseTime = feature["upCourseTime"] | constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;
        actuator.downCourseTime = feature["downCourseTime"] | constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;
        actuator.typeControl = feature["typeControl"] | ActuatorControlType::VIRTUAL;
        actuator.knxAddress[0] = feature["area"] | 0;
        actuator.knxAddress[1] = feature["line"] | 0;
        actuator.knxAddress[2] = feature["member"] | 0;
        actuator.autoOff = feature["autoOff"] | 0ul;
        actuator.state = feature["state"] | 0;
        JsonArray inputsJson = feature["inputs"];
        for (auto in : inputsJson)
        {
          actuator.inputs.push_back(in | 0u);
        }
        JsonArray outputsJson = feature["outputs"];
        for (auto out : outputsJson)
        {
          actuator.outputs.push_back(out | 0u);
        }
        actuator.setup();
        actuatores.push_back(actuator);
      }
      else
      {
        auto match = std::find_if(actuatores.begin(), actuatores.end(), [id](const Actuator &item)
                                  { return id.equals(item.uniqueId); });
        if (match != actuatores.end())
        {
          Actuator &actuator = *match;
          FeaturePinPlan *plan = findPlan(pinPlans, id, FeatureKind::ACTUATOR);
          if (plan == nullptr)
            continue;

          const ActuatorDriver oldDriver = actuator.driver;
          const std::vector<unsigned int> oldInputs = actuator.inputs;
          const unsigned long nextUpCourseTime = feature["upCourseTime"] | actuator.upCourseTime;
          const unsigned long nextDownCourseTime = feature["downCourseTime"] | actuator.downCourseTime;
          const uint8_t nextKnxArea = feature["area"] | actuator.knxAddress[0];
          const uint8_t nextKnxLine = feature["line"] | actuator.knxAddress[1];
          const uint8_t nextKnxMember = feature["member"] | actuator.knxAddress[2];

          const bool driverChanged = oldDriver != plan->driver;
          const bool inputsChanged = oldInputs != plan->inputs;
          const bool restartRequiredForActuator = plan->restartRequired;

          if (strlen(feature["name"] | I18N::NO_NAME) > 0)
            strlcpy(actuator.name, feature["name"] | I18N::NO_NAME, sizeof(actuator.name));
          actuator.driver = plan->driver;
          actuator.upCourseTime = nextUpCourseTime;
          actuator.downCourseTime = nextDownCourseTime;
          actuator.knxAddress[0] = nextKnxArea;
          actuator.knxAddress[1] = nextKnxLine;
          actuator.knxAddress[2] = nextKnxMember;
          actuator.autoOff = feature["autoOff"] | actuator.autoOff;
          if (plan->inputsProvided)
            actuator.inputs = plan->inputs;
          if (plan->outputsProvided)
            actuator.outputs = plan->outputs;

          if (restartRequiredForActuator)
          {
            // Output, shutter-course and KNX changes need full hardware setup.
            // Re-running setup inside the request can pulse relays, duplicate
            // KNX callbacks and leak the current shutter controller. Keep the
            // feature inert until the controlled restart rebuilds everything.
            restartRequired = true;
          }
          else if (inputsChanged || driverChanged)
          {
            actuator.rebuildInputHandlers();
          }
        }
      }
    }
    if (String("SENSOR").equals(feature["group"] | ""))
    {
      if (restore)
      {
      }
      else
      {
        auto match = std::find_if(sensors.begin(), sensors.end(), [id](const Sensor &item)
                                  { return id.equals(item.uniqueId); });
        if (match != sensors.end())
        {
          Sensor &sensor = *match;
          if (strlen(feature["name"] | I18N::NO_NAME) > 0)
            strlcpy(sensor.name, feature["name"] | I18N::NO_NAME, sizeof(sensor.name));

          FeaturePinPlan *plan = findPlan(pinPlans, id, FeatureKind::SENSOR);
          if (plan != nullptr && plan->inputsProvided)
          {
            sensor.inputs = plan->inputs;
            if (plan->restartRequired)
              restartRequired = true;
          }
        }
      }
    }
  }
  const bool mqttChanged = previousMqttPort != mqttPort ||
                           strcmp(previousMqttHost, mqttIpDns) != 0 ||
                           strcmp(previousMqttUsername, mqttUsername) != 0 ||
                           strcmp(previousMqttPassword, mqttPassword) != 0;
  if (mqttChanged)
    setupMQTT(true);
  const bool networkChanged = previousDhcp != dhcp ||
                              strcmp(previousNodeId, nodeId) != 0 ||
                              strcmp(previousWifiSSID, wifiSSID) != 0 ||
                              strcmp(previousWifiSecret, wifiSecret) != 0 ||
                              strcmp(previousWifiIp, wifiIp) != 0 ||
                              strcmp(previousWifiMask, wifiMask) != 0 ||
                              strcmp(previousWifiGw, wifiGw) != 0 ||
                              strcmp(previousAccessPointPassword,
                                     accessPointPassword) != 0;
  const bool apiCredentialsChanged =
      strcmp(previousApiUser, apiUser) != 0 ||
      strcmp(previousApiPassword, apiPassword) != 0;
  if (networkChanged || apiCredentialsChanged)
  {
    restartRequired = true;
  }
  root.clear();
  if (!persist())
  {
    // The previous atomic file is still authoritative, but the live graph has
    // already changed. Transfer ownership of the feature lease to a controlled
    // response-after-disconnect restart so no loop can act on an undurable map.
    responseRoot["restartRequired"] = true;
    return ConfigUpdateResult::PERSISTENCE_FAILED;
  }
  json(responseRoot, true);
  if (restartRequired)
    responseRoot["restartRequired"] = true;
  endConfigUpdate();
  return ConfigUpdateResult::OK;
}

void ConfigOnofre::json(JsonVariant &root, bool allFields)
{
  root["nodeId"] = nodeId;
  root["chipId"] = chipId;
  root["mqttIpDns"] = mqttIpDns;
  root["mqttPort"] = mqttPort;
  root["mqttUsername"] = mqttUsername;
  root["apiUser"] = apiUser;
  root["wifiSSID"] = wifiSSID;
  root["dhcp"] = dhcp;
  // DYNAMIC VALUES
  if (allFields)
  {
    // Recovery needs an exact hardware family. The historical `mcu` field is
    // also an OTA folder selector and reports C3 as ESP32, so do not overload it.
    root["backupVariant"] = currentMcuName();
    root["mqttConnected"] = mqttConnected();
    // Expose configuration state without returning CloudIO credentials to the
    // browser. The panel cannot inspect the deliberately omitted username.
    root["cloudConfigured"] = cloudIOUsername[0] != '\0';
    // Credentials present is not the same as a working link: a device can be
    // adopted and still be offline. The panel needs to tell those apart.
    root["cloudConnected"] = cloudIOConnected();
    // Shown so the panel can say why a schedule is not running: without a clock
    // the device deliberately refuses to water rather than guess the hour.
    root["clockSynced"] = clockSynced();
    root["clockNow"] = clockNowIso();
    // So the panel can follow an update it asked for instead of guessing.
    otaStatusJson(root);
    // Diagnostics for the local panel. Deliberately not sent to the cloud sync,
    // which has no use for them and is posted far more often.
    root["freeHeap"] = ESP.getFreeHeap();
#ifdef ESP8266
    root["heapFrag"] = ESP.getHeapFragmentation();
    root["maxFreeBlock"] = ESP.getMaxFreeBlockSize();
#endif
    root["uptime"] = millis() / 1000;
    root["resetReason"] = deviceResetReason();
    root["sketchSize"] = ESP.getSketchSize();
    root["freeSketchSpace"] = ESP.getFreeSketchSpace();
    // The board's usable GPIOs, so the panel draws its pin map from what the
    // firmware actually accepts instead of keeping a copy that drifts.
    JsonArray usable = root["usablePins"].to<JsonArray>();
    for (auto pin : DefaultPins::outputInputPins)
      usable.add(pin);
#ifdef ESP32
    JsonArray inputOnly = root["inputOnlyPins"].to<JsonArray>();
#ifndef ESP32C6
    for (auto pin : DefaultPins::intputOnlyPins)
      inputOnly.add(pin);
#endif
#endif
  }
  root["wifiIp"] = WiFi.localIP().toString();
  root["wifiMask"] = WiFi.subnetMask().toString();
  root["wifiGw"] = WiFi.gatewayIP().toString();
  root["firmware"] = String(VERSION);
  root["buildDate"] = kFirmwareBuildDate;
#ifdef ESP32
#ifdef ESP32C6
  // Must not report plain "ESP32": this string picks the OTA folder, and an 8 MB
  // ESP32 image does not belong on a C6.
  root["mcu"] = "ESP32-C6";
#else
  root["mcu"] = "ESP32";
#endif
#endif
#ifdef ESP8266
#ifdef HAN_MODE
  root["mcu"] = "ESP8266-HAN";
#else
  root["mcu"] = "ESP8266";
#endif
#endif
  root["mac"] = WiFi.macAddress();
  if (allFields)
    root["signal"] = WiFi.RSSI();
  JsonArray outInPins = root["outInPins"].to<JsonArray>();
#ifdef ESP32
  JsonVariant inPins = root["inPins"].to<JsonArray>();

#ifndef ESP32C6
  for (auto p : DefaultPins::intputOnlyPins)
  {
    inPins.add(p);
  }
#endif
#endif
  for (auto p : DefaultPins::outputInputPins)
  {
    outInPins.add(p);
  }

  // The schedule travels with the features it commands, so the panel gets both
  // in one read. Only on the full payload: the cloud sync does not schedule.
  if (allFields)
  {
    for (auto &a : actuatores)
    {
      if (a.isGardenValve())
      {
        irrigation.json(root);
        break;
      }
    }
  }

  JsonArray features = root["features"].to<JsonArray>();
  for (auto s : actuatores)
  {
    JsonObject a = features.add<JsonObject>();
    if (allFields)
      a["group"] = "ACTUATOR";
    a["id"] = s.uniqueId;
    a["name"] = s.name;
    a["typeControl"] = s.typeControl;
    if (s.typeControl == ActuatorControlType::GPIO_OUTPUT)
    {
      if (allFields)
        a["upCourseTime"] = s.upCourseTime;
      if (allFields)
        a["downCourseTime"] = s.downCourseTime;
      JsonArray outputs = a["outputs"].to<JsonArray>();
      for (auto out : s.outputs)
      {
        outputs.add(out);
      }
    }
    if (allFields)
      a["inputMode"] = s.driverToInputMode();
    a["family"] = s.familyToText();
    a["driver"] = s.driverToText();
    a["state"] = s.state;
    a["autoOff"] = s.autoOff;
    // An open valve is always on a clock: the program's minutes while a cycle is
    // watering it, its own autoOff otherwise (the loop enforces both). Reporting
    // where it is between the two ends is what lets a panel show the time
    // draining away rather than a number that only moves when it is re-read.
    unsigned long valveLeft = 0ul;
    unsigned long valveTotal = 0ul;
    if (s.valveClock(valveLeft, valveTotal))
    {
      a["secondsLeft"] = valveLeft;
      a["totalSeconds"] = valveTotal;
    }
    a["area"] = s.knxAddress[0];
    a["line"] = s.knxAddress[1];
    a["member"] = s.knxAddress[2];
    JsonArray inputs = a["inputs"].to<JsonArray>();
    for (auto in : s.inputs)
    {
      inputs.add(in);
    }
  }
  for (auto s : sensors)
  {
    JsonObject a = features.add<JsonObject>();
    if (allFields)
      a["group"] = "SENSOR";
    a["id"] = s.uniqueId;
    a["name"] = s.name;
    a["family"] = s.familyToText();
    if (s.lastBinaryState >= 0)
    {
      if (allFields)
        a["state"] = s.state;
    }
    a["driver"] = s.driverToText();
    JsonArray inputs = a["inputs"].to<JsonArray>();
    for (auto in : s.inputs)
    {
      inputs.add(in);
    }
  }
}

void ConfigOnofre::requestWifiScan()
{
#ifdef ESP32
  wifiScan.store(true, std::memory_order_release);
#else
  wifiScan = true;
#endif
}

bool ConfigOnofre::isWifiScanRequested()
{
#ifdef ESP32
  return wifiScan.exchange(false, std::memory_order_acq_rel);
#else
  if (wifiScan)
  {
    wifiScan = false;
    return true;
  }
  return false;
#endif
}

void ConfigOnofre::requestCloudIOSync()
{
#ifdef ESP32
  cloudIOSync.store(true, std::memory_order_release);
#else
  cloudIOSync = true;
#endif
}

bool ConfigOnofre::isCloudIOSyncRequested()
{
#ifdef ESP32
  return cloudIOSync.exchange(false, std::memory_order_acq_rel);
#else
  if (cloudIOSync)
  {
    cloudIOSync = false;
    return true;
  }
  return false;
#endif
}

void ConfigOnofre::requestReloadWifi()
{
#ifdef ESP32
  wifiReload.store(true, std::memory_order_release);
#else
  wifiReload = true;
#endif
}
bool ConfigOnofre::isReloadWifiRequested()
{
#ifdef ESP32
  return wifiReload.exchange(false, std::memory_order_acq_rel);
#else
  if (wifiReload)
  {
    wifiReload = false;
    return true;
  }
  return false;
#endif
}
void ConfigOnofre::loopActuators()
{
  if (!tryBeginFeatureLoopAccess())
    return;

  // A restart-pending actuator has already released or halted hardware. Keep
  // the complete actuator/irrigation loop inert until the controlled restart;
  // otherwise irrigation could command a half-reconfigured output map.
  for (const auto &sw : actuatores)
    if (!sw.ready)
    {
      endFeatureAccess();
      return;
    }

  for (auto &sw : actuatores)
  {
    // A valve opened by a program answers to the program's timer, not to its own
    // autoOff: the default 30 minutes would otherwise cut a longer zone short and
    // the cycle would read that as someone closing the valve by hand.
    if (sw.state == 100 && sw.autoOff > 0 && sw.lastChange > 0 && sw.lastChange + (sw.autoOff * 1000) < millis() &&
        !irrigation.isRunningZone(sw.uniqueId))
    {
      sw.changeState(StateOrigin::AUTO, 0);
    }
    if (sw.typeControl == ActuatorControlType::GPIO_OUTPUT && sw.isCover() &&
        sw.shutter != nullptr)
    {
      sw.shutter->loop();
    }
    for (auto &button : sw.buttons)
    {
      button.loop();
    }
    if (wifiConnected() && sw.isKnxSupport())
    {
      knx.loop();
      if (sw.knxSync == 3)
      {
#ifdef DEBUG_ONOFRE
        Log.notice("%s KNX Request Sync for: %s" CR, tags::actuatores, sw.name);
#endif
        knx.send_1byte_int(knx.GA_to_address(sw.knxAddress[0], sw.knxAddress[1], sw.knxAddress[2]), KNX_CT_READ, 0);
      }
      if (sw.knxSync < 4)
      {
        sw.knxSync++;
      }
    }
  }
  irrigation.loop();
  endFeatureAccess();
}
void ConfigOnofre::requestRestart()
{
#ifdef ESP32
  reboot.store(true, std::memory_order_release);
#else
  reboot = true;
#endif
}
void ConfigOnofre::loopSensors()
{
  if (!tryBeginFeatureLoopAccess())
    return;
  for (const auto &sensor : sensors)
    if (!sensor.ready)
    {
      endFeatureAccess();
      return;
    }
  for (auto &s : sensors)
  {
    s.loop();
  }
  endFeatureAccess();
}

bool ConfigOnofre::isRestartRequested()
{
#ifdef ESP32
  return reboot.exchange(false, std::memory_order_acq_rel);
#else
  if (reboot)
  {
    reboot = false;
    return true;
  }
  return false;
#endif
}

void ConfigOnofre::requestAutoUpdate()
{
#ifdef ESP32
  autoUpdate.store(true, std::memory_order_release);
#else
  autoUpdate = true;
#endif
}
// Asking must not consume: loop() calls this only to decide whether to skip its
// normal work, and it used to clear the flag doing so. When the MQTT command
// landed after checkInternalRoutines() had already run — loopWiFi() yields in
// between — that guard ate the request and no update ever happened.
bool ConfigOnofre::isAutoUpdateRequested()
{
#ifdef ESP32
  return autoUpdate.load(std::memory_order_acquire);
#else
  return autoUpdate;
#endif
}
// Taken once, by whoever actually performs the update.
bool ConfigOnofre::takeAutoUpdateRequest()
{
#ifdef ESP32
  return autoUpdate.exchange(false, std::memory_order_acq_rel);
#else
  if (autoUpdate)
  {
    autoUpdate = false;
    return true;
  }
  return false;
#endif
}

void ConfigOnofre::requestLoadDefaults()
{
#ifdef ESP32
  loadDefaults.store(true, std::memory_order_release);
#else
  loadDefaults = true;
#endif
}
bool ConfigOnofre::isLoadDefaultsRequested()
{
#ifdef ESP32
  return loadDefaults.exchange(false, std::memory_order_acq_rel);
#else
  if (loadDefaults)
  {
    loadDefaults = false;
    return true;
  }
  return false;
#endif
}
