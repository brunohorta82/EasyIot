#define FIRMWARE_VERSION 5.0

#define HARDWARE "bhonofre"

//IMPORTANT CHANGE THIS IF USE ONOFRE COVER
//cover -> Estores
//single -> 1 Relé iluminação
//dual -> 2 relés iluminação
#define FACTORY_TYPE "blinds"


#define CONFIG_FILENAME  "/config_"+String(HARDWARE)+".json"
#define CONFIG_BUFFER_SIZE 1024

//WIFI
#define WIFI_SSID ""
#define WIFI_SECRET ""

//AP PASSWORD  
#define AP_SECRET "EasyIot@"

//MQTT  
#define MQTT_BROKER_IP ""
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#define PAYLOAD_ON "ON"
#define PAYLOAD_OFF "OFF"
#define PAYLOAD_CLOSE "CLOSE"
#define PAYLOAD_OPEN "OPEN"
#define PAYLOAD_STOP "STOP"
#define PAYLOAD_PULSE_OFF_ON "PULSE_OFF"
#define PAYLOAD_PULSE_ON_OFF "PULSE_ON"

//DEFAULT GPIOS
#define RELAY_ONE 4
#define RELAY_TWO 5
#define SWITCH_ONE 12
#define SWITCH_TWO 13
//CONTROL FLAGS
bool shouldReboot = false;
bool reloadMqttConfiguration = false;
bool wifiUpdated = false;
bool laodDefaults = false;
bool adopted = false;
bool autoUpdate = false;
int easyConfig = 0;

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
