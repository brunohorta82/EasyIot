
#define TIME_READINGS_DELAY 30000ul

//FUNTIONS
#define TEMPERATURE_FUNCTION 1
#define HUMIDITY_FUNCTION 2
#define MOTION_FUNCTION 4
#define OPENING 5
#define ANALOG_FUNCTION 7

//SENSORS
#define PIR_TYPE 65
#define LDR_TYPE 21
#define DS18B20_TYPE 90
#define REED_SWITCH_TYPE 56

int analogReadCount = 0;
float avg = 0;
JsonArray &sns = getJsonArray();

typedef struct
{
  unsigned int gpio;
  unsigned int type;
  String id;
  bool disabled;
  bool lastBinaryRead;
  JsonObject &sensorJson;
  DHT_nonblocking *dht;
  OneWire *oneWire;
  DallasTemperature *dallas;
} sensor_t;
std::vector<sensor_t> _sensors;

const String sensorsFilename = "sensors.json";

void removeSensor(String _id)
{
  int sensorFound = false;
  int index = 0;
  for (unsigned int i = 0; i < sns.size(); i++)
  {
    JsonObject &sensorJson = sns.get<JsonVariant>(i);
    if (sensorJson.get<String>("id").equals(_id))
    {
      sensorFound = true;
      index = i;
    }
  }
  if (sensorFound)
  {
    sns.remove(index);
  }

  persistSensorsFile(false);
}
JsonArray &getStoredSensors()
{
  return sns;
}

void loopSensors()
{
  float temperature = -127.0;
  float humidity = -127.0;
  float analogReadValue = 0;

  bool motion = false;
  static unsigned long measurement_timestamp = millis();
  static unsigned long measurement_timestampldr = millis();

  for (unsigned int i = 0; i < _sensors.size(); i++)
  {
    if (_sensors[i].disabled){
      continue;
    }
    
    unsigned int sensorType = _sensors[i].type;
    switch (sensorType)
    {
    case LDR_TYPE:
      if (millis() - measurement_timestampldr > 100)
      {
        analogReadCount++;
        analogReadValue = analogRead(A0);
        measurement_timestampldr = millis();
        avg += analogReadValue;
      }

      break;
    case PIR_TYPE:
      motion = digitalRead(_sensors[i].gpio);
      break;
    case REED_SWITCH_TYPE:

      break;
    case DS18B20_TYPE:
      if (millis() - measurement_timestamp > TIME_READINGS_DELAY)
      {
        _sensors[i].dallas->begin();
        _sensors[i].dallas->requestTemperatures();
        measurement_timestamp = millis();
        temperature = _sensors[i].dallas->getTempCByIndex(0);
        if (temperature == 85.0 || temperature == (-127.0))
        {
          continue;
        }
      }
      break;
    case DHT_TYPE_11:
    case DHT_TYPE_21:
    case DHT_TYPE_22:
      if (millis() - measurement_timestamp > TIME_READINGS_DELAY)
      {
        if (!_sensors[i].dht->measure(&temperature, &humidity))
        {
          continue;
        }
      }

      break;
    }

    JsonArray &functions = _sensors[i].sensorJson.get<JsonVariant>("functions");
    for (int i = 0; i < functions.size(); i++)
    {
      JsonObject &f = functions.get<JsonVariant>(i);
      String _mqttState = f.get<String>("mqttStateTopic");
      int _type = f.get<unsigned int>("type");
      bool _retain = f.get<bool>("mqttRetain");
      if (_type == TEMPERATURE_FUNCTION  && temperature != -127.0)
      {
        publishOnMqttQueue(_mqttState, String(temperature, 1), _retain);
        measurement_timestamp = millis();
      }
      else if (_type == HUMIDITY_FUNCTION  && humidity != -127.0)
      {
        publishOnMqttQueue(_mqttState, String(humidity, 1), _retain);
        measurement_timestamp = millis();
      }
      else if (_type == MOTION_FUNCTION  && motion != _sensors[i].lastBinaryRead)
      {
        _sensors[i].lastBinaryRead = motion;
        publishOnMqtt(_mqttState, motion ? PAYLOAD_ON : PAYLOAD_OFF, _retain);
      }
      else if (_type == ANALOG_FUNCTION )
      {
        if (analogReadCount >= 10)
        {
          publishOnMqttQueue(_mqttState, String(avg / analogReadCount, 1), false);

          analogReadCount = 0;
          avg = 0;
        }
      }
    }
  }
}
JsonObject &storeSensor(JsonObject &_sensor)
{
  bool sensorFound = false;
  String _id = _sensor.get<String>("id");
  for (unsigned int i = 0; i < sns.size(); i++)
  {
    JsonObject &sensorJson = sns.get<JsonVariant>(i);
    if (sensorJson.get<String>("id").equals(_id))
    {
      sensorFound = true;
      String _name = _sensor.get<String>("name");
      sensorJson.set("gpio", _sensor.get<unsigned int>("gpio"));
      sensorJson.set("name", _name);
      sensorJson.set("discoveryDisabled", _sensor.get<bool>("discoveryDisabled"));
      sensorJson.set("icon", _sensor.get<String>("icon"));
      sensorJson.set("disabled", _sensor.get<bool>("disabled"));
      if (sensorJson.get<unsigned int>("type") != _sensor.get<unsigned int>("type"))
      {
        sensorJson.remove("functions");
        JsonArray &functionsJson = getJsonArray();
        sensorJson.set("type", _sensor.get<unsigned int>("type"));
        //createFunctions(functionsJson, sensorJson.get<String>("id"), _sensor.get<unsigned int>("type"));
        sensorJson.set("functions", functionsJson);
      }
      JsonArray &functions = sensorJson.get<JsonVariant>("functions");
      JsonArray &functionsUpdated = _sensor.get<JsonVariant>("functions");
      for (int i = 0; i < functions.size(); i++)
      {
        JsonObject &f = functions.get<JsonVariant>(i);
        for (int b = 0; b < functionsUpdated.size(); b++)
        {
          JsonObject &fu = functionsUpdated.get<JsonVariant>(b);
          if (f.get<String>("uniqueName").equals(fu.get<String>("uniqueName")) && !fu.get<String>("name").equals(""))
          {
            f.set("name", fu.get<String>("name"));
            f.set("type", fu.get<unsigned int>("type"));
            f.set("unit", fu.get<String>("unit"));
            f.set("icon", fu.get<String>("icon"));
          }
        }
      }
    }
  }
  if (!sensorFound)
  {
    _sensor.set("id", normalize(_sensor.get<String>("name")));
     String sn = "";
     _sensor.printTo(sn);
     sns.add(getJsonObject(sn.c_str()));
  }
  persistSensorsFile(true);
  return _sensor;
}
void persistSensorsFile(boolean rebuild)
{
  if (rebuild)
  {
    rebuildAllMqttTopics(false, true);
  }
  if (SPIFFS.begin())
  {
    logger("[SENSORS] Open " + sensorsFilename);
    File rFile = SPIFFS.open(sensorsFilename, "w+");
    if (!rFile)
    {
      logger("[SENSORS] Open sensors file Error!");
    }
    else
    {
      sns.printTo(rFile);
    }
    rFile.close();
  }
  else
  {
    logger("[SENSORS] Open file system Error!");
  }
  SPIFFS.end();
  applyJsonSensors();
  logger("[SENSORS] New sensors config loaded.");
}
void loadStoredSensors()
{
  bool loadDefaults = false;
  if (SPIFFS.begin())
  {
    File cFile;

    if (SPIFFS.exists(sensorsFilename))
    {
      cFile = SPIFFS.open(sensorsFilename, "r+");
      if (!cFile)
      {
        logger("[SENSORS] Create file Sensor Error!");
        return;
      }
      logger("[SENSORS] Read stored file config...");
      JsonArray &storedSensors = getJsonArray(cFile);
      if (!storedSensors.success())
      {
        logger("[SENSORS] Json file parse Error!");
        loadDefaults = true;
      }
      else
      {

        logger("[SENSORS] Apply stored file config...");
        for (int i = 0; i < storedSensors.size(); i++)
        {
          sns.add(storedSensors.get<JsonVariant>(i));
        }
        applyJsonSensors();
      }
    }
    else
    {
      loadDefaults = true;
    }
    cFile.close();
    if (loadDefaults)
    {
      logger("[SENSORS] Apply default config...");
      cFile = SPIFFS.open(sensorsFilename, "w+");
      sns.printTo(cFile);
      applyJsonSensors();
      cFile.close();
    }
  }
  else
  {
    logger("[SWITCH] Open file system Error!");
  }
  SPIFFS.end();
}

void applyJsonSensors()
{
  _sensors.clear();
  for (int i = 0; i < sns.size(); i++)
  {
    JsonObject &sensorJson = sns.get<JsonVariant>(i);
    int gpio = sensorJson.get<unsigned int>("gpio");
    int type = sensorJson.get<unsigned int>("type");
    int disabled = sensorJson.get<bool>("disabled");
    String id = sensorJson.get<String>("id");
    switch (type)
    {
    case LDR_TYPE:
      _sensors.push_back({A0, type, id, disabled, false, sensorJson, NULL, NULL, NULL});
      break;
    case PIR_TYPE:
      _sensors.push_back({gpio, type, id, disabled, false, sensorJson, NULL, NULL, NULL});
      configGpio(gpio, INPUT);

      break;
    case REED_SWITCH_TYPE:
      if (gpio == 16)
      {
        configGpio(gpio, INPUT_PULLDOWN_16);
      }
      else
      {
        configGpio(gpio, INPUT_PULLUP);
      }
      break;
    case DHT_TYPE_11:
    case DHT_TYPE_21:
    case DHT_TYPE_22:
    {
      DHT_nonblocking *dht_sensor = new DHT_nonblocking(gpio, type);
      _sensors.push_back({gpio, type, id, disabled, false, sensorJson, dht_sensor, NULL, NULL});
    }
    break;
    case DS18B20_TYPE:
      OneWire *oneWire = new OneWire(gpio);
      DallasTemperature *sensors = new DallasTemperature(oneWire);
      _sensors.push_back({gpio, type, id, disabled, false, sensorJson, NULL, oneWire, sensors});
      break;
    }
  }
}
