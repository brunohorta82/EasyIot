#define HARDWARE "onofre"
#define FACTORY_TYPE "cover"  //suported types cover light
#define FIRMWARE_VERSION 6.11
#define FIRMWARE_VERSION_X "6x11"
const String DEFAULT_NODE_ID = String(HARDWARE) +"-"+String(FACTORY_TYPE)+"-"+String(ESP.getChipId())+"-"+String(FIRMWARE_VERSION_X);

#define CONFIG_FILENAME  "/config_bh"+String(HARDWARE)+".json"
#define CONFIG_BUFFER_SIZE 1024

//AP PASSWORD  
#define AP_SECRET "EasyIot@"

#define PAYLOAD_ON "ON"
#define PAYLOAD_OFF "OFF"
#define PAYLOAD_CLOSE "CLOSE"
#define PAYLOAD_OPEN "OPEN"
#define PAYLOAD_STOP "STOP"
#define PAYLOAD_LOCK "LOCK"
#define PAYLOAD_UNLOCK "UNLOCK"



//CONTROL FLAGS
bool REBOOT = false;
bool LOAD_DEFAULTS = false;
bool AUTO_UPDATE = false;
bool STORE_CONFIG = false;
bool WIFI_SCAN = false;

void requestConfigStorage(){
  STORE_CONFIG = true;
}
void requestReboot(){
  REBOOT = true;
}
void requestAutoUpdate(){
  AUTO_UPDATE = true;
}
void requestLoadDefaults(){
  AUTO_UPDATE = true;
}
void requestWifiScan(){
  WIFI_SCAN = true;
}

DynamicJsonBuffer jsonBuffer(CONFIG_BUFFER_SIZE);

JsonArray &getJsonArray() {
    return jsonBuffer.createArray();
}

JsonArray &getJsonArray(File file) {
    return jsonBuffer.parseArray(file);
}

JsonObject &getJsonObject() {
    return jsonBuffer.createObject();
}

JsonObject &getJsonObject(File file) {
    return jsonBuffer.parseObject(file);
}

JsonObject &getJsonObject(const char *data) {
    return jsonBuffer.parseObject(data);
}
