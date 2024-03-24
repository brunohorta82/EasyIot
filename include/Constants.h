#pragma once
enum Template
{
    NO_TEMPLATE,
    DUAL_LIGHT,
    DUAL_SWITCH,
    COVER,
    GARAGE,
    HAN_MODULE,
    GARDEN,
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

namespace Payloads
{
    constexpr const char *presenceOnPayload{"detected"};
    constexpr const char *presenceOffPayload{"clear"};
    constexpr const char *motionOnPayload{"detected"};
    constexpr const char *motionOffPayload{"clear"};
    constexpr const char *windowDoornOnPayload{"closed"};
    constexpr const char *windowDoornOffPayload{"open"};
    constexpr const char *rainOnPayload{"rain"};
    constexpr const char *rainOffPayload{"clear"};
}
// namespace constantsSensor
namespace tags
{
    constexpr const char *system{"[SYSTEM]"};
    constexpr const char *config{"[CONFIG]"};
    constexpr const char *mqtt{"[MQTT]"};
    constexpr const char *wifi{"[WIFI]"};
    constexpr const char *discovery{"[DISCOVERY]"};
    constexpr const char *sensors{"[SENSORS]"};
    constexpr const char *actuatores{"[ACTUATORS]"};
    constexpr const char *webserver{"[WEBSERVER]"};
    constexpr const char *cloudIO{"[CLOUDIO]"};
    constexpr const char *homeassistant{"[HOMEASSISTANT]"};

} // namespace tags
namespace configFilenames
{
    constexpr const char *config = "/config.json";
}

namespace constantsConfig
{
    constexpr unsigned long SHUTTER_DEFAULT_COURSE_TIME_SECONS{25};
    constexpr unsigned long energyReadDelay{5000l};
    constexpr unsigned long climateReadDelay{10000l};
    constexpr unsigned long rainDelay{5000l};
    constexpr unsigned long pirDelay{100l};
    constexpr unsigned long ld2410Delay{200l};
    constexpr unsigned long hcsr04Delay{100l};
    constexpr unsigned long hallsensorDelay{1000l};
    constexpr unsigned long illuminanceReadDelay{5000l};
    constexpr unsigned long storeConfigDelay{5000ul};
    constexpr const char *apSecret{"bhonofre"}; // AP PASSWORD
    constexpr const char *apiUser{"admin"};     // API USER
    constexpr const char *apiPassword{"xpto"};  // API PASSWORD
    constexpr const char *PW_HIDE{"******"};
    constexpr unsigned long DEFAULT_TIME_SENSOR_ERROR_CLEAR{60000};
}
#ifdef ESP32
namespace DefaultPins
{
    // ESP32 DEFAULT PINS
    constexpr unsigned int noGPIO{99u};
    constexpr unsigned int INPUT_ONE{14u};
    constexpr unsigned int INPUT_TWO{13u};
    constexpr unsigned int OUTPUT_ONE{4u};
    constexpr unsigned int OUTPUT_TWO{5u};
    constexpr unsigned int PZEM_TX{26u};
    constexpr unsigned int PZEM_RX{27u};
    constexpr int SDA{32u};
    constexpr int SCL{33u};
    constexpr int HAN_TX{13u};
    constexpr int HAN_RX{14u};
    constexpr unsigned int OUTPUT_VALVE_THREE{noGPIO};
    constexpr unsigned int OUTPUT_VALVE_FOUR{noGPIO};
#ifdef ESP32_MAKER_4MB
    constexpr unsigned int outputInputPins[] = {7, 12, 13, 14, 19, 20, 21, 22, 25};
#else
    constexpr unsigned int outputInputPins[] = {7, 8, 12, 13, 14, 19, 20, 21, 22, 25};
#endif
    constexpr unsigned int intputOnlyPins[] = {34, 35, 36, 37, 38};
}
#endif
#ifdef ESP8266
namespace DefaultPins
{
    // ESP8266 DEFAULT PINS
    constexpr unsigned int noGPIO{999u};
    constexpr unsigned int INPUT_ONE{12u};
    constexpr unsigned int INPUT_TWO{13u};
    constexpr unsigned int OUTPUT_ONE{4u};
    constexpr unsigned int OUTPUT_TWO{5u};
    constexpr unsigned int OUTPUT_VALVE_THREE{14u};
    constexpr unsigned int OUTPUT_VALVE_FOUR{13u};
    constexpr unsigned int PZEM_TX{3u};
    constexpr unsigned int PZEM_RX{1u};
    constexpr int SDA{2u};
    constexpr int SCL{13u};
    constexpr int HAN_TX{14u};
    constexpr int HAN_RX{12u};
    constexpr unsigned int outputInputPins[] = {0, 1, 2, 3, 4, 5, 12, 13, 14, 16};
}
#endif
namespace constanstsCloudIO
{
    constexpr const char *mqttDns{"mqtt.bhonofre.pt"};
    constexpr int mqttPort{1883};
    constexpr const char *configUrl{"http://cloudio.bhonofre.pt/devices/config"};
#ifdef HAN_MODE
    constexpr const char *otaUrl{"http://cloudio.bhonofre.pt/firmware/update/latest?variant=ESP8266-HAN"};
#elif ESP32_MAKER_4MB
    constexpr const char *otaUrl{"http://cloudio.bhonofre.pt/firmware/update/latest?variant=ESP32-MAKER-4MB"};
#else
    constexpr const char *otaUrl{"http://cloudio.bhonofre.pt/firmware/update/latest"};
#endif
}

namespace Discovery
{
    constexpr int I2C_SHT4X_ADDRESS{0x44};           // TEMPERATURE/HUMIDITY
    constexpr int I2C_LTR303_ADDRESS{0x29};          // ILLUMINANCE
    constexpr int I2C_SSD1306_ADDRESS{0x3C};         // DISPLAY
    constexpr int I2C_TMF880X_ADDRESS{0x41};         // TIME OF FLY
    constexpr int MODBUS_PZEM_ADDRESS_START{0x10};   // POWER METER
    constexpr int MODBUS_PZEM_ADDRESS_DEFAULT{0xF8}; // POWER METER
}

namespace Family
{
    constexpr const char *SWITCH{"SWITCH"};
    constexpr const char *GARDEN{"GARDEN"};
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
    constexpr const char *VALVE_ONE{"Válvula 1"};
    constexpr const char *VALVE_TWO{"Válvula 2"};
    constexpr const char *VALVE_THREE{"Válvula 3"};
    constexpr const char *VALVE_FOUR{"Válvula 4"};
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
    constexpr const char *COVER_SINGLE_PUSH{"COVER_SINGLE_PUSH"};
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
    constexpr const char *HAN_8N2{"HAN_MODBUS_8N2"};
    constexpr const char *RAIN{"RAIN"};
    constexpr const char *DOOR{"DOOR"};
    constexpr const char *WINDOW{"WINDOW"};
    constexpr const char *PIR{"PIR"};
    constexpr const char *GARDEN_VALVE{"GARDEN_VALVE"};
    constexpr const char *HCSR04{"HCSR04"};
    constexpr const char *LD2410{"LD2410"};
    constexpr const char *TMF882X{"TMF882X"};

    constexpr const char *INVALID{"INVALID"};
}
