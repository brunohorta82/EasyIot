#ifndef SWITCHES_H
#define SWITCHES_H

#include "Arduino.h"
#include "ArduinoJson.h"
#include "FS.h"

enum SwitchMode
{
    SWITCH = 1,
    PUSH = 2,
    DUAL_SWITCH = 4,
    DUAL_PUSH = 5
};
enum SwitchSlaveMode
{
    S_MQTT_PUB,
    S_MQTT_SUB,
    S_HTTP_POST,
    S_HTTP_GET,
};
enum SwitchControlType
{
    RELAY_AND_MQTT = 1,
    MQTT = 2
};
struct SwitchStatePool
{
    char id[32];
    char state[10];
};
struct SwitchT
{
    char id[32]; //Generated from name without spaces and no special characters
    char name[24];
    char family[10];                      //switch, light, cover, lock
    SwitchMode mode = SWITCH;             // MODE_SWITCH, MODE_PUSH, MODE_DUAL_SWITCH, MODE_DUAL_PUSH
    SwitchControlType typeControl = MQTT; //MQTT OR RELAY
    bool alexaSupport = true;
    bool haSupport = true;
    bool childLock = false;
    //GPIOS INPUT
    unsigned int primaryGpio;
    unsigned int secondaryGpio;
    bool pullup; //USE INTERNAL RESISTOR

    //GPIOS OUTPUT
    unsigned int primaryGpioControl;
    unsigned int secondaryGpioControl;
    bool inverted;

    //KNX
    uint8_t knxLevelOne;
    uint8_t knxLevelTwo;
    uint8_t knxLevelThree;
    uint8_t knxIdRegister;
    uint8_t knxIdAssign;
    bool knxNotifyGroup = true;

    //AUTOMATIONS
    unsigned long autoStateDelay;
    char autoStateValue[10];
    unsigned long timeBetweenStates;

    //MQTT
    char mqttCommandTopic[128];
    char mqttStateTopic[128];
    char mqttPositionStateTopic[128];
    char mqttPositionCommandTopic[128];
    char mqttPayload[10];
    bool mqttRetain;

    //CONTROL VARIABLES
    char stateControl[10];    //ON, OFF, STOP, CLOSE, OPEN, LOCK, UNLOCK
    int positionControlCover; //COVER PERCENTAGE 100% = open, 0% close
    int lastPercentage;
    bool lastPrimaryGpioState;
    bool lastSecondaryGpioState;
    unsigned long lastTimeChange = 0ul;
    int percentageRequest = -1;
    int statePoolIdx;
    unsigned int statePoolStart;
    unsigned int statePoolEnd;
    bool slave;

    void load(File &file);
    void save(File &file) const;
    void updateFromJson(JsonObject doc);
    void changeState(const char *state);
};
struct Switches
{
    std::vector<SwitchT> items;
    unsigned long lastChange = 0ul;
    void load(File &file);
    void save(File &file) const;
    bool remove(const char *id);
    size_t serializeToJson(Print &output);
};
void stateSwitchByName(Switches &switches, const char *name, const char *state, const char *value);
void loop(Switches &switches);
void load(Switches &switches);
void remove(Switches &switches, const char *id);
void update(Switches &switches, const String &id, JsonObject doc);
void mqttSwitchControl(Switches &switches, const char *topic, const char *payload);
void sendToServerEvents(const String &topic, const String &payload);
void stateSwitchById(Switches &switches, const char *id, const char *state);
struct Switches &getAtualSwitchesConfig();
#endif