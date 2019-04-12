#define RELAY_TYPE "relay"
#define MQTT_TYPE "mqtt"
#define SWITCH_DEVICE "switch"
#define BUTTON_SWITCH 1
#define BUTTON_PUSH 2
#define OPEN_CLOSE_SWITCH 4
#define OPEN_CLOSE_PUSH 5
#define AUTO_OFF 6
#define BUTTON_SET_PULLUP true
#define INIT_STATE_OFF false
#define BUTTON_MASTER false
#define BUTTON_SLAVE true
#define EASY_LIGHT 1
#define EASY_BLINDS 2

String statesPool[] = {"OPEN","STOP","CLOSE","STOP"};

JsonArray& sws = getJsonArray();
bool coverNeedsStop;
long openCloseonTime;
int _gpioClose;
int _gpioOpen;
typedef struct {
    long onTime;
    int gpio;
    Bounce* debouncer;
    String id;
    bool pullup;
    int mode;
    bool state;
    bool locked;
   
    
} switch_t;
std::vector<switch_t> _switchs;

const String switchsFilename = "switchs.json";

JsonArray& saveSwitch(JsonArray& _switchs){

    for (unsigned int s=0; s < _switchs.size();s++) {
    JsonObject& _switch = _switchs.get<JsonVariant>(s);
      int switchFound = false;
   String type = _switch.get<String>("type");
   String _id = _switch.get<String>("id");
  for (unsigned int i=0; i < sws.size(); i++) {
     JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("id").equals(_id)){
      switchFound = true;
      removeComponentHaConfig(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix"),getConfigJson().get<String>("nodeId"),switchJson.get<String>("type"),switchJson.get<String>("class"),switchJson.get<String>("id"));
      String _name = _switch.get<String>("name");
      switchJson.set("gpio",_switch.get<unsigned int>("gpio"));
      switchJson.set("gpioOpen",_switch.get<unsigned int>("gpioOpen"));
      switchJson.set("gpioClose",_switch.get<unsigned int>("gpioClose"));
      switchJson.set("name",_name);
      switchJson.set("spot",_switch.get<String>("spot"));
      switchJson.set("discoveryDisabled",_switch.get<bool>("discoveryDisabled"));
      switchJson.set("pullup",_switch.get<bool>("pullup"));
      int swMode = _switch.get<unsigned int>("mode");
       switchJson.set("mode",swMode);
      String typeControl = _switch.get<String>("typeControl");
      switchJson.set("typeControl",typeControl);
      switchJson.set("pullState",0);
      switchJson.set("type",_switch.get<String>("type"));     
      if(!typeControl.equals(RELAY_TYPE) && (swMode != OPEN_CLOSE_SWITCH || swMode != OPEN_CLOSE_SWITCH)){
        switchJson.remove("gpioControl");
       }else{
        switchJson.set("gpioControl",_switch.get<unsigned int>("gpioControl"));
        switchJson.set("gpioControlOpen",_switch.get<unsigned int>("gpioControlOpen"));
        switchJson.set("gpioControlClose",_switch.get<unsigned int>("gpioControlClose"));
       }
      switchJson.set("master",_switch.get<bool>("master"));
      String mqttCommand = MQTT_COMMAND_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name);
      switchJson.set("mqttCommandTopic",mqttCommand);
      switchJson.set("mqttStateTopic",MQTT_STATE_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name));
      subscribeOnMqtt(mqttCommand);
    }
  }
  if(!switchFound){
      String _name = _switch.get<String>("name");
      String _id = "B"+String(millis());
      String typeControl = _switch.get<String>("typeControl");
      switchJson(_id,_switch.get<unsigned int>("gpio"),_switch.get<unsigned int>("gpioOpenClose"),_switch.get<unsigned int>("gpio"),typeControl,_switch.get<unsigned int>("gpioControl"),_switch.get<unsigned int>("gpioControlOpen"),_switch.get<unsigned int>("gpioControlClose"),INIT_STATE_OFF,_name, _switch.get<bool>("pullup"),INIT_STATE_OFF,  _switch.get<unsigned int>("mode"), _switch.get<bool>("master"), MQTT_STATE_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name), MQTT_COMMAND_TOPIC_BUILDER(_id,SWITCH_DEVICE,_name), _switch.get<String>("type"));
  }
  }
  saveSwitchs();
  applyJsonSwitchs();
  createHASwitchsComponent();  
  return sws;
 }
 
 void openAction(int gpioClose, int gpioOpen){
  logger("[SWITCH] OPEN");
  stopAction(gpioClose, gpioOpen);
  delay(100); 
  turnOff( getRelay(gpioClose));
  delay(100);  
  turnOn( getRelay(gpioOpen));
  delay(100);
 coverAutoStop(gpioClose,  gpioOpen);
}
void closeAction(int gpioClose, int gpioOpen){
   stopAction(gpioClose, gpioOpen);
  logger("[SWITCH] CLOSE");
  delay(100);  
  turnOff( getRelay(gpioOpen));
  delay(100);  
  turnOn( getRelay(gpioClose));
  delay(100); 
 coverAutoStop(gpioClose,  gpioOpen);
}
void stopAction(int gpioClose, int gpioOpen){
  logger("[SWITCH] STOP");
  turnOff( getRelay(gpioClose));  
  turnOff( getRelay(gpioOpen));
  coverNeedsStop = false;
}
void coverAutoStop(int gpioClose, int gpioOpen){
  coverNeedsStop = true;
  openCloseonTime = millis();
  _gpioClose = gpioClose;
  _gpioOpen =  gpioOpen;
  }
void stateSwitch(String id, String state) {
  for (unsigned int i=0; i < sws.size(); i++) {
     JsonObject& switchJson = sws.get<JsonVariant>(i);   
    if(switchJson.get<String>("id").equals(id)){
      switchJson.set("stateControlCover",state);
    if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
      int gpioOpen =switchJson.get<unsigned int>("gpioControlOpen");
      int gpioClose = switchJson.get<unsigned int>("gpioControlClose");
      if(String("OPEN").equals(state)){
        switchJson.set("positionControlCover",1);
        openAction(gpioClose,gpioOpen);
        }else if(String("STOP").equals(state)){
          stopAction(gpioClose,gpioOpen);
        }if(String("CLOSE").equals(state)){
          switchJson.set("positionControlCover",0);
          closeAction(gpioClose,gpioOpen);
        }
        }
        publishState(switchJson);      
       }
    }
}
void stateSwitchByName(String name, String state) {
  for (unsigned int i=0; i < sws.size(); i++) {
     JsonObject& switchJson = sws.get<JsonVariant>(i);   
    if(switchJson.get<String>("name").equals(name)){
      switchJson.set("stateControlCover",state);
    if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
      int gpioOpen =switchJson.get<unsigned int>("gpioControlOpen");
      int gpioClose = switchJson.get<unsigned int>("gpioControlClose");
      int gpioControl = switchJson.get<unsigned int>("gpioControl");
      if(String("OPEN").equals(state)){
        switchJson.set("positionControlCover",1);
        openAction(gpioClose,gpioOpen);
        }else if(String("STOP").equals(state)){
          stopAction(gpioClose,gpioOpen);
        }else if(String("CLOSE").equals(state)){
          switchJson.set("positionControlCover",0);
          closeAction(gpioClose,gpioOpen);
        }else if(String("ON").equals(state)){
         turnOn( getRelay(switchJson.get<unsigned int>("gpioControl")));
          } else if(String("OFF").equals(state)){
             turnOff( getRelay(switchJson.get<unsigned int>("gpioControl")));
            }
        }
        publishState(switchJson);      
       }
    }
}

void applyJsonSwitchs(){
  _switchs.clear();
  for(int i  = 0 ; i < sws.size() ; i++){ 
    JsonObject& switchJson = sws.get<JsonVariant>(i);   
    int gpio= switchJson.get<unsigned int>("gpio");
      bool lock =switchJson.get<bool>("childLockStateControl");
    if(gpio ==  99){
       _switchs.push_back({0,gpio,new Bounce(),switchJson.get<String>("id"),false,switchJson.get<unsigned int>("mode"),false});
      continue;
     }
    if(switchJson.get<unsigned int>("mode") == OPEN_CLOSE_SWITCH){
          int gpioOpen= switchJson.get<unsigned int>("gpioOpen");
          int gpioClose= switchJson.get<unsigned int>("gpioClose");
          bool pullup =switchJson.get<bool>("pullup");
          bool state =switchJson.get<bool>("state");
        
          int gpioControl = switchJson.get<unsigned int>("gpioControl");
          if ( gpioOpen == 16) {
             configGpio(gpioOpen, INPUT_PULLDOWN_16);
          } else {
             configGpio(gpioOpen, pullup ? INPUT_PULLUP  : INPUT);
          }
          if ( gpioClose == 16) {
             configGpio(gpioClose, INPUT_PULLDOWN_16);
          } else {
             configGpio(gpioClose, pullup ? INPUT_PULLUP  : INPUT);
          }
    Bounce* debouncerOpen = new Bounce(); 
    Bounce* debouncerClose = new Bounce(); 
    debouncerOpen->attach(gpioOpen);
    debouncerOpen->interval(5); // interval in ms
    debouncerClose->attach(gpioClose);
    debouncerClose->interval(5); // interval in ms
    if(switchJson.get<String>("stateControlCover").equals(PAYLOAD_OPEN)){
      _switchs.push_back({0,gpioOpen,debouncerOpen,switchJson.get<String>("id"),pullup,OPEN_CLOSE_SWITCH,state, lock});
      _switchs.push_back({0,gpioClose,debouncerClose,switchJson.get<String>("id"),pullup,OPEN_CLOSE_SWITCH,!state,lock});
    }else if(switchJson.get<String>("stateControlCover").equals(PAYLOAD_CLOSE)){
      _switchs.push_back({0,gpioOpen,debouncerOpen,switchJson.get<String>("id"),pullup,OPEN_CLOSE_SWITCH,!state,lock});
      _switchs.push_back({0,gpioClose,debouncerClose,switchJson.get<String>("id"),pullup,OPEN_CLOSE_SWITCH,state,lock});
    }else{
      _switchs.push_back({0,gpioOpen,debouncerOpen,switchJson.get<String>("id"),pullup,OPEN_CLOSE_SWITCH,state,lock});
      _switchs.push_back({0,gpioClose,debouncerClose,switchJson.get<String>("id"),pullup,OPEN_CLOSE_SWITCH,state,lock});
      }
    //initNormal(switchJson.get<bool>("stateControl"),gpioControl);
      }else{
        
    bool pullup =switchJson.get<bool>("pullup");
    bool state =switchJson.get<bool>("state");
    int gpioControl = switchJson.get<unsigned int>("gpioControl");
    if ( gpio == 16) {
      configGpio(gpio, INPUT_PULLDOWN_16);
    } else {
      configGpio(gpio, pullup ? INPUT_PULLUP  : INPUT);
    }
    Bounce* debouncer = new Bounce(); 
    debouncer->attach(gpio);
    debouncer->interval(5); // interval in ms
    _switchs.push_back({0,gpio,debouncer,switchJson.get<String>("id"),pullup,switchJson.get<unsigned int>("mode"),state,lock});
    if(switchJson.get<unsigned int>("mode") == AUTO_OFF){
      switchJson.set("stateControl",false);
    }
       initNormal(switchJson.get<bool>("stateControl"),gpioControl);
      
        }

  }
}

void toogleSwitch(String id) {
     for (unsigned int i=0; i < _switchs.size(); i++) {
    if(  _switchs[i].id.equals(id)){
      _switchs[i].onTime = millis();
    }
    
   }
  for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("id").equals(id)){
    if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
      if(switchJson.get<unsigned int>("mode") == AUTO_OFF){
          switchJson.set("stateControl",turnOnRelayNormal( switchJson.get<unsigned int>("gpioControl")));
      }else{
          bool gpioState = toogleNormal( switchJson.get<unsigned int>("gpioControl"));
          switchJson.set("stateControl",gpioState);
        }  
    }else if(switchJson.get<String>("typeControl").equals(MQTT_TYPE)){
       switchJson.set("stateControl",!switchJson.get<bool>("stateControl"));
       publishState(switchJson); 
    }
    }
   }   
}

void mqttSwitchControl(String topic, String payload) {
  for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("mqttCommandTopic").equals(topic)){
    if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
       int gpio = switchJson.get<unsigned int>("gpioControl");
      if(String(PAYLOAD_ON).equals(payload)){
        turnOn(getRelay(gpio));
        }else if (String(PAYLOAD_OFF).equals(payload)){
        turnOff( getRelay(gpio));  
       } else if(String(PAYLOAD_OPEN).equals(payload) || String(PAYLOAD_CLOSE).equals(payload) ||String(PAYLOAD_STOP).equals(payload)){
       stateSwitch(switchJson.get<String>("id"), payload);
       }else if(String("CHILD_UNLOCK").equals(payload)){
       switchJson.set("childLockStateControl",false);
         saveSwitchs();
         applyJsonSwitchs();
      }else if(String("CHILD_LOCK").equals(payload)){
      switchJson.set("childLockStateControl",true);
        saveSwitchs();
        applyJsonSwitchs();
      }else if(payload.equals("LOCK") ||payload.equals("UNLOCK")){
        toogleSwitch(switchJson.get<String>("id"));
        } 
    }else  if(switchJson.get<String>("typeControl").equals(MQTT_TYPE)){
      toogleSwitch(switchJson.get<String>("id"));
    }
    }
   }
 }   


void triggerSwitch(bool _state,  String id, int gpio) {
   for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<String>("id").equals(id)){
      switchJson.set("state",_state);
      if(switchJson.get<String>("typeControl").equals(RELAY_TYPE)){
        if( switchJson.get<unsigned int>("mode") == OPEN_CLOSE_PUSH){
          int currentStatePool = switchJson.get<unsigned int>("currentStatePool");
              stateSwitch(id,statesPool[currentStatePool%4]);
              switchJson.set("currentStatePool",currentStatePool+1);
              
         }else if( switchJson.get<unsigned int>("mode") == OPEN_CLOSE_SWITCH){
          if(gpio == switchJson.get<unsigned int>("gpioOpen") && _state){
              stateSwitch(id,"OPEN");
         }else if(gpio == switchJson.get<unsigned int>("gpioClose") && _state){
              stateSwitch(id,"CLOSE");      
         }else{
          stateSwitch(id,"STOP");
         }
        }else{
           if( switchJson.get<unsigned int>("mode") == AUTO_OFF){
              if(_state){
                  turnOn( getRelay(switchJson.get<unsigned int>("gpioControl")));
                }else{
                  turnOff( getRelay(switchJson.get<unsigned int>("gpioControl")));
                }
            }else{
              bool gpioState = toogleNormal(switchJson.get<unsigned int>("gpioControl"));
               switchJson.set("stateControl",gpioState);  
            }
         }
    
      }else if(switchJson.get<String>("typeControl").equals(MQTT_TYPE)){
         toogleSwitch(switchJson.get<String>("id"));
      }
    }
   }   
}

void publishState(JsonObject& switchJson){
    saveSwitchs();
    String swtr = "";
    switchJson.printTo(swtr);
    publishOnEventSource("switch",swtr);
    if(switchJson.get<String>("type").equals("cover")){
      publishOnMqtt(switchJson.get<String>("mqttStateTopic").c_str(),String(switchJson.get<unsigned int>("positionControlCover")),true);
    }else{
      publishOnMqtt(switchJson.get<String>("mqttStateTopic").c_str(),switchJson.get<bool>("stateControl") ? PAYLOAD_ON : PAYLOAD_OFF,true);   
     }
    
}

void switchNotify(int gpio, bool _gpioState){
  for (unsigned int i=0; i < sws.size(); i++) {
     JsonObject& switchJson = sws.get<JsonVariant>(i);
    if(switchJson.get<unsigned int>("gpioControl") == gpio){
      switchJson.set("stateControl",_gpioState);
      publishState( switchJson);
    }
  }
  
}

JsonArray& getStoredSwitchs(){
  return sws;
}

void loadStoredSwitchs(){
  bool loadDefaults = false;
  if(SPIFFS.begin()){
    File cFile;   

    if(SPIFFS.exists(switchsFilename)){
      cFile = SPIFFS.open(switchsFilename,"r+"); 
      if(!cFile){
        logger("[SWITCH] Create file switchs Error!");
        return;
      }
        logger("[SWITCH] Read stored file config...");
       JsonArray& storedSwitchs = getJsonArray(cFile);       
      for(int i = 0 ; i< storedSwitchs.size(); i++){
        sws.add(storedSwitchs.get<JsonVariant>(i));
        }
        if (!storedSwitchs.success()) {
         logger("[SWITCH] Json file parse Error!");
          loadDefaults = true;
        }else{
          logger("[SWITCH] Apply stored file config...");
          applyJsonSwitchs();
        }
        
     }else{
        loadDefaults = true;
     }
    cFile.close();
     if(loadDefaults){
      logger("[SWITCH] Apply default config...");
      cFile = SPIFFS.open(switchsFilename,"w+");
      createDefaultSwitchs(FACTORY_TYPE == "cover" ? EASY_BLINDS : EASY_LIGHT);
      sws.printTo(cFile);
      applyJsonSwitchs();
      cFile.close();
      }
     
  }else{
     logger("[SWITCH] Open file system Error!");
  }
   SPIFFS.end(); 
   
}

void saveSwitchs(){
   if(SPIFFS.begin()){
      logger("[SWITCH] Open "+switchsFilename);
      File rFile = SPIFFS.open(switchsFilename,"w+");
      if(!rFile){
        logger("[SWITCH] Open switch file Error!");
      } else {
       
      sws.printTo(rFile);
      }
      rFile.close();
   }else{
     logger("[SWITCH] Open file system Error!");
  }
  SPIFFS.end();
  logger("[SWITCH] New switch config loaded.");
}

void switchJson(String _id,int _gpio ,int _gpioOpen ,int _gpioClose ,String _typeControl, int _gpioControl,int _gpioControlOpen,int _gpioControlClose, bool _stateControl,  String _name, bool _pullup, bool _state, int _mode, bool _master, String _mqttStateTopic, String _mqttCommandTopic, String _type){
    JsonObject& switchJson =  getJsonObject();
      switchJson.set("id", _id);
      switchJson.set("gpio", _gpio);
      switchJson.set("gpioOpen", _gpioClose);
      switchJson.set("gpioClose", _gpioOpen);
      switchJson.set("pullup", _pullup);
      if(_typeControl.equals(RELAY_TYPE)){
        switchJson.set("gpioControl", _gpioControl);
        switchJson.set("gpioControlOpen",_gpioControlOpen);
        switchJson.set("gpioControlClose",_gpioControlClose);
      }
      switchJson.set("typeControl", _typeControl);
      switchJson.set("stateControl", _stateControl);
      switchJson.set("mqttStateTopic", _mqttStateTopic);
      switchJson.set("mqttCommandTopic", _mqttCommandTopic);
      switchJson.set("mqttRetain", true);
      switchJson.set("master", _master);
      switchJson.set("name", _name);
      switchJson.set("mode", _mode);
      switchJson.set("state", _state);
      switchJson.set("childLockStateControl", false);
      switchJson.set("type", _type);
      switchJson.set("class", SWITCH_DEVICE);
      sws.add(switchJson);
}
void rebuildSwitchMqttTopics( String oldPrefix,String oldNodeId){
      bool store = false;
      JsonArray& _devices = getStoredSwitchs();
      for(int i  = 0 ; i < _devices.size() ; i++){ 
        store = true;
      JsonObject& switchJson = _devices[i];
      removeComponentHaConfig(oldPrefix,oldNodeId,switchJson.get<String>("type"),switchJson.get<String>("class"),switchJson.get<String>("id"));      
      String id = switchJson.get<String>("id");
      String name = switchJson.get<String>("name");
      switchJson.set("mqttCommandTopic",MQTT_COMMAND_TOPIC_BUILDER(id,SWITCH_DEVICE,name));
      switchJson.set("mqttStateTopic",MQTT_STATE_TOPIC_BUILDER(id,SWITCH_DEVICE,name));
      subscribeOnMqtt(switchJson.get<String>("mqttCommandTopic"));
    }
    if(store){
      saveSwitchs();
      
        
        createHASwitchsComponent();  
      
    }
  }


void createDefaultSwitchs(int type){
    String id1 = "B1";
    if(type == EASY_LIGHT){    
      String id2 = "B2";
      switchJson(id1,SWITCH_ONE,0,0,RELAY_TYPE,RELAY_ONE,0,0,INIT_STATE_OFF,  "Interruptor1", BUTTON_SET_PULLUP,INIT_STATE_OFF,  BUTTON_SWITCH, BUTTON_MASTER, MQTT_STATE_TOPIC_BUILDER(id1,SWITCH_DEVICE,"Interrutor1"), MQTT_COMMAND_TOPIC_BUILDER(id1,SWITCH_DEVICE,"Interruptor1"), "light");
      switchJson(id2,SWITCH_TWO,0,0,RELAY_TYPE,RELAY_TWO,0,0, INIT_STATE_OFF, "Interruptor2", BUTTON_SET_PULLUP,INIT_STATE_OFF,  BUTTON_SWITCH, BUTTON_MASTER, MQTT_STATE_TOPIC_BUILDER(id2,SWITCH_DEVICE,"Interrutor2"),MQTT_COMMAND_TOPIC_BUILDER(id2,SWITCH_DEVICE,"Interruptor2"), "light");
    }else if(type == EASY_BLINDS){
      switchJson(id1,0,SWITCH_ONE,SWITCH_TWO,RELAY_TYPE,0,RELAY_ONE,RELAY_TWO,INIT_STATE_OFF,"Interruptor", BUTTON_SET_PULLUP,INIT_STATE_OFF,  OPEN_CLOSE_SWITCH, BUTTON_MASTER, MQTT_STATE_TOPIC_BUILDER(id1,SWITCH_DEVICE,"Interrutor"), MQTT_COMMAND_TOPIC_BUILDER(id1,SWITCH_DEVICE,"Interruptor"), "cover");
   }
}
void removeSwitch(String _id){
  int switchFound = false;
  int index = 0;
  for (unsigned int i=0; i < sws.size(); i++) {
    JsonObject& switchJson = sws.get<JsonVariant>(i);   
    if(switchJson.get<String>("id").equals(_id)){
      switchFound = true;
      index  = i;
      removeComponentHaConfig(getConfigJson().get<String>("homeAssistantAutoDiscoveryPrefix"),getConfigJson().get<String>("nodeId"),switchJson.get<String>("type"),switchJson.get<String>("class"),switchJson.get<String>("id"));
    }
  }
  if(switchFound){
    sws.remove(index);
     
    }

  saveSwitchs();
  applyJsonSwitchs();
  
    createHASwitchsComponent();  
 
}
void loopSwitchs(){
    if(coverNeedsStop){
        if(openCloseonTime+25000 < millis() ){
          stopAction(_gpioClose,_gpioOpen);
          }
    }
    for (unsigned int i=0; i < _switchs.size(); i++) {
      if(_switchs[i].locked){
        continue;}
      Bounce* b =   _switchs[i].debouncer;
      b->update();
      bool value =  b->read();
    
      value = _switchs[i].pullup ? !value : value;
      int swmode = _switchs[i].mode;
       if(swmode == AUTO_OFF) {
         
        if(_switchs[i].onTime > 0 && _switchs[i].onTime + 1000 < millis()){
          _switchs[i].onTime = 0;
        
          triggerSwitch( false, _switchs[i].id, _switchs[i].gpio);
          continue;
          }
        }
          if(_switchs[i].state != value){
            _switchs[i].state = value;
            if(swmode == BUTTON_SWITCH || swmode == OPEN_CLOSE_SWITCH || !value) {   
                if(swmode == AUTO_OFF) {
                  value = !value;
                }
              triggerSwitch( value, _switchs[i].id, _switchs[i].gpio);
            } 
         }
    }
}
 void tryMe(){
    for (unsigned int i=0; i < _switchs.size(); i++) {
      bool value =   !_switchs[i].state;
      int swmode = _switchs[i].mode;
       if(swmode == AUTO_OFF) {
        if(_switchs[i].onTime > 0 && _switchs[i].onTime + 1000 < millis()){
          _switchs[i].onTime = 0;
          triggerSwitch( false, _switchs[i].id, _switchs[i].gpio);
          continue;
          }
        }
          if(_switchs[i].state != value){
            _switchs[i].state = value;
            if(swmode == BUTTON_SWITCH || swmode == OPEN_CLOSE_SWITCH || !value) {   
                if(swmode == AUTO_OFF) {
                  value = !value;
                }
              triggerSwitch( value, _switchs[i].id, _switchs[i].gpio);
            } 
         }
    }
}
 
