AsyncEventSource events("/events");

JsonObject& configJson = getJsonObject();
typedef struct {
    int gpio;
} gpios_t;
std::vector<gpios_t> inUseGpios;
void logger(String payload){
  if(payload.equals(""))return;
  Serial.println(payload);
}
 

void resetToFactoryConfig(){
   SPIFFS.format();
   shouldReboot = true;
}

JsonObject& getConfigJson(){
 return configJson;
}

String getUpdateUrl(){
 return "http://release.bhonofre.pt/release_"+String(FACTORY_TYPE)+".bin";
 }
String getHostname(){
  String nodeId = configJson.get<String>("nodeId");
  if(nodeId.equals(DEFAULT_NODE_ID)){
    return DEFAULT_NODE_ID;
  }
  return nodeId;
}

String normalize(String inputStr){
  inputStr.trim();
  inputStr.replace(" ","_");
  return inputStr;
  }
String getApName(){
   String nodeId = configJson.get<String>("nodeId");
  if(nodeId.equals(DEFAULT_NODE_ID)){
    return DEFAULT_NODE_ID;
  }
  return  nodeId;
}
void applyUpdateConfig(double outdatedVersion){
  if(outdatedVersion < 1.4){
      JsonArray& _devices =  getStoredSensors();
      for(int i  = 0 ; i < _devices.size() ; i++){ 
        JsonObject& d = _devices[i]; 
        JsonArray& functions = d.get<JsonVariant>("functions");
        for(int i  = 0 ; i < functions.size() ; i++){
          JsonObject& f = functions[i];    
          String _name = f.get<String>("name");
          if(_name.equals("Temperatura")){
            f.set("uniqueName","temperature");
          }else if(_name.equals("Humidade")){
            f.set("uniqueName","humidity");
          }
          
        }
        rebuildSensorsMqttTopics();     
    }
  }
  if(outdatedVersion < 3.54){
    configJson.set("hardwareId", String(ESP.getChipId()));
  }
}

void loadStoredConfiguration(){
  bool configFail = true;
  if(SPIFFS.begin()){
    File cFile;   
    if(SPIFFS.exists(CONFIG_FILENAME)){
      cFile = SPIFFS.open(CONFIG_FILENAME,"r+"); 
      if(cFile){
        logger("[CONFIG] Read stored file config...");
        JsonObject& storedConfig = getJsonObject(cFile);
        if (storedConfig.success()) {
          configJson.set("firmware", FIRMWARE_VERSION);
          configJson.set("nodeId",storedConfig.get<String>("nodeId"));
          configJson.set("hostname",storedConfig.get<String>("hostname"));
          configJson.set("hardware",HARDWARE);
          configJson.set("hardwareId", String(ESP.getChipId()));
          configJson.set("type", String(FACTORY_TYPE));
          
          configJson.set("homeAssistantAutoDiscoveryPrefix",storedConfig.get<String>("homeAssistantAutoDiscoveryPrefix"));
          
          configJson.set("mqttIpDns",storedConfig.get<String>("mqttIpDns"));
          configJson.set("mqttUsername",storedConfig.get<String>("mqttUsername"));
          configJson.set("mqttPort",storedConfig.get<unsigned int>("mqttPort"));
          configJson.set("mqttPassword",storedConfig.get<String>("mqttPassword"));
          
          configJson.set("wifiSSID",storedConfig.get<String>("wifiSSID"));
          configJson.set("wifiSecret", storedConfig.get<String>("wifiSecret"));
          
          configJson.set("wifiSSID2",storedConfig.get<String>("wifiSSID2"));
          configJson.set("wifiSecret2", storedConfig.get<String>("wifiSecret2"));
          
          configJson.set("staticIp", storedConfig.get<bool>("staticIp"));
          configJson.set("wifiIp", storedConfig.get<String>("wifiIp"));
          configJson.set("wifiMask", storedConfig.get<String>("wifiMask"));
          configJson.set("wifiGw", storedConfig.get<String>("wifiGw"));
          
          configJson.set("apSecret", storedConfig.get<String>("apSecret"));
          
          configJson.set("configTime", storedConfig.get<long>("configTime"));
          configJson.set("configkey", storedConfig.get<String>("configkey"));
          
          logger("[CONFIG] Apply stored file config with success...");
          cFile.close();
          configFail = false;
        }
      }
    }
    
  if(configFail){
    logger("[CONFIG] Apply default config...");
    cFile = SPIFFS.open(CONFIG_FILENAME,"w+"); 
    configJson.set("nodeId",DEFAULT_NODE_ID);
    configJson.set("homeAssistantAutoDiscoveryPrefix", "homeassistant");
    configJson.set("hostname",getHostname());
    configJson.set("mqttPort",1883);
    configJson.set("type", String(FACTORY_TYPE));
    configJson.set("configTime",0L);
    configJson.set("apSecret", AP_SECRET);
    configJson.set("hardware", HARDWARE);
    configJson.set("configTime", 0L);
    configJson.set("firmware", FIRMWARE_VERSION);
    configJson.printTo(cFile);
  }
  SPIFFS.end(); 
  }else{
    logger("[CONFIG] File system error...");
   }
}


JsonObject& saveNode(JsonObject& nodeConfig){
  String  nodeId = nodeConfig.get<String>("nodeId");
 
  if(nodeId != nullptr && !configJson.get<String>("nodeId").equals(nodeId)){
    nodeId.replace(" ","");
    String oldNodeId = configJson.get<String>("nodeId");
    configJson.set("nodeId",nodeId);
  
    //reloadWiFiConfig();
    reloadMqttConfig();
    rebuildSwitchMqttTopics(configJson.get<String>("homeAssistantAutoDiscoveryPrefix"),oldNodeId);
    rebuildSensorsMqttTopics();
  }
  saveConfig();
  return configJson;
} 

JsonObject& saveWifi(JsonObject& _config){
  configJson.set("wifiSSID",_config.get<String>("wifiSSID"));
  configJson.set("wifiSecret", _config.get<String>("wifiSecret"));
  configJson.set("wifiSSID2",_config.get<String>("wifiSSID2"));
  configJson.set("wifiSecret2", _config.get<String>("wifiSecret2"));
  configJson.set("wifiIp", _config.get<String>("wifiIp"));
  configJson.set("wifiMask", _config.get<String>("wifiMask"));
  configJson.set("wifiGw", _config.get<String>("wifiGw"));
  configJson.set("staticIp", _config.get<bool>("staticIp"));
  wifiUpdated = true;
  return configJson;
}

JsonObject& adoptControllerConfig(JsonObject& _config, String configkey){
  configJson.set("wifiSSID",_config.get<String>("wifiSSID"));
  configJson.set("wifiSecret", _config.get<String>("wifiSecret"));
  
  configJson.set("wifiSSID2",_config.get<String>("wifiSSID2"));
  configJson.set("wifiSecret2", _config.get<String>("wifiSecret2"));
  
  configJson.set("mqttIpDns",_config.get<String>("mqttIpDns"));
  configJson.set("mqttUsername",_config.get<String>("mqttUsername"));
  configJson.set("mqttPassword",_config.get<String>("mqttPassword"));
  
  configJson.set("configTime",_config.get<long>("configTime"));
  configJson.set("configkey",configkey);
  
  configJson.set("homeAssistantAutoDiscoveryPrefix",_config.get<String>("homeAssistantAutoDiscoveryPrefix"));
  
   rebuildSwitchMqttTopics(configJson.get<String>("homeAssistantAutoDiscoveryPrefix"),configJson.get<String>("nodeId"));
  rebuildSensorsMqttTopics();
  reloadMqttConfig();
  saveConfig();
  return configJson;
}

void updateNetworkConfig(){
  if(!configJson.get<bool>("staticIp")){
     configJson.set("wifiIp",WiFi.localIP().toString());
     configJson.set("wifiMask",WiFi.subnetMask().toString());
     configJson.set("wifiGw", WiFi.gatewayIP().toString());
   }
  saveConfig();
}
JsonObject& saveMqtt(JsonObject& _config){
  configJson.set("mqttIpDns",_config.get<String>("mqttIpDns"));
  configJson.set("mqttUsername",_config.get<String>("mqttUsername"));
  configJson.set("mqttPassword",_config.get<String>("mqttPassword"));
  configJson.set("mqttEmbedded",_config.get<String>("mqttEmbedded"));
  saveConfig();
  reloadMqttConfig();
  return configJson;
} 

void saveConfig(){
   if(SPIFFS.begin()){
      File rFile = SPIFFS.open(CONFIG_FILENAME,"w+");
      if(!rFile){
        logger("[CONFIG] Open config file Error!");
      } else {
       
      configJson.printTo(rFile);
      }
      rFile.close();
   }else{
     logger("[CONFIG] Open file system Error!");
  }
  SPIFFS.end();
  logger("[CONFIG] New config stored.");
  
}

void releaseGpio(int gpio){
  
}

void configGpio(int gpio,int mode){
  pinMode(gpio,mode);
  inUseGpios.push_back({gpio});  
}
