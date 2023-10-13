#pragma once

enum Template
{
    NO_TEMPLATE,
    DUAL_LIGHT,
    COVER,
    GARAGE,
    PZEM,
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
    constexpr unsigned int noGPIO{99u};
    constexpr unsigned long storeConfigDelay{5000ul};
    constexpr const char *apSecret{"bhonofre"}; // AP PASSWORD
    constexpr const char *apiUser{"admin"};     // API USER
    constexpr const char *apiPassword{"xpto"};  // API PASSWORD
} // namespace constantsConfig

namespace constanstsCloudIO
{
    constexpr const char *mqttDns{"mqtt.bhonofre.pt"};
    constexpr int mqttPort{1883};
    constexpr const char *configUrl{"http://cloudio.bhonofre.pt/devices/config"};
} // namespace constanstsCloudIO

namespace constanstsFamilies
{
    constexpr const char *F_SWITCH{"SWITCH"};
    constexpr const char *F_LIGTH{"LIGTH"};
    constexpr const char *F_CLIMATE{"CLIMATE"};
    constexpr const char *F_SECURITY{"SECURITY"};
    constexpr const char *F_GENERIC{"GENERIC"};
    constexpr const char *F_ENERGY{"ENERGY"};
}

namespace constanstsI18N
{
    constexpr const char *T_LIGHT_ONE{"Interruptor1"};
    constexpr const char *T_LIGHT_TWO{"Interruptor2"};
    constexpr const char *T_GARAGE{"Garagem"};
    constexpr const char *T_COVER{"Estore"};
    constexpr const char *T_HAN{"Contador"};
    constexpr const char *T_ENERGY{"Energia"};
}