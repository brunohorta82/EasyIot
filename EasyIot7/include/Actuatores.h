#pragma once
#include "Arduino.h"
#include "Constants.hpp"
#include <Button2.h>
#include "ArduinoJson.h"
#include <vector>
#include "FS.h"
class Shutters;
enum ActuatorState
{
    OFF_OPEN = 0,
    ON_CLOSE = 100,
    STOP = 101,
    TOGGLE = 102
};
enum StateOrigin
{
    INTERNAL = 0,
    GPIO_INPUT = 1,
    KNX = 2,
    MQTT = 3,
    CLOUDIO = 4,
    WEBPANEL = 5
};

enum Actuatorype
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

enum ActuatorControlType
{
    GPIO_OUTPUT = 1,
    VIRTUAL = 2
};

class Actuator
{
public:
    // CONFIG
    char uniqueId[24]{};
    int sequence = 0;
    char name[24] = {0};
    Actuatorype type = SWITCH_PUSH;
    ActuatorControlType typeControl = VIRTUAL;
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
    bool lastPrimaryGpioState = true;
    bool lastSecondaryGpioState = true;
    bool lastPrimaryStateGpioState = true;
    bool lastSecondaryStateGpioState = true;

    // VIRTUAL COVER CONTROLLER
    Shutters *shutter;
    unsigned long upCourseTime = 25 * 1000;
    unsigned long downCourseTime = 25 * 1000;

    // METHODS
    constexpr bool isCover()
    {
        return type == COVER_PUSH || type == COVER_DUAL_PUSH || type == COVER_DUAL_GENERIC;
    };
    constexpr bool isLight()
    {
        return type == LIGHT_PUSH || type == LIGHT_GENERIC;
    };
    constexpr bool isSwitch()
    {
        return type == SWITCH_PUSH || type == SWITCH_GENERIC;
    };
    constexpr bool isGarage()
    {
        return type == GARAGE_PUSH;
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
            return Family::LIGTH;
        if (isCover())
            return Family::CLIMATE;
        if (isGarage())
            return Family::SECURITY;
        if (isSwitch())
            return Family::SWITCH;
        return Family::GENERIC;
    };
    String typeToText()
    {
        switch (type)
        {
        case SWITCH_PUSH:
            return FeatureTypes::SWITCH_PUSH;
        case SWITCH_GENERIC:
            return FeatureTypes::SWITCH_GENERIC;
        case COVER_PUSH:
            return FeatureTypes::COVER_PUSH;
        case COVER_DUAL_PUSH:
            return FeatureTypes::COVER_DUAL_PUSH;
        case COVER_DUAL_GENERIC:
            return FeatureTypes::COVER_DUAL_GENERIC;
        case LOCK_PUSH:
            return FeatureTypes::LOCK_PUSH;
        case LIGHT_PUSH:
            return FeatureTypes::LIGHT_GENERIC;
        case LIGHT_GENERIC:
            return FeatureTypes::LIGHT_GENERIC;
        case GARAGE_PUSH:
            return FeatureTypes::SWITCH_PUSH;
        }
        return FeatureTypes::GENERIC;
    };
    Actuator *changeState(StateOrigin origin, int state);
    void setup();
    void notifyState(StateOrigin origin);
};
