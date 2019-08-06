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

enum SwitchControlType{
    RELAY_AND_MQTT = 1,
    MQTT = 2
};
struct SwitchT
{
    char id[32]; //Generated from name without spaces and no special characters
    char name[24];
    char family[10];   //switch, light, cover, lock
    SwitchMode mode = SWITCH; // MODE_SWITCH, MODE_PUSH, MODE_DUAL_SWITCH, MODE_DUAL_PUSH
    SwitchControlType typeControl = MQTT;   //MQTT OR RELAY
    bool alexaSupport;
    bool haSupport;
    //GPIOS INPUT
    unsigned int primaryGpio;
    unsigned int secondaryGpio;
    bool pullup; //USE INTERNAL RESISTOR

    //GPIOS OUTPUT
    unsigned int primaryGpioControl;
    unsigned int secondaryGpioControl;
    bool inverted;

    //AUTOMATIONS
    bool autoState;
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
    unsigned long lastTimeChange;
    int percentageRequest;
    int statePoolIdx;
    unsigned int statePoolStart;
    unsigned int statePoolEnd;
    void load(File& file);
    void save(File& file) const;
    void updateFromJson(JsonObject doc);
    void changeState(const char *state);
    
};
struct Switches {
  std::vector<SwitchT> items;
  unsigned long lastChange = 0ul;
  void load(File& file);
  void save(File& file) const;
  bool remove(const char* id) const;
  size_t serializeToJson(Print &output);
};
void stateSwitchByName(const char *name, const char *state, const char *value);
void loopSwitches();
void loadStoredSwitches();
void removeSwitch(const char *id);
void updateSwitch(const String& id,JsonObject doc);
void mqttSwitchControl(const char *topic, const char *payload);
void initSwitchesHaDiscovery();
void sendToServerEvents(const String& topic,const  String& payload);
void stateSwitchById(const char *id, const char *state);
struct Switches &getAtualSwitchesConfig();
#endif