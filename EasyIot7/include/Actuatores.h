#pragma once
#include "Arduino.h"
#include "constants.h"
#include <Button2.h>
#include "ArduinoJson.h"
#include <vector>
#include "FS.h"
class Shutters;
enum SwitchState
{
    OFF = 0,
    ON = 1,
    OPEN = 0,
    CLOSE = 100,
    STOP = 101,
    TOGGLE = 102
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
    char uniqueId[24]{};
    int sequence = 0;
    char name[24] = {0};
    SwitchFamily family = SWITCH_PUSH;
    SwitchControlType typeControl = VIRTUAL;
    int state = 0;
    // CLOUDIO
    char cloudIOwriteTopic[128]{};
    char cloudIOreadTopic[128]{};
    // KNX
    uint8_t knxAddress[3] = {0, 0, 0};
    uint8_t knxIdRegister = 0;
    uint8_t knxIdAssign = 0;
    // MQTT
    char writeTopic[128]{};
    char readTopic[128]{};
    // GPIOS INPUT
    std::vector<unsigned int> inputs;
    std::vector<Button2> buttons;

    // GPIOS OUTPUT
    std::vector<unsigned int> outputs;

    // CONTROL VARIABLES
    int lastInputEvent = 0;
    int currentInputEvent = 0;
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
    String familyToText()
    {
        if (isLight())
            return "light";
        if (isCover())
            return "cover";
        if (isGarage())
            return "garage";
        if (isSwitch())
            return "switch";
        return "generic";
    };
    ActuatorT *changeState(SwitchStateOrigin origin, int state);
    void setup();
    void notifyState(SwitchStateOrigin origin);
};
