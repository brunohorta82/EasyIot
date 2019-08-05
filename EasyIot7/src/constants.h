#ifndef CONSTANTS_H
#define CONSTANTS_H
namespace constantsMqtt
{
constexpr const char *mqttCloudURL{"mqtt.bhonofre.pt"};
constexpr const char *availablePayload{"online"};
constexpr const char *unavailablePayload{"offline"};
constexpr const int defaultPort{1883};
constexpr const char *homeassistantOnlineTopic{"hass/status"};
} // namespace constantsMqtt
namespace constantsSensor
{
constexpr const char *noneClass{"None"};
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
} // namespace tags
namespace configFilenames
{
constexpr const char *config{"/config_bhonofre.json"};
constexpr const char *sensors{"/sensors.json"};
constexpr const char *switches{"/switches.json"};
} // namespace configFilenames
namespace constantsConfig
{
constexpr const char *updateURL{"http://release.bhonofre.pt/firmware.bin"};
constexpr const char *newID{"NEW"};
constexpr int noGPIO{99};

constexpr const char *apSecret{"EasyIot@"}; //AP PASSWORD
} // namespace constantsConfig

namespace constanstsSwitch
{

constexpr const unsigned long delayDebounce{50ul};              //50 milliseconds
constexpr const unsigned long delayCoverProtection{50ul};       //50 milliseconds
constexpr const unsigned long coverAutoStopProtection{90000ul}; // after 90 seconds turn off all relay to enhance the life cycle
constexpr const char *payloadOn{"ON"};
constexpr const char *payloadOff{"OFF"};
constexpr const char *payloadClose{"CLOSE"};
constexpr const char *payloadStateClose{"closed"};
constexpr const char *payloadOpen{"OPEN"};
constexpr const char *payloadStateOpen{"open"};
constexpr const char *payloadStop{"STOP"};
constexpr const char *payloadStateStop{""};
constexpr const char *payloadLock{"LOCK"};
constexpr const char *payloasStateLock{"LOCK"};
constexpr const char *payloadUnlock{"UNLOCK"};
constexpr const char *payloadStateUnlock{"UNLOCK"};
constexpr const char *payloadReleased{"RELEASED"};

constexpr const char *familyLight{"light"};
constexpr const char *familySwitch{"switch"};
constexpr const char *familyCover{"cover"};
constexpr const char *familyLock{"lock"};

constexpr const int offIdx{0};
constexpr const int onIdx{1};
constexpr const int firtStopIdx{2};
constexpr const int openIdx{3};
constexpr const int secondStopIdx{4};
constexpr const int closeIdx{5};
constexpr const int lockIdx{6};
constexpr const int unlockIdx{7};
constexpr const int coverStartIdx{2};
constexpr const int converEndIdx{5};
constexpr const int switchStartIdx{0};
constexpr const int switchEndIdx{1};
constexpr const int lockStartIdx{6};
constexpr const int lockEndIdx{8};
} // namespace constanstsSwitch
#endif