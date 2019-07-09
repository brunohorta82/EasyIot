#include "Switches.h"
#include "Mqtt.h"

std::vector<SwitchT> switchs;

void saveSwitchs(){
  
}
void setupSwitchs(){
  struct SwitchT s1;
  s1.gpio = 12;
  pinMode(s1.gpio,INPUT_PULLUP);
  s1.mode = MODE_BUTTON_SWITCH;
  strlcpy(s1.name, "Interruptor", sizeof(s1.name));
  strlcpy(s1.mqttCommandTopic, "cenas/set", sizeof(s1.mqttCommandTopic));  
  strlcpy(s1.mqttStateTopic, "cenas/state", sizeof(s1.mqttStateTopic));
  s1.mqttReatain = true;
  s1.gpioSingleControl = 5;
  s1.inverted = false;
  s1.typeControl = TYPE_RELAY;
  s1.statePoolStart = 0;
  s1.statePoolEnd = 1;
  subscribeOnMqtt(s1.mqttCommandTopic);
  pinMode(s1.gpioSingleControl,OUTPUT);
  switchs.push_back(s1);
}
void mqttSwitchControl(String topic, String payload){
  for (unsigned int i = 0; i < switchs.size(); i++){
    if(strcmp(switchs[i].mqttCommandTopic,topic.c_str()) == 0){
       for (unsigned int p = switchs[i].statePoolStart  ; p <= switchs[i].statePoolEnd; p++){
        if(strcmp(payload.c_str(),statesPool[p].c_str()) == 0){
          switchs[i].statePoolIdx = p;
          stateSwitch(&switchs[i],statesPool[p]);
          break;    
    }
  }
  }
  }
}

void stateSwitch(SwitchT *switchT, String state)
{
  
   if (String(PAYLOAD_OPEN).equals(state))
    {
      switchT->positionControlCover = 100;
      strlcpy(switchT->stateControl, PAYLOAD_OPEN, sizeof(switchT->stateControl));
      strlcpy(switchT->mqttPayload, PAYLOAD_STATE_OPEN, sizeof(switchT->mqttPayload));
      if(switchT->typeControl == TYPE_RELAY){
        pinMode(switchT->gpioStopControl,OUTPUT);
        pinMode(switchT->gpioOpenCloseControl,OUTPUT);
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
      if(switchT->typeControl == TYPE_RELAY){
        pinMode(switchT->gpioStopControl,OUTPUT);
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
       if(switchT->typeControl == TYPE_RELAY){
        pinMode(switchT->gpioStopControl,OUTPUT);
        pinMode(switchT->gpioOpenCloseControl,OUTPUT);
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
      if(switchT->typeControl == TYPE_RELAY){
        logger(SWITCHES_TAG,switchT->name);
        logger(SWITCHES_TAG,state);
        logger(SWITCHES_TAG,String(switchT->gpioSingleControl));
        pinMode(switchT->gpioSingleControl,OUTPUT);
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
      }
    }
    else if (String(PAYLOAD_OFF).equals(state))
    {
      strlcpy(switchT->stateControl, PAYLOAD_OFF, sizeof(switchT->stateControl));
      strlcpy(switchT->mqttPayload, PAYLOAD_OFF, sizeof(switchT->mqttPayload));
      if(switchT->typeControl == TYPE_RELAY){
                logger(SWITCHES_TAG,switchT->name);
        logger(SWITCHES_TAG,state);
        logger(SWITCHES_TAG,String(switchT->gpioSingleControl));
         pinMode(switchT->gpioSingleControl,OUTPUT);
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? HIGH : LOW); //TURN OFF
      }
    }
    else if (String(PAYLOAD_LOCK).equals(state))
    {
      strlcpy(switchT->stateControl, PAYLOAD_LOCK, sizeof(switchT->stateControl));
      strlcpy(switchT->mqttPayload, PAYLOAD_STATE_LOCK, sizeof(switchT->mqttPayload));
      if(switchT->typeControl == TYPE_RELAY){
         pinMode(switchT->gpioSingleControl,OUTPUT);
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
        delay(500);
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN OFF
      }
    }
    else if (String(PAYLOAD_UNLOCK).equals(state))
    {
      strlcpy(switchT->stateControl, PAYLOAD_UNLOCK, sizeof(switchT->stateControl));
      strlcpy(switchT->mqttPayload, PAYLOAD_STATE_UNLOCK, sizeof(switchT->mqttPayload));
      if(switchT->typeControl == TYPE_RELAY){
        pinMode(switchT->gpioSingleControl,OUTPUT);
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
        delay(500);
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN OFF
      }
    }
    publishOnMqtt(switchT->mqttStateTopic,switchT->mqttPayload,switchT->mqttReatain);
}
void loopSwitches()
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
   
    SwitchT *sw =  &switchs[i];
    bool value = digitalRead(sw->gpio);
    unsigned long currentTime = millis();
    if(value != sw->lastGpioState && currentTime - sw->lastTimeChange >= 5){
    
    sw->lastGpioState = value;
    sw->lastTimeChange = currentTime;
   switch (sw->mode)
   {
   case MODE_BUTTON_SWITCH:
   case MODE_OPEN_CLOSE_SWITCH:
     sw->statePoolIdx = (sw->statePoolIdx + 1 + sw->statePoolStart)  %  ((sw->statePoolEnd - sw->statePoolStart) +1);
     stateSwitch(sw,statesPool[sw->statePoolIdx]);
     Serial.println( sw->statePoolIdx);
     break;
        case MODE_BUTTON_PUSH:
        case MODE_OPEN_CLOSE_PUSH:
        if(!value){ //PUSHED
          sw->statePoolIdx = (sw->statePoolIdx + 1 + sw->statePoolStart)  %  ((sw->statePoolEnd - sw->statePoolStart) +1);
          stateSwitch(sw,statesPool[sw->statePoolIdx]);
        }
     break;
   default:
     break;
   }
   if(sw-> mode == MODE_AUTO_OFF && strcmp(PAYLOAD_ON,sw->stateControl) == 0 && (sw->lastTimeChange + sw->autoOffDelay)  < millis()){
     stateSwitch(sw,PAYLOAD_OFF);
   }
  }
  }
}
