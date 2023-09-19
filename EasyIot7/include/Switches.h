#ifndef SWITCHES_H
#define SWITCHES_H
#include "constants.h"
#include "Arduino.h"
#include "ArduinoJson.h"
#include <vector>
#include "FS.h"
static const String STATES_POLL[] = {constanstsSwitch::payloadOff, constanstsSwitch::payloadOn, constanstsSwitch::payloadStop, constanstsSwitch::payloadOpen, constanstsSwitch::payloadStop, constanstsSwitch::payloadClose, constanstsSwitch::payloadOpen, constanstsSwitch::payloadClose};
class Bounce;
class Shutters;
enum SwitchMode
{
    SWITCH = 1,
    PUSH = 2,
    DUAL_SWITCH = 4,
    DUAL_PUSH = 5,
    GATE_SWITCH = 6
};

enum SwitchControlType
{
    GPIO_OUTPUT = 1,
    NONE = 2
};

struct SwitchT
{
    double firmware = 0.0;
    char id[32] = {0}; // Generated from name without spaces and no special characters
    char name[24] = {0};
    char family[10] = {0};    // switch, light, cover, lock
    SwitchMode mode = SWITCH; // MODE_SWITCH, MODE_PUSH, MODE_DUAL_SWITCH, MODE_DUAL_PUSH
    SwitchControlType typeControl = NONE;

    // INTEGRATIONS
    bool cloudIOSupport = true;
    bool haSupport = false;
    bool knxSupport = false;
    bool mqttSupport = false;

    // UTILS
    bool childLock = false;

    // GPIOS INPUT
    unsigned int primaryGpio = constantsConfig::noGPIO;
    unsigned int secondaryGpio = constantsConfig::noGPIO;
    unsigned int primaryStateGpio = constantsConfig::noGPIO;
    unsigned int secondaryStateGpio = constantsConfig::noGPIO;
    bool pullup = true; // USE INTERNAL RESISTOR

    // GPIOS OUTPUT
    unsigned int primaryGpioControl = constantsConfig::noGPIO;
    unsigned int secondaryGpioControl = constantsConfig::noGPIO;
    unsigned int thirdGpioControl = constantsConfig::noGPIO;
    bool inverted = false;

    // AUTOMATIONS
    unsigned long autoStateDelay = 0;
    char autoStateValue[10] = {0};

    // MQTT
    char mqttCommandTopic[128] = {0};
    char mqttStateTopic[128] = {0};
    bool mqttRetain = false;

    // CONTROL VARIABLES
    int lastPercentage = 0;
    bool lastPrimaryGpioState = true;
    bool lastSecondaryGpioState = true;
    bool lastPrimaryStateGpioState = true;
    bool lastSecondaryStateGpioState = true;
    Bounce *debouncerPrimary = nullptr;
    Bounce *debouncerSecondary = nullptr;
    int statePoolIdx = -1;
    bool isCover = false;
    bool isGate = false;
    unsigned long lastChangeState = 0;

    // CLOUDIO
    char mqttCloudCommandTopic[128] = {0};
    char mqttCloudStateTopic[128] = {0};

    // VIRTUAL COVER CONTROLLER
    Shutters *shutter;
    unsigned long upCourseTime = 25 * 1000;
    unsigned long downCourseTime = 25 * 1000;
    float calibrationRatio = 0.1;
    char shutterState[21] = {0};

    // KNX
    uint8_t knxLevelOne = 0;
    uint8_t knxLevelTwo = 0;
    uint8_t knxLevelThree = 0;
    uint8_t knxIdRegister = 0;
    uint8_t knxIdAssign = 0;
    // METHODS
    void load(File &file);
    void save(File &file) const;
    void updateFromJson(JsonObject doc);
    const String changeState(const char *state, const char *origin);
    const char *rotateState();
    void toJson(JsonVariant &root) const;
    const String getCurrentState() const;
    void configPins();
    const void notifyState(bool dirty, const char *origin);
    void reloadMqttTopics();
};
struct Switches
{
    unsigned long lastChange = 0ul;
    std::vector<SwitchT> items;
    void load(File &file);
    const char *rotate(const char *id);
    void save(File &file) const;
    void save(bool syncState);
    const String stateSwitchById(const char *id, const char *state);
    Switches &remove(const char *id);
    Switches &updateFromJson(const String &id, JsonObject &doc);
    void toJson(JsonVariant &root);
};
void stateSwitchByName(Switches &switches, const char *name, const char *state, const char *value);
void loop(Switches &switches);
void load(Switches &switches);
void mqttSwitchControl(Switches &switches, const char *topic, const char *payload);
void sendToServerEvents(const String &topic, const String &payload);
struct Switches &getAtualSwitchesConfig();
int findPoolIdx(const char *state, int currentIdx, const char *family);
void reloadSwitches();
#endif