#include "Switches.h"
#include "Mqtt.h"

std::vector<SwitchT> switchs;

void updateSwitches(JsonObject doc, bool persist){
    
    
}
void loadStoredSwitchs()
{
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(CONFIG_FILENAME, "r+");
    const size_t CAPACITY = JSON_OBJECT_SIZE(24) + 512;
    DynamicJsonDocument doc(CAPACITY);
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
      logger(SWITCHES_TAG, "Default switches loaded.");
    }
    else
    {
      logger(SWITCHES_TAG, "Stored switches loaded.");
    }
    file.close();
    updateSwitches(doc.as<JsonObject>(),error);
    
   
  }
  SPIFFS.end();   
}

void saveSwitchs()
{
}
String getSwitchesConfigStatus(){
  String object = "";
  if (SPIFFS.begin())
  {
    File file = SPIFFS.open(SWITCHES_CONFIG_FILENAME, "r+");

 if (!file) {
    Serial.println(F("Failed to read file"));
    return "[]";
  }
  while (file.available()) {
    object += (char)file.read();
  }
  Serial.println();
    file.close();
  }
  SPIFFS.end();
  return object;
}
int findPoolIdx(String state, unsigned int start, unsigned int end)
{
  for (unsigned int p = start; p <= end; p++)
  {
    if (strcmp(state.c_str(), statesPool[p].c_str()) == 0)
    {
      return p;
      break;
    }
  }
  return start;
}
void setupSwitchs()
{
  struct SwitchT s1;
  s1.primaryGpio = 12;
  s1.secondaryGpio = 13;
  pinMode(s1.primaryGpio, INPUT_PULLUP);
  pinMode(s1.secondaryGpio, INPUT_PULLUP);
  s1.mode = MODE_BUTTON_PUSH;
  s1.autoState = true;
  strlcpy(s1.autoStateValue, "OFF", sizeof(s1.autoStateValue));
  s1.autoStateDelay = 1000;
  s1.timeBetweenStates = 9000;
  strlcpy(s1.name, "Interruptor", sizeof(s1.name));
  strlcpy(s1.mqttCommandTopic, "cenas/set", sizeof(s1.mqttCommandTopic));
  strlcpy(s1.mqttStateTopic, "cenas/state", sizeof(s1.mqttStateTopic));
  s1.mqttReatain = true;
  s1.gpioOpenCloseControl = 5;
  s1.gpioStopControl = 4;
  s1.inverted = false;
  s1.typeControl = TYPE_RELAY;
  s1.onTime = 0;
  s1.statePoolStart = 0;
  s1.statePoolEnd = 1;
  s1.percentage = 0;
  s1.percentageRequest = INT_MAX;
  subscribeOnMqtt(s1.mqttCommandTopic);
  pinMode(s1.gpioSingleControl, OUTPUT);

  //STORED STATES
  s1.statePoolIdx = s1.statePoolStart;
  s1.lastPrimaryGpioState = true;
  s1.lastPercentage = 0;
  s1.lastSecondaryGpioState = true;
  switchs.push_back(s1);
}
void mqttSwitchControl(String topic, String payload)
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
    if (strcmp(switchs[i].mqttCommandTopic, topic.c_str()) == 0)
    {

      for (unsigned int p = switchs[i].statePoolStart; p <= switchs[i].statePoolEnd; p++)
      {
        if (strcmp(payload.c_str(), statesPool[p].c_str()) == 0)
        {
          switchs[i].statePoolIdx = p;
          stateSwitch(&switchs[i], statesPool[p]);
          return;
        }
      }
      stateSwitch(&switchs[i], payload);
    }
  }
}
boolean isValidNumber(String str)
{
  for (byte i = 0; i < str.length(); i++)
  {
    if (isDigit(str.charAt(i)))
      return true;
  }
  return false;
}
void stateSwitch(SwitchT *switchT, String state)
{
  logger(SWITCHES_TAG, switchT->name);
  logger(SWITCHES_TAG, state);
  logger(SWITCHES_TAG, String(switchT->statePoolIdx));

  if (String(PAYLOAD_OPEN).equals(state))
  {
    switchT->positionControlCover = 100;
    strlcpy(switchT->stateControl, PAYLOAD_OPEN, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_OPEN, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      pinMode(switchT->gpioStopControl, OUTPUT);
      pinMode(switchT->gpioOpenCloseControl, OUTPUT);
      delay(DELAY_COVER_PROTECTION);
      digitalWrite(switchT->gpioStopControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(DELAY_COVER_PROTECTION);
      digitalWrite(switchT->gpioOpenCloseControl, switchT->inverted ? LOW : HIGH); //TURN ON -> OPEN REQUEST
      delay(DELAY_COVER_PROTECTION);
      digitalWrite(switchT->gpioStopControl, switchT->inverted ? LOW : HIGH); //TURN ON -> EXECUTE REQUEST
    }
  }
  else if (String(PAYLOAD_STOP).equals(state))
  {
    switchT->positionControlCover = 50;
    strlcpy(switchT->stateControl, PAYLOAD_STOP, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_STOP, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      pinMode(switchT->gpioStopControl, OUTPUT);
      delay(DELAY_COVER_PROTECTION);
      digitalWrite(switchT->gpioStopControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(DELAY_COVER_PROTECTION);
    }
  }
  else if (String(PAYLOAD_CLOSE).equals(state))
  {

    switchT->positionControlCover = 0;
    strlcpy(switchT->stateControl, PAYLOAD_CLOSE, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_CLOSE, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      pinMode(switchT->gpioStopControl, OUTPUT);
      pinMode(switchT->gpioOpenCloseControl, OUTPUT);
      delay(DELAY_COVER_PROTECTION);
      digitalWrite(switchT->gpioStopControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> STOP
      delay(DELAY_COVER_PROTECTION);
      digitalWrite(switchT->gpioOpenCloseControl, switchT->inverted ? HIGH : LOW); //TURN OFF -> CLOSE REQUEST
      delay(DELAY_COVER_PROTECTION);
      digitalWrite(switchT->gpioStopControl, switchT->inverted ? LOW : HIGH); //TURN ON -> EXECUTE REQUEST
    }
  }
  else if (String(PAYLOAD_ON).equals(state))
  {

    strlcpy(switchT->stateControl, PAYLOAD_ON, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_ON, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {

      pinMode(switchT->gpioSingleControl, OUTPUT);
      digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
    }
  }
  else if (String(PAYLOAD_OFF).equals(state))
  {
    strlcpy(switchT->stateControl, PAYLOAD_OFF, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_OFF, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      logger(SWITCHES_TAG, switchT->name);
      logger(SWITCHES_TAG, state);
      logger(SWITCHES_TAG, String(switchT->gpioSingleControl));
      pinMode(switchT->gpioSingleControl, OUTPUT);
      digitalWrite(switchT->gpioSingleControl, switchT->inverted ? HIGH : LOW); //TURN OFF
    }
  }
  else if (String(PAYLOAD_LOCK).equals(state))
  {
    strlcpy(switchT->stateControl, PAYLOAD_LOCK, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_LOCK, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      pinMode(switchT->gpioSingleControl, OUTPUT);
      digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
     
    }
  }
  else if (String(PAYLOAD_UNLOCK).equals(state))
  {
    strlcpy(switchT->stateControl, PAYLOAD_UNLOCK, sizeof(switchT->stateControl));
    strlcpy(switchT->mqttPayload, PAYLOAD_STATE_UNLOCK, sizeof(switchT->mqttPayload));
    if (switchT->typeControl == TYPE_RELAY)
    {
      pinMode(switchT->gpioSingleControl, OUTPUT);
      digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
    
    }
  }
  else if (isValidNumber(state))
  {
    switchT->percentageRequest = state.toInt();
    if (switchT->percentage > switchT->percentageRequest)
    {
      switchT->statePoolIdx = 2;
      stateSwitch(switchT, statesPool[switchT->statePoolIdx]);
    }
    else
    {
      switchT->statePoolIdx = 4;
      stateSwitch(switchT, statesPool[switchT->statePoolIdx]);
    }
  }
  publishOnMqtt(switchT->mqttStateTopic, switchT->mqttPayload, switchT->mqttReatain);
  switchT->lastTimeChange = millis();
}
bool stateTimeout(SwitchT *sw){
return (sw->autoState && strcmp(sw->autoStateValue, sw->stateControl) != 0 && (sw->lastTimeChange + sw->autoStateDelay) < millis());
}
boolean positionDone(SwitchT *sw, int currentPercentage){
return ((sw->percentage == 0 || sw->percentage == 100 || sw->percentageRequest == sw->percentage) && currentPercentage > 0 && strcmp(sw->autoStateValue, sw->stateControl) != 0);
}
void loopSwitches()
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {

    SwitchT *sw = &switchs[i];
    bool primaryValue = digitalRead(sw->primaryGpio);
    bool secondaryValue = digitalRead(sw->secondaryGpio);
    unsigned long currentTime = millis();

    bool primaryGpioEvent = primaryValue != sw->lastPrimaryGpioState;
    bool secondaryGpioEvent = secondaryValue != sw->lastSecondaryGpioState;

    if ((primaryGpioEvent || secondaryGpioEvent) && currentTime - sw->lastTimeChange >= 5)
    {
      sw->lastPrimaryGpioState = primaryValue;
      sw->lastSecondaryGpioState = secondaryValue;
      sw->lastTimeChange = currentTime;

      switch (sw->mode)
      {
      case MODE_BUTTON_SWITCH:
        sw->statePoolIdx = (sw->statePoolIdx + 1 + sw->statePoolStart) % ((sw->statePoolEnd - sw->statePoolStart) + 1);
        stateSwitch(sw, statesPool[sw->statePoolIdx]);
        break;
      case MODE_BUTTON_PUSH:
      case MODE_OPEN_CLOSE_PUSH:
        if (!primaryValue)
        { //PUSHED
          sw->statePoolIdx = (sw->statePoolIdx + 1 + sw->statePoolStart) % ((sw->statePoolEnd - sw->statePoolStart) + 1);
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        break;
      case MODE_OPEN_CLOSE_SWITCH:
        if (primaryValue == true && secondaryValue == true)
        {
          sw->statePoolIdx = sw->statePoolIdx == 2 ? 3 : 5;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        else if (primaryGpioEvent && !primaryValue)
        {
          sw->statePoolIdx = 2;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        else if (secondaryGpioEvent && !secondaryValue)
        {
          sw->statePoolIdx = 4;
          stateSwitch(sw, statesPool[sw->statePoolIdx]);
        }
        break;
      default:
        break;
      }
    }

    int currentPercentage = 0;
    if (digitalRead(sw->gpioStopControl))
    {
      if (sw->onTime == 0)
      {
        sw->onTime = millis();
      }
      long currentOffset = sw->timeBetweenStates - millis() - sw->onTime;
      int position = max(0l, (long)(sw->timeBetweenStates - currentOffset));
      currentPercentage = (position * 100) / sw->timeBetweenStates;
      if (digitalRead(sw->gpioOpenCloseControl))
      {
        sw->percentage = max(0, sw->lastPercentage - currentPercentage);
      }
      else
      {
        sw->percentage = min(100, currentPercentage + sw->lastPercentage);
      }
    }
    else
    {
      sw->onTime = 0;
      sw->lastPercentage = sw->percentage;
    }

    if (stateTimeout(sw) || positionDone(sw,currentPercentage))
    {
      sw->statePoolIdx = findPoolIdx(sw->autoStateValue, sw->statePoolIdx, sw->statePoolEnd);
      sw->percentageRequest = INT_MAX;
      stateSwitch(sw, statesPool[sw->statePoolIdx]);
    }
  }
}
