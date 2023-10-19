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

enum ActuatoDriver
{
    SWITCH_PUSH = 1,
    SWITCH_GENERIC = 2,
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
    ActuatoDriver driver = SWITCH_PUSH;
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
        return  driver == COVER_DUAL_PUSH || driver == COVER_DUAL_GENERIC;
    };
    constexpr bool isLight()
    {
        return driver == LIGHT_PUSH || driver == LIGHT_GENERIC;
    };
    constexpr bool isSwitch()
    {
        return driver == SWITCH_PUSH || driver == SWITCH_GENERIC;
    };
    constexpr bool isGarage()
    {
        return driver == GARAGE_PUSH;
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
        return Family::NONE;
    };
    String driverToText()
    {
        switch (driver)
        {
        case SWITCH_PUSH:
            return FeatureDrivers::SWITCH_PUSH;
        case SWITCH_GENERIC:
            return FeatureDrivers::SWITCH_GENERIC;
        case COVER_DUAL_PUSH:
            return FeatureDrivers::COVER_DUAL_PUSH;
        case COVER_DUAL_GENERIC:
            return FeatureDrivers::COVER_DUAL_GENERIC;
        case LOCK_PUSH:
            return FeatureDrivers::LOCK_PUSH;
        case LIGHT_PUSH:
            return FeatureDrivers::LIGHT_GENERIC;
        case LIGHT_GENERIC:
            return FeatureDrivers::LIGHT_GENERIC;
        case GARAGE_PUSH:
            return FeatureDrivers::SWITCH_PUSH;
        }
        return FeatureDrivers::GENERIC;
    };
    Actuator *changeState(StateOrigin origin, int state);
    void setup();
    void notifyState(StateOrigin origin);
};
