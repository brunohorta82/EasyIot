#ifndef SWITCHES_H
#define SWITCHES_H
#include "Arduino.h"
#include "config.h"
#include "Templates.h"
#include "WebServer.h"
#define SWITCHES_TAG "[SWITCHES]"
#define SWITCH_DEVICE "switch"

#define DELAY_COVER_PROTECTION 50 //50 milliseconds

#define PAYLOAD_ON "ON"
#define PAYLOAD_OFF "OFF"
#define PAYLOAD_CLOSE "CLOSE"
#define PAYLOAD_STATE_CLOSE "closed"
#define PAYLOAD_OPEN "OPEN"
#define PAYLOAD_STATE_OPEN "open"
#define PAYLOAD_STOP "STOP"
#define PAYLOAD_STATE_STOP ""
#define PAYLOAD_LOCK "LOCK"
#define PAYLOAD_STATE_LOCK "LOCK"
#define PAYLOAD_UNLOCK "UNLOCK"
#define PAYLOAD_STATE_UNLOCK "UNLOCK"

#define TYPE_RELAY 1
#define TYPE_MQTT 2

#define MODE_SWITCH 1
#define MODE_PUSH 2
#define MODE_DUAL_SWITCH 4
#define MODE_DUAL_PUSH 5

#define SWITCHES_CONFIG_FILENAME  "switchs.json"
const String statesPool[] = {"OFF", "ON","STOP","OPEN", "STOP", "CLOSE","LOCK","UNLOCK"};

struct SwitchT{
    char id[32]; //Generated from name without spaces and no special characters
    char name[24];
    char family[10]; //switch, cover
    unsigned int mode; // MODE_SWITCH, MODE_PUSH, MODE_DUAL_SWITCH, MODE_DUAL_PUSH
    int typeControl; //MQTT OR RELAY
    
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
    char stateControl[8]; //ON, OFF, STOP, CLOSE, OPEN, LOCK, UNLOCK
    int positionControlCover; //COVER PERCENTAGE
    int lastPercentage;
    bool lastPrimaryGpioState;
    bool lastSecondaryGpioState;
    unsigned long lastTimeChange;
    int percentageRequest;
    unsigned long onTime;
    int statePoolIdx;
    unsigned int statePoolStart;
    unsigned int statePoolEnd;
    
};

void loopSwitches();
void stateSwitch(SwitchT *switchT, String state);
void loadStoredSwitchs();
void saveSwitchs();
void removeSwitch(String id, bool persist);
void updateSwitches(JsonObject doc, bool persist);
void mqttSwitchControl(String topic, String payload);
void initSwitchesMqttAndDiscovery();
void sendToServerEvents(String topic, String payload);
void stateSwitchById(String id, String state);
String getSwitchesConfigStatus();


#endif