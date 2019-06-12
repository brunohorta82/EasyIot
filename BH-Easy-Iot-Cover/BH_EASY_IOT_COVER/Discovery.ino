
fauxmoESP fauxmo;

void startAlexaDiscovery(){
   fauxmo.createServer(false);
    fauxmo.setPort(80); // required for gen3 devices
    fauxmo.enable(true);
    fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
        stateSwitchByName(String(device_name), state ? "ON" : "OFF");
    });
}

void reloadAlexaDiscoveryServices(){
  JsonArray& _devices = getStoredSwitchs();
      for(int i  = 0 ; i < _devices.size() ; i++){ 
        JsonObject& switchJson = _devices[i];
        String _name = switchJson.get<String>("name");
        fauxmo.removeDevice(_name.c_str());
        fauxmo.addDevice(_name.c_str());
      }
 }
void reloadMqttDiscoveryServices(){
     String ipMqtt = getConfigJson().get<String>("mqttIpDns");
      if( ipMqtt == "")return;
      String prefix =  getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix");
      JsonArray& _devices = getStoredSwitchs();
      for(int i  = 0 ; i < _devices.size() ; i++){ 
        JsonObject& switchJson = _devices[i];
        String _id = switchJson.get<String>("id");
        String _name = switchJson.get<String>("name");
        String type = switchJson.get<String>("type");
        if(type.equals("cover")){
          publishOnMqttQueue(prefix+"/cover/"+String(ESP.getChipId())+"/"+_id+"/config",createHaCover(switchJson),true); 
          publishOnMqttQueue(prefix+"/cover/"+String(ESP.getChipId())+"/"+_id+"/config","", false); 
          subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic")); 
        }else if(type.equals("light")){
          publishOnMqttQueue(prefix+"/light/"+String(ESP.getChipId())+"/"+_id+"/config",createHaLight(switchJson),true); 
          publishOnMqttQueue(prefix+"/light/"+String(ESP.getChipId())+"/"+_id+"/config","", false); 
          subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic")); 
        }else if(type.equals("switch")){
          publishOnMqttQueue(prefix+"/switch/"+String(ESP.getChipId())+"/"+_id+"/config",createHaSwitch(switchJson),true); 
          publishOnMqttQueue(prefix+"/switch/"+String(ESP.getChipId())+"/"+_id+"/config","", false); 
          subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic")); 
        }
      }
      
      JsonArray& _sensores = getStoredSensors();
      for(int i  = 0 ; i < _sensores.size() ; i++){ 
        JsonObject& sensorJson = _sensores.get<JsonVariant>(i);  
        JsonArray& functions = sensorJson.get<JsonVariant>("functions");
        for(int i  = 0 ; i < functions.size() ; i++){
          JsonObject& f = functions.get<JsonVariant>(i);
          String uniqueName = f.get<String>("uniqueName");
        }     
    }
}

void loopDiscovery(){
   fauxmo.handle();
}

String createHaSwitch(JsonObject& _switchJson){
   String object = "";
   JsonObject& switchJson = getJsonObject();
   switchJson.set("name", _switchJson.get<String>("name"));
   switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
   switchJson.set("state_topic",  _switchJson.get<String>("mqttStateTopic"));
   switchJson.set("retain",  _switchJson.get<bool>("retain"));
   switchJson.set("payload_on", String(PAYLOAD_ON));
   switchJson.set("payload_off",  String(PAYLOAD_OFF));
   switchJson.printTo(object);
   return object;
}
String createHaLight(JsonObject& _switchJson){
   String object = "";
   JsonObject& switchJson = getJsonObject();
   switchJson.set("name", _switchJson.get<String>("name"));
   switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
   switchJson.set("state_topic",  _switchJson.get<String>("mqttStateTopic"));
   switchJson.set("retain",  _switchJson.get<bool>("retain"));
   switchJson.set("payload_on", String(PAYLOAD_ON));
   switchJson.set("payload_off",  String(PAYLOAD_OFF));
   switchJson.printTo(object);
   return object;
}
String createHaCover(JsonObject& _switchJson){
  String object = "";
   JsonObject& switchJson = getJsonObject();
   switchJson.set("name", _switchJson.get<String>("name"));
   switchJson.set("command_topic", _switchJson.get<String>("mqttCommandTopic"));
   switchJson.set("state_topic",  _switchJson.get<String>("mqttStateTopic"));
   switchJson.set("retain",  _switchJson.get<bool>("retain"));
   switchJson.set("payload_open", String(PAYLOAD_OPEN));
   switchJson.set("payload_close",  String(PAYLOAD_CLOSE));
   switchJson.set("payload_stop",  String(PAYLOAD_STOP));
   switchJson.printTo(object);
   return object;
}

void removeFromAlexaDiscovery(String _name){
  fauxmo.removeDevice(_name.c_str());
}

void removeFromHaDiscovery( String type, String _id){
     publishOnMqttQueue(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix")+"/"+type+"/"+String(ESP.getChipId())+"/"+_id+"/config","", false); 
}
