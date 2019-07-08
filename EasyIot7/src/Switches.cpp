#include "Switches.h"

std::vector<SwitchT> switchs;

void saveSwitchs(){
  
}

void stateSwitch(SwitchT *switchT, String state)
{

   if (String(PAYLOAD_OPEN).equals(state))
    {
      switchT->positionControlCover = 100;
      strlcpy(switchT->stateControl, PAYLOAD_OPEN, sizeof(switchT->stateControl));
      strlcpy(switchT->mqttPayload, PAYLOAD_STATE_OPEN, sizeof(switchT->mqttPayload));
      if(switchT->typeControl == TYPE_RELAY){
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
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
      }
    }
    else if (String(PAYLOAD_OFF).equals(state))
    {
      strlcpy(switchT->stateControl, PAYLOAD_OFF, sizeof(switchT->stateControl));
      strlcpy(switchT->mqttPayload, PAYLOAD_OFF, sizeof(switchT->mqttPayload));
      if(switchT->typeControl == TYPE_RELAY){
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? HIGH : LOW); //TURN OFF
      }
    }
    else if (String(PAYLOAD_LOCK).equals(state))
    {
      strlcpy(switchT->stateControl, PAYLOAD_LOCK, sizeof(switchT->stateControl));
      strlcpy(switchT->mqttPayload, PAYLOAD_STATE_LOCK, sizeof(switchT->mqttPayload));
      if(switchT->typeControl == TYPE_RELAY){
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
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN ON
        delay(500);
        digitalWrite(switchT->gpioSingleControl, switchT->inverted ? LOW : HIGH); //TURN OFF
      }
    } 
}
void loopSwitches()
{
  for (unsigned int i = 0; i < switchs.size(); i++)
  {
    Bounce *b = _switchs[i].debouncer;
    b->update();
    bool value = b->read();
   SwitchT *sw =  &switchs[i];
   bool value;
   unsigned int gpio;
   if(value != sw->lastGpioState){
    sw->lastGpioState = value;
    sw->lastTimeChange = millis();
   switch (sw->mode)
   {
   case MODE_BUTTON_SWITCH:
   case MODE_OPEN_CLOSE_SWITCH:
     stateSwitch(sw,sw->statesPool[sw->statePoolIdx]);
     sw->statePoolIdx = sw->statePoolIdx + 1  % sw->statesPoolSize;
     break;
        case MODE_BUTTON_PUSH:
        case MODE_OPEN_CLOSE_PUSH:
        if(!value){ //PUSHED
          stateSwitch(sw,statesPoolSwitch[sw->statePoolIdx]);
          sw->statePoolIdx = sw->statePoolIdx + 1  % statesPoolSwitchSize;
        }
     break;
   default:
     break;
   }
   if(sw-> mode == MODE_AUTO_OFF && sw->lastTimeChange + sw->autoOffDelay  < millis()){
     stateSwitch(sw,PAYLOAD_OFF);
   }
  }
  
  }
}
