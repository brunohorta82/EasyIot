#include <dht_nonblocking.h> // https://github.com/brunohorta82/DHT_nonblocking
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
#define TIME_READINGS_DELAY 3000ul

/* Uncomment according to your sensortype. */
#define DHT_SENSOR_TYPE DHT_TYPE_11
//#define DHT_SENSOR_TYPE DHT_TYPE_21
//#define DHT_SENSOR_TYPE DHT_TYPE_22

static const int DHT_SENSOR_PIN = 0;

const String sensorsFilename = "sensors.json";

JsonArray &sns = getJsonArray();

typedef struct
{
  JsonObject &sensorJson;
  DebounceEvent *binaryDebounce;

} sensor_bin;
std::vector<sensor_bin> _sensorsBinary;
int _sensorsBinarySize = 0; 

typedef struct
{
  JsonObject &sensorJson;
 DHT_nonblocking* dht;
  long delayRead;
  long lastRead;
  float temperature;
  float humidity ;
} sensor_dht;
std::vector<sensor_dht> _sensorsDHT;
int _sensorsDHTSize = 0;


typedef struct
{
  JsonObject &sensorJson;
  DallasTemperature *dallas;
  long delayRead;
  long lastRead;
} sensor_dallas;
std::vector<sensor_dallas> _sensorsDallas;
int _sensorsDallasSize = 0;

typedef struct
{
  JsonObject &sensorJson;
  int gpio;
  long delayRead;
  long lastRead;
  
} sensor_analog;
std::vector<sensor_analog> _sensorsAnalog;
int _sensorsAnalogSize = 0;


void callbackBinarySensor(uint8_t gpio, uint8_t event, uint8_t count, uint16_t length){
    Serial.print("Gpio : "); Serial.print(gpio);
    Serial.print("Event : "); Serial.print(event);
    Serial.print(" Count : "); Serial.print(count);
    Serial.print(" Length: "); Serial.print(length);
    Serial.println();
}
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

  persistSensorsFile();
}


JsonArray &getStoredSensors()
{
  return sns;
}

void loopSensors()
{
  static unsigned long measurement_timestamp = millis( );
/*  for (unsigned int i = 0; i <_sensorsBinarySize; i++)
  {
    DebounceEvent *b = _sensorsBinary[i].binaryDebounce;
    b->loop();
     Serial.println("llll");
  }

  for (unsigned int i = 0; i <_sensorsAnalogSize; i++){
     if (_sensorsAnalog[i].lastRead + _sensorsAnalog[i].delayRead < millis()){
        _sensorsAnalog[i].lastRead = millis();
        Serial.println(analogRead(_sensorsAnalog[i].gpio));
      }  
  }*/
  
for (unsigned int i = 0; i <_sensorsDHTSize; i++){
      if( millis( ) - measurement_timestamp > TIME_READINGS_DELAY ){
          if(!_sensorsDHT[i].dht->measure( &_sensorsDHT[i].temperature, &_sensorsDHT[i].humidity )){
      
      continue;
      }
       Serial.println(_sensorsDHT[i].temperature);
      }
      
}

 /* for (unsigned int i = 0; i < _sensorsDallasSize; i++){
     if (_sensorsDallas[i].lastRead + _sensorsDallas[i].delayRead < millis()){
         float temperature = -127.0;
       _sensorsDallas[i].dallas->begin();
        _sensorsDallas[i].dallas->requestTemperatures();
       _sensorsDallas[i].lastRead = millis();
        temperature =_sensorsDallas[i].dallas->getTempCByIndex(0);
      
        Serial.println(temperature);
        
      }  
  }*/
}

 


JsonObject &storeSensor(JsonObject &_sensor)
{  removeSensor(_sensor.get<String>("id"));
  _sensor.set("id", normalize(_sensor.get<String>("name")));
  rebuildSensorMqttTopics(_sensor);
  rebuildDiscoverySensorMqttTopics(_sensor);
  String sn = "";
  _sensor.printTo(sn);
  sns.add(getJsonObject(sn.c_str()));
  
  
  persistSensorsFile();
  return _sensor;
}


void persistSensorsFile()
{
  
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
  _sensorsAnalog.clear();
  _sensorsDHT.clear();
  _sensorsBinary.clear();
  _sensorsDallas.clear();
 
  for (int i = 0; i < sns.size(); i++)
  {
    JsonObject &sensorJson = sns.get<JsonVariant>(i);
    int gpio = sensorJson.get<unsigned int>("gpio");
    int type = sensorJson.get<unsigned int>("type");
    switch (type)
    {
    case LDR_TYPE:
      _sensorsAnalog.push_back({sensorJson,A0,2000L,0L});
      break;
    case PIR_TYPE:
      _sensorsBinary.push_back({ sensorJson, new DebounceEvent(gpio, callbackBinarySensor, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH)});
      break;
    case REED_SWITCH_TYPE:
    _sensorsBinary.push_back({ sensorJson,new  DebounceEvent(gpio, callbackBinarySensor, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH| BUTTON_SET_PULLUP )});
      break;
      
    case DHT_TYPE_11:
    case DHT_TYPE_21:
    case DHT_TYPE_22:
     {
        DHT_nonblocking* dht_sensor = new DHT_nonblocking( gpio,type );
         _sensorsDHT.push_back({sensorJson,dht_sensor,4000ul,0L,0L,0L});
      }
    break;
    case DS18B20_TYPE:
     _sensorsDallas.push_back({ sensorJson, new DallasTemperature(new OneWire(gpio)),5000L,0L});
      break;
    
    }
  }
  _sensorsAnalogSize =_sensorsAnalog.size();
  _sensorsDHTSize =_sensorsDHT.size();
  _sensorsBinarySize =_sensorsBinary.size();
  _sensorsDallasSize = _sensorsDallas.size();
}
