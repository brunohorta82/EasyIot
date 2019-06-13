#define HARDWARE "onofre"
#define FACTORY_TYPE "cover" 
#define FIRMWARE_VERSION 6.11

const String DEFAULT_NODE_ID = String(HARDWARE) +"-"+String(FACTORY_TYPE)+"-"+String(ESP.getChipId())+"-"+String(FIRMWARE_VERSION);

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
bool shouldReboot = false;
bool reloadMqttConfiguration = false;
bool wifiUpdated = false;
bool laodDefaults = false;
bool adopted = false;
bool autoUpdate = false;

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
