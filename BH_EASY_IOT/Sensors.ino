

#define TIME_READINGS_DELAY 30000ul
#define SENSOR_DEVICE  "sensor"
#define BINARY_SENSOR_DEVICE  "binary_sensor"
#define SENSOR_PIN 14
#define TEMPERATURE_TYPE 1
#define MOTION_TYPE 4
#define HUMIDITY_TYPE 2
#define DS18B20_TYPE 90
#define REED_SWITCH_TYPE 56
#define PIR_TYPE 65
#define LDR_TYPE 21
#define ANALOG_TYPE 7

int analogReadCount = 0;
 float avg = 0;
JsonArray& sns = getJsonArray();

typedef struct {
    unsigned int gpio;
    unsigned int type;
    String id;
    bool disabled;
    bool lastBinaryRead;
    JsonObject& sensorJson;
    DHT_nonblocking* dht;
    OneWire* oneWire;
    DallasTemperature* dallas;  
} sensor_t;
std::vector<sensor_t> _sensors;

const String sensorsFilename = "sensors.json";
void removeSensor(String _id){
  int sensorFound = false;
  int index = 0;
  for (unsigned int i=0; i < sns.size(); i++) {
    JsonObject& sensorJson = sns.get<JsonVariant>(i);   
    if(sensorJson.get<String>("id").equals(_id)){
      sensorFound = true;
      index  = i;
    }
  }
  if(sensorFound){
    sns.remove(index);
     
    }

  saveSensors();
  applyJsonSensors();
}
JsonArray& getStoredSensors(){
  return sns;
}

void sensorJson(String _id,int _gpio ,bool _disabled,  String _name,  int _type,JsonArray& functions){
      JsonObject& sensorJson = getJsonObject();
      sensorJson.set("id", _id);
      sensorJson.set("gpio", _gpio);
      sensorJson.set("name", _name);
      sensorJson.set("disabled", _disabled);
      sensorJson.set("type", _type);
      sensorJson.set("class", _type == PIR_TYPE ? BINARY_SENSOR_DEVICE : SENSOR_DEVICE);
      sensorJson.set("functions", functions);
      sns.add(sensorJson);
   
}

void createFunctions( JsonArray& functionsJson,String id,int type){

  switch(type){
    case LDR_TYPE:
    createFunctionArray(functionsJson,"Sensor de Luz","ligth_sensor","",MQTT_STATE_TOPIC_BUILDER(id,SENSOR_DEVICE,"ligth_sensor"),false,ANALOG_TYPE);
    break;
    case PIR_TYPE:
    createFunctionArray(functionsJson,"Movimento","motion","C",MQTT_STATE_TOPIC_BUILDER(id,SENSOR_DEVICE,"motion"),false,MOTION_TYPE);
    break;
    case DS18B20_TYPE:
    createFunctionArray(functionsJson,"Temperatura","temperature","ºC",MQTT_STATE_TOPIC_BUILDER(id,SENSOR_DEVICE,"temperature"),false,TEMPERATURE_TYPE);
    break;
    case DHT_TYPE_11:
    case DHT_TYPE_21:
    case DHT_TYPE_22:
      createFunctionArray(functionsJson,"Temperatura","temperature","ºC",MQTT_STATE_TOPIC_BUILDER(id,SENSOR_DEVICE,"temperature"),false,TEMPERATURE_TYPE);
      createFunctionArray(functionsJson,"Humidade","humidity","%", MQTT_STATE_TOPIC_BUILDER(id,SENSOR_DEVICE,"humidity"),false, HUMIDITY_TYPE);
    break;
    }
 
  }
void createFunctionArray(JsonArray& functionsJson,String _name, String _uniqueName, String _unit, String _mqttStateTopic, bool _retain, int _type ){
    JsonObject& functionJson = functionsJson.createNestedObject();
      functionJson.set("name", _name);
      functionJson.set("uniqueName", _uniqueName);
      functionJson.set("unit", _unit);
      functionJson.set("type", _type);
      functionJson.set("mqttStateTopic", _mqttStateTopic);
      functionJson.set("mqttRetain", _retain);
}
void loopSensors(){
    float temperature = -127.0; 
    float humidity = -127.0;
    float analogReadValue = 0;
 
    bool motion = false;
   static unsigned long measurement_timestamp = millis( );
   
    for (unsigned int i=0; i < _sensors.size(); i++) {
      if(_sensors[i].disabled){
        continue;
        }
   unsigned int sensorType = _sensors[i].type;
    switch( sensorType){
      case LDR_TYPE:
      if( millis( ) - measurement_timestamp >100 ){
        analogReadCount++;
        analogReadValue= analogRead(A0);
        measurement_timestamp = millis( );
        avg += analogReadValue;
      }else{
         continue;
        }
    
        break;
      case PIR_TYPE:
         motion = digitalRead(_sensors[i].gpio);
      break;
      case  REED_SWITCH_TYPE:
        
      break;
      case DS18B20_TYPE:
      if( millis( ) - measurement_timestamp > TIME_READINGS_DELAY ){
        _sensors[i].dallas->begin();
        _sensors[i].dallas->requestTemperatures();
        measurement_timestamp = millis( );
        temperature =   _sensors[i].dallas->getTempCByIndex(0);
        if( temperature == 85.0 ||  temperature == (-127.0)){
          continue;
          }
      }
      break;
      case DHT_TYPE_11:
      case DHT_TYPE_21:
      case DHT_TYPE_22:
      if( millis( ) - measurement_timestamp > TIME_READINGS_DELAY ){
     if(!_sensors[i].dht->measure( &temperature, &humidity )){
      continue;
      }
       
      }
      break;
      }
     
       
        JsonArray& functions = _sensors[i].sensorJson.get<JsonVariant>("functions");
        for(int i  = 0 ; i < functions.size() ; i++){
          JsonObject& f = functions.get<JsonVariant>(i);    
          String _mqttState = f.get<String>("mqttStateTopic");
          int _type =f.get<unsigned int>("type");
          bool _retain =f.get<bool>("mqttRetain");   
          if(_type == TEMPERATURE_TYPE && temperature != -127.0){
            publishOnMqttQueue(_mqttState ,String(  temperature,1),_retain);
             measurement_timestamp = millis( );
          }else if(_type == HUMIDITY_TYPE && humidity != -127.0){
            publishOnMqttQueue(_mqttState ,String( humidity,1),_retain);
             measurement_timestamp = millis( );
          }else if(_type == MOTION_TYPE && motion != _sensors[i].lastBinaryRead){ 
            _sensors[i].lastBinaryRead = motion;
            publishOnMqtt(_mqttState ,motion ? PAYLOAD_ON : PAYLOAD_OFF,_retain);
          }else if(_type == ANALOG_TYPE){
            if(analogReadCount >= 10){
              publishOnMqttQueue(_mqttState ,String(avg/analogReadCount ,1),false);
              analogReadCount = 0;
              avg = 0;
            }
             
           }
         }
    }
    
}
JsonArray& saveSensor(String _id,JsonObject& _sensor){
    bool sensorFound = false;
  for (unsigned int i=0; i < sns.size(); i++) {
    JsonObject& sensorJson = sns.get<JsonVariant>(i);  
    if(sensorJson.get<String>("id").equals(_id)){
      sensorFound = true;
      String _name = _sensor.get<String>("name");
      sensorJson.set("gpio",_sensor.get<unsigned int>("gpio"));
      sensorJson.set("name",_name);
      sensorJson.set("discoveryDisabled",_sensor.get<bool>("discoveryDisabled"));
      sensorJson.set("icon",_sensor.get<String>("icon"));
      sensorJson.set("disabled",_sensor.get<bool>("disabled"));
      removeComponentHaConfig(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix"),getConfigJson().get<String>("nodeId"),sensorJson.get<String>("type"),sensorJson.get<String>("class"), sensorJson.get<String>("id"));
      if( sensorJson.get<unsigned int>("type") != _sensor.get<unsigned int>("type")){
          sensorJson.remove("functions");
           JsonArray& functionsJson = getJsonArray();
          sensorJson.set("type",_sensor.get<unsigned int>("type"));
          createFunctions(functionsJson,sensorJson.get<String>("id"),_sensor.get<unsigned int>("type"));
          sensorJson.set("functions",functionsJson);  
        }
        JsonArray& functions = sensorJson.get<JsonVariant>("functions");
        JsonArray& functionsUpdated = _sensor.get<JsonVariant>("functions");
        for(int i  = 0 ; i < functions.size() ; i++){
           JsonObject& f = functions.get<JsonVariant>(i);
           for(int b  = 0 ; b < functionsUpdated.size() ; b++){
            JsonObject& fu = functionsUpdated.get<JsonVariant>(b);
            if(f.get<String>("uniqueName").equals(fu.get<String>("uniqueName")) && !fu.get<String>("name").equals("")){
              f.set("name",fu.get<String>("name"));
              f.set("type",fu.get<unsigned int>("type"));
               f.set("unit",fu.get<String>("unit"));
                f.set("icon",fu.get<String>("icon"));
            }

          }
        }
    }
    
  }
    if(!sensorFound){
          String id = "S"+String(millis());
          JsonArray& functionsJson = getJsonArray();
          JsonArray& functionsNew = _sensor.get<JsonVariant>("functions");
          for(int i  = 0 ; i < functionsNew .size() ; i++){
            JsonObject& f = functionsNew.get<JsonVariant>(i);
            createFunctionArray(functionsJson,f.get<String>("name"), f.get<String>("uniqueName"), f.get<String>("unit"), MQTT_STATE_TOPIC_BUILDER(id,SENSOR_DEVICE,f.get<String>("uniqueName")), f.get<bool>("retain"), f.get<unsigned int>("type") );
          }
          
          sensorJson(id,_sensor.get<unsigned int>("gpio"),_sensor.get<bool>("disabled"),_sensor.get<String>("name"),  _sensor.get<unsigned int>("type"),functionsJson);
    }
  saveSensors();
  applyJsonSensors();
    createHASensorComponent();  
   return sns;
}
void saveSensors(){
   if(SPIFFS.begin()){
      logger("[SENSORS] Open "+sensorsFilename);
      File rFile = SPIFFS.open(sensorsFilename,"w+");
      if(!rFile){
        logger("[SENSORS] Open sensors file Error!");
      } else {
      sns.printTo(rFile);
      }
      rFile.close();
   }else{
     logger("[SENSORS] Open file system Error!");
  }
  SPIFFS.end();
  logger("[SENSORS] New sensors config loaded.");
}
void loadStoredSensors(){
  bool loadDefaults = false;
  if(SPIFFS.begin()){
    File cFile;   

    if(SPIFFS.exists(sensorsFilename)){
      cFile = SPIFFS.open(sensorsFilename,"r+"); 
      if(!cFile){
        logger("[SENSORS] Create file Sensor Error!");
        return;
      }
        logger("[SENSORS] Read stored file config...");
        JsonArray &storedSensors = getJsonArray(cFile);
        if (!storedSensors.success()) {
          logger("[SENSORS] Json file parse Error!");
          loadDefaults = true;
        }else{
         
          logger("[SENSORS] Apply stored file config...");
          for(int i = 0 ; i< storedSensors.size(); i++){
            sns.add(storedSensors.get<JsonVariant>(i));
            }
          applyJsonSensors();
        }
        
     }else{
        loadDefaults = true;
     }
    cFile.close();
     if(loadDefaults){
      logger("[SENSORS] Apply default config...");
      cFile = SPIFFS.open(sensorsFilename,"w+"); 
      sns.printTo(cFile);
      applyJsonSensors();
      cFile.close();
      }
     
  }else{
     logger("[SWITCH] Open file system Error!");
  }
   SPIFFS.end(); 
   
}

void applyJsonSensors(){
  _sensors.clear();
  for(int i  = 0 ; i < sns.size() ; i++){ 
    JsonObject& sensorJson = sns.get<JsonVariant>(i);  
    int gpio= sensorJson.get<unsigned int>("gpio");
    int type= sensorJson.get<unsigned int>("type");
    int disabled= sensorJson.get<bool>("disabled");
    String id= sensorJson.get<String>("id");
    switch(type){
      case LDR_TYPE:
       _sensors.push_back({A0,type,id,disabled,false,sensorJson,NULL,NULL,NULL});
      break;
      case PIR_TYPE:
         _sensors.push_back({gpio,type,id,disabled,false,sensorJson,NULL,NULL,NULL});
         configGpio(gpio,INPUT);
       
      break;
      case  REED_SWITCH_TYPE:
        if ( gpio == 16) {
             configGpio(gpio, INPUT_PULLDOWN_16);
          } else {
             configGpio(gpio,INPUT_PULLUP );
          }
      break;
      case DHT_TYPE_11:
      case DHT_TYPE_21:
      case DHT_TYPE_22:
      {
        DHT_nonblocking* dht_sensor = new DHT_nonblocking( gpio,type );
        _sensors.push_back({gpio,type,id,disabled,false,sensorJson, dht_sensor,NULL,NULL});
      }
      break;
      case DS18B20_TYPE:   
      OneWire* oneWire = new OneWire (SENSOR_PIN);
      DallasTemperature* sensors = new DallasTemperature(oneWire);
       _sensors.push_back({gpio,type,id,disabled,false,sensorJson, NULL ,oneWire,sensors});
     break;
     }
  }
}

void rebuildSensorsMqttTopics(){
      bool store = false;
      for(int i  = 0 ; i < sns.size() ; i++){ 
        store = true;
        JsonObject& sensorJson = sns.get<JsonVariant>(i);  
        JsonArray& functions = sensorJson.get<JsonVariant>("functions");
        for(int i  = 0 ; i < functions.size() ; i++){
          JsonObject& f = functions.get<JsonVariant>(i);
          String _mqttState =f.get<String>("mqttStateTopic");
          String id = sensorJson.get<String>("id");
          String uniqueName = f.get<String>("uniqueName");
          f.set("mqttStateTopic",MQTT_STATE_TOPIC_BUILDER(id,SENSOR_DEVICE,uniqueName));
        }     
    }
    if(store){
      saveSensors();
      
        createHASensorComponent();  
      
    }
  }
