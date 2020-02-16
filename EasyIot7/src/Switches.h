#ifndef SWITCHES_H
#define SWITCHES_H
#include "constants.h"
#include "Arduino.h"
#include "ArduinoJson.h"
#include "FS.h"
static const String STATES_POLL[] = {constanstsSwitch::payloadOff, constanstsSwitch::payloadOn, constanstsSwitch::payloadStop, constanstsSwitch::payloadOpen, constanstsSwitch::payloadStop, constanstsSwitch::payloadClose, constanstsSwitch::payloadReleased, constanstsSwitch::payloadUnlock, constanstsSwitch::payloadLock};
class Bounce;
enum SwitchMode
{
    SWITCH = 1,
    PUSH = 2,
    DUAL_SWITCH = 4,
    DUAL_PUSH = 5
};

enum SwitchControlType
{
    GPIO_OUTPUT = 1,
    MQTT = 2,
    KNX = 3
};

struct SwitchT
{
    double firmware;
    char id[32]; //Generated from name without spaces and no special characters
    char name[24];
    char family[10];                      //switch, light, cover, lock
    SwitchMode mode = SWITCH;             // MODE_SWITCH, MODE_PUSH, MODE_DUAL_SWITCH, MODE_DUAL_PUSH
    SwitchControlType typeControl = KNX;

    //INTEGRATIONS
    bool cloudIOSupport = true;
    bool haSupport = false;
    bool knxSupport = false;
    bool mqttSupport = true;
    
    //UTILS
    bool childLock = false;

    //GPIOS INPUT
    unsigned int primaryGpio;
    unsigned int secondaryGpio;
    bool pullup; //USE INTERNAL RESISTOR

    //GPIOS OUTPUT
    unsigned int primaryGpioControl;
    unsigned int secondaryGpioControl;
    bool inverted;

    //AUTOMATIONS
    unsigned long autoStateDelay;
    char autoStateValue[10];

    //MQTT
    char mqttCommandTopic[128];
    char mqttStateTopic[128];
    bool mqttRetain;

    //CONTROL VARIABLES
    
    int lastPercentage;
    bool lastPrimaryGpioState;
    bool lastSecondaryGpioState;
    Bounce *debouncerPrimary;
    Bounce *debouncerSecondary;
    int statePoolIdx;
    String currentState =  STATES_POLL[statePoolIdx];
    unsigned long lastChangeState;
    
    //CLOUDIO
    char mqttCloudCommandTopic[128];
    char mqttCloudStateTopic[128];
    
    //METHODS
    void load(File &file);
    void save(File &file) const;
    void updateFromJson(JsonObject doc);
    void changeState(const char *state);
    void reloadMqttTopics();
   String getCurrentState();
};
struct Switches
{
    std::vector<SwitchT> items;
    unsigned long lastChange = 0ul;
    void load(File &file);
    void save(File &file) const;
    bool remove(const char *id);
    
    size_t serializeToJson( Print &output);
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
int findPoolIdx(const char *state, unsigned int currentIdx,  const char *family);
void reloadSwitches();
#endif