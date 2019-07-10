#ifndef SWITCHES_H
#define SWITCHES_H
#include "Arduino.h"
#include "config.h"
#include "Templates.h"
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

#define MODE_BUTTON_SWITCH 1
#define MODE_BUTTON_PUSH 2
#define MODE_OPEN_CLOSE_SWITCH 4
#define MODE_OPEN_CLOSE_PUSH 5

#define SWITCHES_CONFIG_FILENAME  "switchs.json"
const String statesPool[] = {"ON", "OFF","OPEN", "STOP", "CLOSE", "STOP","LOCK","UNLOCK"};

struct SwitchT{
    char id[24];
    char name[24];
    unsigned int primaryGpio;
    unsigned int secondaryGpio;
    unsigned long timeBetweenStates;
    int percentage;
    int lastPercentage;
    
    bool lastPrimaryGpioState;
    bool lastSecondaryGpioState;
    
    unsigned long lastTimeChange;
    
    bool autoState;
    unsigned long autoStateDelay;
    char autoStateValue[10];
    int percentageRequest;
    unsigned long onTime;
    int typeControl; //MQTT OR RELAY
    
    unsigned int mode; // MODE_BUTTON_SWITCH, MODE_BUTTON_PUSH, MODE_OPEN_CLOSE_SWITCH, MODE_OPEN_CLOSE_PUSH, MODE_AUTO_OFF
    
    char stateControl[8]; //ON, OFF, STOP, CLOSE, OPEN

    char mqttCommandTopic[128];
    char mqttStateTopic[128];
    char mqttPayload[10];
    bool mqttReatain;
    bool pullup; //USE INTERNAL RESISTOR
    
    unsigned int gpioSingleControl;  
    unsigned int gpioOpenControl;
    unsigned int gpioCloseControl;
    unsigned int gpioOpenCloseControl;
    unsigned int gpioStopControl;
    unsigned int positionControlCover; //COVER PERCENTAGE
    bool inverted;
    
    int statePoolIdx;
    unsigned int statePoolStart;
    unsigned int statePoolEnd;
    
};

void loopSwitches();
void stateSwitch(SwitchT *switchT, String state);
void setupSwitchs();
void mqttSwitchControl(String topic, String payload);
String getSwitchesConfigStatus();


#endif