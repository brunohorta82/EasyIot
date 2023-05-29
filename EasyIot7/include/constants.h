#ifndef CONSTANTS_H
#define CONSTANTS_H
namespace constantsMqtt
{
    constexpr const char *mqttCloudURL{"mqtt.bhonofre.pt"};
    constexpr const char *availablePayload{"online"};
    constexpr const char *homeAssistantAutoDiscoveryPrefix{"homeassistant"};
    constexpr const char *unavailablePayload{"offline"};
    constexpr const int defaultPort{1883};
    constexpr const char *homeassistantOnlineTopic{"homeassistant/status"};
} // namespace constantsMqtt
namespace constantsSensor
{
    constexpr const char *noneClass{"none"};
    constexpr const char *powerMeterClass{"power_meter"};
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
    constexpr const char *switches{"[SWITCHES]"};
    constexpr const char *alexa{"[ALEXA]"};
    constexpr const char *webserver{"[WEBSERVER]"};
    constexpr const char *emoncms{"[EMONCMS]"};
    constexpr const char *cloudIO{"[CLOUDIO]"};
} // namespace tags
namespace configFilenames
{
    constexpr const char *config = "/config.bin";
    constexpr const char *sensors = "/sensors.bin";
    constexpr const char *switches = "/switches.bin";

} // namespace configFilenames
namespace constantsConfig
{
#if NO_FEATURES
    constexpr const char *firmwareMode{"NO_FEATURES"};
#endif
#if DUAL_LIGHT
    constexpr const char *firmwareMode{"DUAL_LIGHT"};
#endif
#if COVER
    constexpr const char *firmwareMode{"COVER"};
#endif
#if COVER_V3
    constexpr const char *firmwareMode{"COVER_V3"};
#endif
#if BHPZEM_004T_V03_2_0
    constexpr const char *firmwareMode{"BHPZEM_004T_V03_2_0"};
#endif
#if GATE
    constexpr const char *firmwareMode{"GATE"};
#endif

    constexpr const char *newID{"NEW"};
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
} // namespace constanstsCloudIO

namespace constanstsSwitch
{

    constexpr const unsigned long delayDebounce{25ul};              // 25 milliseconds
    constexpr const unsigned long delayCoverProtection{50ul};       // 50 milliseconds
    constexpr const unsigned long coverAutoStopProtection{90000ul}; // after 90 seconds turn off all relay to enhance the lifecycle
    constexpr const char *payloadOn{"ON"};
    constexpr const char *payloadOff{"OFF"};
    constexpr const char *payloadClose{"CLOSE"};
    constexpr const char *payloadOpen{"OPEN"};
    constexpr const char *payloadStop{"STOP"};

    constexpr const char *familyLight{"light"};
    constexpr const char *familySwitch{"switch"};
    constexpr const char *familyCover{"cover"};
    constexpr const char *familyGate{"garage"};

    constexpr const int offIdx{0};
    constexpr const int onIdx{1};
    constexpr const int firtStopIdx{2};
    constexpr const int openIdx{3};
    constexpr const int secondStopIdx{4};
    constexpr const int closeIdx{5};
    constexpr const int coverStartIdx{2};
    constexpr const int converEndIdx{5};
    constexpr const int switchStartIdx{0};
    constexpr const int switchEndIdx{1};

} // namespace constanstsSwitch
#endif