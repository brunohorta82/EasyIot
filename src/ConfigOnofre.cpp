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
void actuatoresCallback(message_t const &msg, void *arg);
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
          save();
        }
      }
      else if (address == Discovery::I2C_SSD1306_ADDRESS)
      {
        display = new Adafruit_SSD1306(128, 64, &Wire, -1);
        if (display->begin(SSD1306_SWITCHCAPVCC, Discovery::I2C_SSD1306_ADDRESS))
        {
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
      }
      else if (address == Discovery::I2C_LTR303_ADDRESS)
      {
        if (!isSensorExists(address))
        {
          prepareLTR303(address);
          save();
        }
      }
      else if (address == Discovery::I2C_TMF880X_ADDRESS)
      {
        prepareTMF882X(address);
      }
    }
  }
#ifdef DEBUG_ONOFRE
  Log.notice("%s Smart Bus Done." CR, tags::config);
#endif
}
ConfigOnofre &ConfigOnofre::pauseFeatures()
{
  pauseFeaturesLoop = true;
  return *this;
}
ConfigOnofre &ConfigOnofre::resumeFeatures()
{
  pauseFeaturesLoop = false;
  return *this;
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
  templateId = doc["templateId"];
  strlcpy(chipId, String(chipIdHex).c_str(), sizeof(chipId));
  sprintf(provisionId, "ONOFRE%s", chipId);

#endif
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
#ifdef DEBUG_ONOFRE
  Log.notice("%s Stored config loaded." CR, tags::config);

#endif
  return *this;
}
void ConfigOnofre::loadTemplate(int templateId)
{
  templateSelect((Template)templateId);
  this->templateId = templateId;
}
ConfigOnofre &ConfigOnofre::save()
{
  File file = LittleFS.open(configFilenames::config, "w+");
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
    JsonDocument a;
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
    a["member"] = s.knxAddress[2];
    a["autoOff"] = s.autoOff;
    a["state"] = s.state;
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
    features.add(a);
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

  if (serializeJson(doc, file) == 0)
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Fail to write File." CR, tags::config);
#endif
  }
  file.close();
#ifdef DEBUG_ONOFRE
  Log.notice("%s ConfigOnofre stored." CR, tags::config);
#endif
  doc.clear();
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

ConfigOnofre &ConfigOnofre::update(JsonObject &root)
{
  bool restore = root["backup"] | false;
  if (restore)
  {
    actuatores.clear();
    sensors.clear();
  }
  dhcp = root["dhcp"] | true;
  String mqttPasswordStr = root["mqttPassword"] | "";
  if (root.containsKey("mqttPassword") && mqttPasswordStr.compareTo(constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(mqttPassword, mqttPasswordStr.c_str(), sizeof(mqttPassword));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Mqtt Password changed: %s" CR, tags::config, mqttPassword);
#endif
  }

  String wifiSecretStr = root["wifiSecret"] | "";
  if (root.containsKey("wifiSecret") && wifiSecretStr.compareTo(constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(wifiSecret, wifiSecretStr.c_str(), sizeof(wifiSecret));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Wifi Password changed: %s" CR, tags::config, wifiSecret);
#endif
  }

  String accessPointPasswordStr = root["accessPointPassword"] | constantsConfig::apSecret;
  if (root.containsKey("accessPointPassword") && accessPointPasswordStr.compareTo(constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(accessPointPassword, accessPointPasswordStr.c_str(), sizeof(accessPointPassword));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Access Point Password changed: %s" CR, tags::config, accessPointPassword);
#endif
  }

  String apiPasswordStr = root["apiPassword"] | constantsConfig::apiPassword;
  if (root.containsKey("apiPassword") && apiPasswordStr.compareTo(constantsConfig::PW_HIDE) != 0)
  {
    strlcpy(apiPassword, apiPasswordStr.c_str(), sizeof(apiPassword));
#ifdef DEBUG_ONOFRE
    Log.notice("%s Api Password changed: %s" CR, tags::config, apiPassword);
#endif
  }

  String n_name = root["nodeId"] | chipId;
  normalize(n_name);
  strlcpy(nodeId, n_name.c_str(), sizeof(nodeId));
  strlcpy(mqttIpDns, root["mqttIpDns"] | "", sizeof(mqttIpDns));
  mqttPort = root["mqttPort"] | constantsMqtt::defaultPort;
  strlcpy(mqttUsername, root["mqttUsername"] | "", sizeof(mqttUsername));
  strlcpy(wifiSSID, root["wifiSSID"] | "", sizeof(wifiSSID));
  strlcpy(wifiIp, root["wifiIp"] | "", sizeof(wifiIp));
  strlcpy(wifiMask, root["wifiMask"] | "", sizeof(wifiMask));
  strlcpy(wifiGw, root["wifiGw"] | "", sizeof(wifiGw));
  strlcpy(apiUser, root["apiUser"] | constantsConfig::apiUser, sizeof(apiUser));
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
        for (auto &actuator : actuatores)
        {
          if (strcmp(actuator.uniqueId, id.c_str()) == 0)
          {
            if (strlen(feature["name"] | I18N::NO_NAME) > 0)
              strlcpy(actuator.name, feature["name"] | I18N::NO_NAME, sizeof(actuator.name));
            actuator.driver = actuator.findDriver(feature["inputMode"] | ActuatorInputMode::PUSH);
            actuator.upCourseTime = feature["upCourseTime"] | constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;
            actuator.downCourseTime = feature["downCourseTime"] | constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;
            actuator.knxAddress[0] = feature["area"] | 0;
            actuator.knxAddress[1] = feature["line"] | 0;
            actuator.knxAddress[2] = feature["member"] | 0;
            actuator.autoOff = feature["autoOff"] | 0ul;
            actuator.setup();
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
        for (auto &sensor : sensors)
        {
          if (strcmp(sensor.uniqueId, id.c_str()) == 0)
          {
            if (strlen(feature["name"] | I18N::NO_NAME) > 0)
              strlcpy(sensor.name, feature["name"] | I18N::NO_NAME, sizeof(sensor.name));
          }
        }
      }
    }
  }
  setupMQTT(false);
  root.clear();
  return this->save();
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
    root["mqttConnected"] = mqttConnected();
  root["wifiIp"] = WiFi.localIP().toString();
  root["wifiMask"] = WiFi.subnetMask().toString();
  root["wifiGw"] = WiFi.gatewayIP().toString();
  root["firmware"] = String(VERSION);
#ifdef ESP32
#ifdef ESP32_MAKER_4MB
  root["mcu"] = "ESP32-MAKER-4MB";
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

  for (auto p : DefaultPins::intputOnlyPins)
  {
    inPins.add(p);
  }
#endif
  for (auto p : DefaultPins::outputInputPins)
  {
    outInPins.add(p);
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
  wifiScan = true;
}

bool ConfigOnofre::isWifiScanRequested()
{
  if (wifiScan)
  {
    wifiScan = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestCloudIOSync()
{
  cloudIOSync = true;
}

bool ConfigOnofre::isCloudIOSyncRequested()
{
  if (cloudIOSync)
  {
    cloudIOSync = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestReloadWifi()
{
  wifiReload = true;
}
bool ConfigOnofre::isReloadWifiRequested()
{
  if (wifiReload)
  {
    wifiReload = false;
    return true;
  }
  return false;
}
void ConfigOnofre::loopActuators()
{
  for (auto &sw : actuatores)
  {
    if (sw.state == 100 && sw.autoOff > 0 && sw.lastChange > 0 && sw.lastChange + (sw.autoOff * 1000) < millis())
    {
      sw.changeState(StateOrigin::AUTO, 0);
    }
    if (sw.typeControl == ActuatorControlType::GPIO_OUTPUT && sw.isCover())
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
}
void ConfigOnofre::requestRestart()
{
  reboot = true;
}
void ConfigOnofre::loopSensors()
{
  for (auto &s : sensors)
  {
    s.loop();
  }
}

bool ConfigOnofre::isRestartRequested()
{
  if (reboot)
  {
    reboot = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestAutoUpdate()
{
  autoUpdate = true;
}
bool ConfigOnofre::isAutoUpdateRequested()
{
  if (autoUpdate)
  {
    autoUpdate = false;
    return true;
  }
  return false;
}

void ConfigOnofre::requestLoadDefaults()
{
  loadDefaults = true;
}
bool ConfigOnofre::isLoadDefaultsRequested()
{
  if (loadDefaults)
  {
    loadDefaults = false;
    return true;
  }
  return false;
}
