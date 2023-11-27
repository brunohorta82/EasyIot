#pragma once

enum Template
{
    NO_TEMPLATE,
    DUAL_LIGHT,
    DUAL_SWITCH,
    COVER,
    GARAGE,
    HAN_MODULE
};
namespace constantsMqtt
{
    constexpr const char *mqttCloudURL{"mqtt.bhonofre.pt"};
    constexpr const char *availablePayload{"online"};
    constexpr const char *homeAssistantAutoDiscoveryPrefixLegacy{"hass"};
    constexpr const char *homeAssistantAutoDiscoveryPrefix{"homeassistant"};
    constexpr const char *unavailablePayload{"offline"};
    constexpr const int defaultPort{1883};
} // namespace constantsMqtt
namespace constantsSensor
{
    constexpr const char *noneClass{"none"};
    constexpr const char *familySensor{"sensor"};
    constexpr const char *binarySensorFamily{"binary_sensor"};
    constexpr unsigned long DEFAULT_TIME_SENSOR_ERROR_CLEAR{60000};
} // namespace constantsSensor
namespace tags
{
    constexpr const char *system{"[SYSTEM]"};
    constexpr const char *config{"[CONFIG]"};
    constexpr const char *mqtt{"[MQTT]"};
    constexpr const char *wifi{"[WIFI]"};
    constexpr const char *discovery{"[DISCOVERY]"};
    constexpr const char *sensors{"[SENSORS]"};
    constexpr const char *actuatores{"[ACTUATORES]"};
    constexpr const char *alexa{"[ALEXA]"};
    constexpr const char *webserver{"[WEBSERVER]"};
    constexpr const char *emoncms{"[EMONCMS]"};
    constexpr const char *cloudIO{"[CLOUDIO]"};
    constexpr const char *homeassistant{"[homeassistant]"};

} // namespace tags
namespace configFilenames
{
    constexpr const char *config = "/config.json";
} // namespace configFilenames
namespace constantsConfig
{
#ifdef ESP8266
    constexpr unsigned int INPUT_ONE{12u};
#endif
#ifdef ESP32
    constexpr unsigned int INPUT_ONE{14u};
#endif
    constexpr unsigned int INPUT_TWO{13u};
    constexpr unsigned int OUTPUT_ONE{4u};
    constexpr unsigned int OUTPUT_TWO{5u};
#ifdef ESP32
    constexpr unsigned int PZEM_TX{26u};
    constexpr unsigned int PZEM_RX{27u};
#endif
#ifdef ESP8266
    constexpr unsigned int PZEM_TX{3u};
    constexpr unsigned int PZEM_RX{1u};
#endif
    constexpr unsigned int noGPIO{99u};
#ifdef ESP32
    constexpr int SDA{32};
    constexpr int SCL{33};
#endif
#ifdef ESP8266
    constexpr int SDA{2};
    constexpr int SCL{13};
#endif
#ifdef ESP32
    constexpr int HAN_TX{13};
    constexpr int HAN_RX{14};
#endif
#ifdef ESP8266
    constexpr int HAN_TX{1};
    constexpr int HAN_RX{3};
#endif
    constexpr unsigned long SHUTTER_DEFAULT_COURSE_TIME_SECONS{25};
    constexpr unsigned long energyReadDelay{5000l};
    constexpr unsigned long climateReadDelay{60000l};
    constexpr unsigned long illuminanceReadDelay{5000l};
    constexpr unsigned long storeConfigDelay{5000ul};
    constexpr const char *apSecret{"bhonofre"}; // AP PASSWORD
    constexpr const char *apiUser{"admin"};     // API USER
    constexpr const char *apiPassword{"xpto"};  // API PASSWORD
    constexpr const char *PW_HIDE{"******"};
}

namespace constanstsCloudIO
{
    constexpr const char *mqttDns{"mqtt.bhonofre.pt"};
    constexpr int mqttPort{1883};
    constexpr const char *configUrl{"http://cloudio.bhonofre.pt/devices/config"};
    constexpr const char *otaUrl{"http://update.bhonofre.pt/firmware/update"};

}

namespace Discovery
{
    constexpr int I2C_SHT4X_ADDRESS{0x44};         // TEMPERATURE/HUMIDITY
    constexpr int I2C_LTR303_ADDRESS{0x29};        // ILLUMINANCE
    constexpr int I2C_SSD1306_ADDRESS{0x3C};       // DISPLAY
    constexpr int MODBUS_PZEM_ADDRESS_START{0x10}; // POWER METER
}

namespace Family
{
    constexpr const char *SWITCH{"SWITCH"};
    constexpr const char *LIGTH{"LIGHT"};
    constexpr const char *CLIMATE{"CLIMATE"};
    constexpr const char *SECURITY{"SECURITY"};
    constexpr const char *ENERGY{"ENERGY"};
    constexpr const char *LEVEL_METER{"LEVEL_METER"};
    constexpr const char *NONE{"NONE"};

}

namespace I18N
{
    constexpr const char *SWICTH_ONE{"Interruptor1"};
    constexpr const char *SWICTH_TWO{"Interruptor2"};
    constexpr const char *GARAGE{"Garagem"};
    constexpr const char *COVER{"Estore"};
    constexpr const char *HAN{"Contador"};
    constexpr const char *ENERGY{"Energia "};
    constexpr const char *ILLUMINANCE{"Iluminação "};
    constexpr const char *CLIMATIZATION{"Climatização"};
    constexpr const char *NO_NAME{"Sem Nome"};

}

namespace FeatureDrivers
{
    constexpr const char *SWITCH_PUSH{"SWITCH_PUSH"};
    constexpr const char *SWITCH_LATCH{"SWITCH_LATCH"};
    constexpr const char *COVER_DUAL_PUSH{"COVER_DUAL_PUSH"};
    constexpr const char *COVER_DUAL_LATCH{"COVER_DUAL_LATCH"};
    constexpr const char *LOCK_PUSH{"LOCK_PUSH"};
    constexpr const char *LIGHT_PUSH{"LIGHT_PUSH"};
    constexpr const char *LIGHT_LATCH{"LIGHT_LATCH"};
    constexpr const char *GARAGE_PUSH{"GARAGE_PUSH"};
    constexpr const char *LTR303{"LTR303"};
    constexpr const char *DS18B20{"DS18B20"};
    constexpr const char *DHT_11{"DHT_11"};
    constexpr const char *DHT_21{"DHT_21"};
    constexpr const char *DHT_22{"DHT_22"};
    constexpr const char *SHT4X{"SHT4X"};
    constexpr const char *PZEM_004T_V03{"PZEM_004T_V03"};
    constexpr const char *HAN{"HAN_MODBUS"};
    constexpr const char *GENERIC{"GENERIC"};
}
