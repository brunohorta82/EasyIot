#pragma once
#include "Arduino.h"
#include "constants.h"
#include <Bounce2.h>
#include "ArduinoJson.h"
#include <vector>
#include "FS.h"
class Shutters;
enum SwitchState
{
    OFF = 0,
    ON = 1,
    OPEN = 2,
    CLOSE = 3,
    STOP = 4,
    PERCENTAGE = 5,
};
enum SwitchStateOrigin
{
    INTERNAL = 0,
    GPIO_INPUT = 1,
    KNX = 2,
    MQTT = 3,
    CLOUDIO = 4,
    WEBPANEL = 5
};

enum SwitchFamily
{
    SWITCH_PUSH = 1,
    SWITCH_GENERIC = 2,
    COVER_PUSH = 3,
    COVER_DUAL_PUSH = 4,
    COVER_DUAL_GENERIC = 5,
    LOCK_PUSH = 6,
    LIGHT_PUSH = 7,
    LIGHT_GENERIC = 8,
    GARAGE_PUSH = 9
};

enum SwitchControlType
{
    GPIO_OUTPUT = 1,
    VIRTUAL = 2
};

class ActuatorT
{
public:
    // CONFIG
    int id = 0;
    char name[24] = {0};
    SwitchFamily family = SWITCH_PUSH;
    SwitchControlType typeControl = VIRTUAL;
    // INTEGRATIONS
    bool cloudIOSupport = true;
    bool haSupport = false;
    // KNX
    uint8_t knxAddress[3] = {0, 0, 0};
    uint8_t knxIdRegister = 0;
    uint8_t knxIdAssign = 0;
    // MQTT
    char writeTopic[128]{};
    char readTopic[128]{};
    // GPIOS INPUT
    std::vector<unsigned int> inputs;
    std::vector<Bounce> inputsBounced;

    // GPIOS OUTPUT
    std::vector<unsigned int> outputs;

    // CONTROL VARIABLES
    unsigned int lastState = 0;
    unsigned int state = 0;
    int lastPercentage = 0;
    bool lastPrimaryGpioState = true;
    bool lastSecondaryGpioState = true;
    bool lastPrimaryStateGpioState = true;
    bool lastSecondaryStateGpioState = true;

    // VIRTUAL COVER CONTROLLER
    Shutters *shutter;
    unsigned long upCourseTime = 25 * 1000;
    unsigned long downCourseTime = 25 * 1000;
    float calibrationRatio = 0.1;
    char shutterState[21] = {0};

    // METHODS
    constexpr bool isCover()
    {
        return family == COVER_PUSH || family == COVER_DUAL_PUSH || family == COVER_DUAL_GENERIC;
    };
    constexpr bool isLight()
    {
        return family == LIGHT_PUSH || family == LIGHT_GENERIC;
    };
    constexpr bool isSwitch()
    {
        return family == SWITCH_PUSH || family == SWITCH_GENERIC;
    };
    constexpr bool isGarage()
    {
        return family == GARAGE_PUSH;
    };
    constexpr bool isKnxSupport()
    {
        return knxAddress[0] > 0 && knxAddress[1] >= 0 && knxAddress[2] >= 0;
    };
    constexpr bool isKnxGroup()
    {
        return knxAddress[0] > 0 && knxAddress[1] >= 0 && knxAddress[2] == 0;
    };
    ActuatorT *changeState(SwitchStateOrigin origin, int state);
    const char *rotateState();
    const String getCurrentState();
    void setup();
    const void notifyState(bool dirty, const char *origin);
};

void stateSwitchByName(const char *name, const char *state, const char *value);
void mqttSwitchControl(SwitchStateOrigin origin, const char *topic, const char *payload);
