

void createHASwitchsComponent(){
  JsonArray& _devices = getStoredSwitchs();
  for(int i  = 0 ; i < _devices.size() ; i++){ 
    JsonObject& switchJson = _devices[i];    
    if(switchJson.get<bool>("discoveryDisabled")){
      continue;
     }  
    String _id = switchJson.get<String>("id");
    String  _type = switchJson.get<String>("type");
    String _class =switchJson.get<String>("class");
    String _name =switchJson.get<String>("name");
    String _mqttCommand =switchJson.get<String>("mqttCommandTopic");
    String _mqttState =switchJson.get<String>("mqttStateTopic");
    bool _retain =switchJson.get<bool>("mqttRetain");
    String state = switchJson.get<bool>("stateControl") ? PAYLOAD_ON : PAYLOAD_OFF;
    publishOnMqttQueue((getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix")+"/"+_type+"/"+getConfigJson().get<String>("nodeId")+"/"+_id+"/config"),("{\"name\": \""+_name+"\", \""+(_type.equals("cover") ? "position_topic" : "state_topic")+"\": \""+_mqttState+"\",\"availability_topic\": \""+getAvailableTopic()+"\", \"command_topic\": \""+_mqttCommand+"\", \"retain\": false,\"state\":\""+state+"\",\"payload_available\":\"1\",\"payload_not_available\":\"0\"}"),true);
    if(switchJson.get<unsigned int>("mode") == 8){
      publishOnMqttQueue((getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix")+"/sensor/"+getConfigJson().get<String>("nodeId")+"/"+_id+"/config"),("{\"name\": \""+_name+"\", \"state_topic\": \""+_mqttState+"\",\"availability_topic\": \""+getAvailableTopic()+"\",\"payload_available\":\"1\",\"payload_not_available\":\"0\"}"),true);
    }
    publishOnMqttQueue((getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix")+"/"+_type+"/"+getConfigJson().get<String>("nodeId")+"/"+_id+"/config"),("{\"name\": \""+_name+"\", \""+(_type.equals("cover") ? "position_topic" : "state_topic")+"\": \""+_mqttState+"\",\"availability_topic\": \""+getAvailableTopic()+"\", \"command_topic\": \""+_mqttCommand+"\", \"retain\": false,\"state\":\""+state+"\",\"payload_available\":\"1\",\"payload_not_available\":\"0\"}"),true);
    subscribeOnMqtt(_mqttCommand.c_str());
    if(_type.equals("cover")){
       publishOnMqttQueue(switchJson.get<String>("mqttStateTopic").c_str(),String(switchJson.get<unsigned int>("positionControlCover")),true);
    }else{
      publishOnMqttQueue(switchJson.get<String>("mqttStateTopic").c_str(),switchJson.get<bool>("stateControl") ? PAYLOAD_ON : PAYLOAD_OFF,true);
      }
   }
}

void createHASensorComponent(){
  JsonArray& sensorsJson = getStoredSensors();
  for(int i  = 0 ; i < sensorsJson.size() ; i++){ 
    JsonObject& sensorJson = sensorsJson.get<JsonVariant>(i);;   
   if(sensorJson.get<bool>("discoveryDisabled")){
      continue;
     }   
    String _id = sensorJson.get<String >("id");
    String  _type = sensorJson.get<String>("type");
    String _class =sensorJson.get<String>("class");
    String _name =sensorJson.get<String>("name");
    JsonArray& functions = sensorJson.get<JsonVariant>("functions");
     for(int i  = 0 ; i < functions.size() ; i++){
        JsonObject& f = functions[i]; 
        String _fname =f.get<String>("name");
        String _unit =f.get<String>("unit");
        String _mqttState =f.get<String>("mqttStateTopic");
        bool _retain =f.get<bool>("mqttRetain");   
        publishOnMqttQueue((getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix")+"/"+_class+"/"+getConfigJson().get<String>("nodeId")+"/"+_class+"_"+_fname+"_"+_id+"/config"),("{\"name\": \""+_fname+"\",\"unit_of_measurement\": \""+_unit+"\", \"state_topic\": \""+_mqttState+"\",\"availability_topic\": \""+getAvailableTopic()+"\",\"payload_available\":\"1\",\"payload_not_available\":\"0\"}"),true);
   } 
  }
}

void realoadHaConfig(){
  createHASwitchsComponent();
  createHASensorComponent();
}

void removeComponentHaConfig(String oldPrefix,String oldNodeId, String _type, String _class, String _id){
   publishOnMqtt((oldPrefix+"/"+_type+"/"+oldNodeId+"/"+_id+"/config"),"",true);
}
