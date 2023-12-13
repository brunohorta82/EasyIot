#pragma once
#include "Arduino.h"
#include "Constants.h"
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
enum ActuatorInputMode
{
    PUSH = 0,
    LATCH = 1,
    ROTATE = 2,
};
enum ActuatorDriver
{
    SWITCH_PUSH = 1,
    SWITCH_LATCH = 2,
    COVER_SINGLE_PUSH = 3,
    COVER_DUAL_PUSH = 4,
    COVER_DUAL_LATCH = 5,
    LIGHT_PUSH = 7,
    LIGHT_LATCH = 8,
    GARAGE_PUSH = 9,
    INVALID = 999
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
    char name[24] = {0};
    ActuatorDriver driver = SWITCH_PUSH;
    ActuatorControlType typeControl = VIRTUAL;
    int state = 0;
    int lastState = 0;
    // CLOUDIO
    char cloudIOwriteTopic[128]{};
    char cloudIOreadTopic[128]{};
    // KNX
    uint8_t knxAddress[3] = {0, 0, 0};
    int knxSync{0};
    // MQTT
    char writeTopic[128]{};
    char readTopic[128]{};
    // GPIOS INPUT
    std::vector<unsigned int> inputs;
    std::vector<Button2> buttons;

    // GPIOS OUTPUT
    std::vector<unsigned int> outputs;

    // CONTROL VARIABLES
    bool ready{false};
    int id = 0;

    // VIRTUAL COVER CONTROLLER
    Shutters *shutter;
    unsigned long upCourseTime = constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;
    unsigned long downCourseTime = constantsConfig::SHUTTER_DEFAULT_COURSE_TIME_SECONS;

    // METHODS
    constexpr bool isCover()
    {
        return driver == COVER_DUAL_PUSH || driver == COVER_DUAL_LATCH || driver == COVER_SINGLE_PUSH;
    };
    constexpr bool isLight()
    {
        return driver == LIGHT_PUSH || driver == LIGHT_LATCH;
    };

    constexpr bool isVirtual()
    {
        return typeControl == ActuatorControlType::VIRTUAL;
    };
    constexpr bool isRelay()
    {
        return typeControl == ActuatorControlType::GPIO_OUTPUT;
    }
    constexpr bool isSwitch()
    {
        return driver == SWITCH_PUSH || driver == SWITCH_LATCH;
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
    bool requireDualInputs()
    {
        return this->driver == ActuatorDriver::COVER_DUAL_LATCH || this->driver == ActuatorDriver::COVER_DUAL_PUSH;
    }
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
        case SWITCH_LATCH:
            return FeatureDrivers::SWITCH_LATCH;
        case COVER_DUAL_PUSH:
            return FeatureDrivers::COVER_DUAL_PUSH;
        case COVER_SINGLE_PUSH:
            return FeatureDrivers::COVER_SINGLE_PUSH;
        case COVER_DUAL_LATCH:
            return FeatureDrivers::COVER_DUAL_LATCH;
        case LIGHT_PUSH:
            return FeatureDrivers::LIGHT_PUSH;
        case LIGHT_LATCH:
            return FeatureDrivers::LIGHT_LATCH;
        case GARAGE_PUSH:
            return FeatureDrivers::GARAGE_PUSH;
        }
        return FeatureDrivers::INVALID;
    };
    ActuatorInputMode driverToInputMode()
    {
        switch (driver)
        {
        case SWITCH_PUSH:
        case COVER_DUAL_PUSH:
        case GARAGE_PUSH:
            return ActuatorInputMode::PUSH;
        case SWITCH_LATCH:
        case LIGHT_LATCH:
        case COVER_DUAL_LATCH:
            return ActuatorInputMode::LATCH;
        case COVER_SINGLE_PUSH:
            return ActuatorInputMode::ROTATE;
        default:
            return ActuatorInputMode::PUSH;
        }
    };
    ActuatorDriver findDriver(ActuatorInputMode inputMode)
    {
        switch (inputMode)
        {
        case PUSH:
            if (isLight())
            {
                return LIGHT_PUSH;
            }
            if (isSwitch())
            {
                return SWITCH_PUSH;
            }
            if (isGarage())
            {
                return GARAGE_PUSH;
            }
            if (isCover())
            {
                return COVER_DUAL_PUSH;
            }
        case LATCH:
            if (isLight())
            {
                return LIGHT_LATCH;
            }
            if (isSwitch())
            {
                return SWITCH_LATCH;
            }
            if (isCover())
            {
                return COVER_DUAL_LATCH;
            }
        case ROTATE:
            if (isLight())
            {
                return LIGHT_PUSH;
            }
            if (isSwitch())
            {
                return SWITCH_PUSH;
            }
            if (isCover())
            {
                return COVER_SINGLE_PUSH;
            }
            if (isGarage())
            {
                return GARAGE_PUSH;
            }
        }
        return driver;
    };

    Actuator *
    changeState(StateOrigin origin, int state);
    void setup();
    void notifyState(StateOrigin origin);
};
