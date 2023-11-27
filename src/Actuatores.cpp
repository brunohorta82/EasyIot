#include "Actuatores.h"
#include <Shutters.h>
#include "HomeAssistantMqttDiscovery.h"
#include "WebServer.h"
#include "Constants.hpp"
#include "ConfigOnofre.h"
#include "Mqtt.h"
#include "CoreWiFi.h"
#include <esp-knx-ip.h>
#include "CloudIO.h"
#include "LittleFS.h"
#include <time.h>
extern ConfigOnofre config;

void shuttersOperationHandler(Shutters *s, ShuttersOperation operation)
{
  switch (operation)
  {
  case ShuttersOperation::UP:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Operation:      %s" CR, tags::actuatores, "UP");
#endif
#ifdef ESP32
    writeToPIN(s->downPin, LOW);
    delay(1);
    writeToPIN(s->upPin, HIGH);
#else
    writeToPIN(s->downPin, HIGH);
    delay(1);
    writeToPIN(s->upPin, HIGH);
#endif
    break;
  case ShuttersOperation::DOWN:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Operation:      %s" CR, tags::actuatores, "DOWN");
#endif
    writeToPIN(s->downPin, HIGH);
    delay(1);
    writeToPIN(s->upPin, LOW);
    break;
  case ShuttersOperation::HALT:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Operation:      %s" CR, tags::actuatores, "STOP");
#endif
#ifdef ESP32
    writeToPIN(s->downPin, LOW);
    delay(1);
    writeToPIN(s->upPin, LOW);
#else
    writeToPIN(s->downPin, LOW);
#endif
    break;
  }
#ifdef DEBUG_ONOFRE
  Log.notice("%s upPin:          %d" CR, tags::actuatores, s->upPin);
  Log.notice("%s downPin :       %d" CR, tags::actuatores, s->downPin);
#endif
}

void onShuttersLevelReached(Shutters *shutters, uint8_t level)
{
  for (auto &a : config.actuatores)
  {
    if (a.sequence == shutters->actuatorId)
    {
      a.state = level;
      a.notifyState(StateOrigin::INTERNAL);
    }
  }
}
void actuatoresCallback(message_t const &msg, void *arg)
{
  int id = *(int *)arg;
  for (auto &s : config.actuatores)
  {
    if (s.sequence == id)
    {
      switch (msg.ct)
      {
      case KNX_CT_ADC_ANSWER:
      case KNX_CT_ADC_READ:
      case KNX_CT_ESCAPE:
      case KNX_CT_INDIVIDUAL_ADDR_REQUEST:
      case KNX_CT_INDIVIDUAL_ADDR_RESPONSE:
      case KNX_CT_INDIVIDUAL_ADDR_WRITE:
      case KNX_CT_MASK_VERSION_READ:
      case KNX_CT_MASK_VERSION_RESPONSE:
      case KNX_CT_MEM_ANSWER:
      case KNX_CT_MEM_READ:
      case KNX_CT_MEM_WRITE:
      case KNX_CT_READ:
        knx.answer_1byte_int(msg.received_on, s.state);
        break;
      case KNX_CT_RESTART:
        break;
      case KNX_CT_ANSWER:
      case KNX_CT_WRITE:
      {
#ifdef DEBUG_ONOFRE
        Log.notice("%s KNX %s" CR, tags::actuatores, KNX_CT_ANSWER == msg.ct ? "SYNC" : "WRITE");
#endif
        int stateIdx = knx.data_to_1byte_int(msg.data);
        s.changeState(StateOrigin::KNX, stateIdx);
        config.save();
        break;
      }
      };
    }
  }
}
void toogle(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::TOGGLE);
    }
  }
}
void openShutter(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::OFF_OPEN);
    }
  }
}
void openShutterLatch(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::OFF_OPEN);
    }
  }
}
void stopShutterLatch(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      if ((digitalRead(a.inputs[0]) && digitalRead(a.inputs[1])))
      {
        config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::STOP);
      }
    }
  }
}
void closeShutterLatch(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::ON_CLOSE);
    }
  }
}
void closeShutter(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::ON_CLOSE);
    }
  }
}
void stopShutter(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::STOP);
    }
  }
}
void garageNotify(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {

      a.state = digitalRead(btn.getPin()) ? OFF_OPEN : ON_CLOSE;
      a.notifyState(StateOrigin::GPIO_INPUT);
    }
  }
}

void Actuator::setup()
{
  buttons.clear();
  if (isCover() && outputs.size() == 2)
  {
    shutter = new Shutters(outputs[0], outputs[1], sequence);
    shutter->setOperationHandler(shuttersOperationHandler)
        .restoreState(state, upCourseTime * 1000, downCourseTime * 1000)
        .setCourseTime(upCourseTime * 1000, downCourseTime * 1000)
        .onLevelReached(onShuttersLevelReached)
        .begin();
  }

  for (auto output : outputs)
  {
    configPIN(output, OUTPUT);
    writeToPIN(output, 0);
    if (isLight() || isSwitch())
    {
      writeToPIN(output, state);
    }
  }
  if (isLight() || isSwitch())
  {
    for (auto input : inputs)
    {
      Button2 button;

      switch (driver)
      {
      case ActuatorDriver::LIGHT_PUSH:
      case ActuatorDriver::SWITCH_PUSH:
      case ActuatorDriver::COVER_SINGLE_PUSH:
        button.begin(input);
        button.setID(sequence);
        button.setPressedHandler(toogle);
        break;
      case ActuatorDriver::LIGHT_LATCH:
      case ActuatorDriver::SWITCH_LATCH:
        button.begin(input, INPUT_PULLUP, state);
        button.setID(sequence);
        button.setChangedHandler(toogle);
      }
      buttons.push_back(button);
    }
  }
  else if (isGarage())
  {
    Button2 button;
    button.begin(inputs[0]);
    button.setID(sequence);
    button.setDebounceTime(1000);
    button.setChangedHandler(garageNotify);
    buttons.push_back(button);
    state = digitalRead(inputs[0]) ? OFF_OPEN : ON_CLOSE;
  }
  else if (isCover())
  {
    if (inputs.size() == 2)
    {
      Button2 buttonOpen;
      buttonOpen.begin(inputs[0]);
      buttonOpen.setID(sequence);
      if (ActuatorDriver::COVER_DUAL_PUSH == driver)
      {
        buttonOpen.setPressedHandler(openShutter);
        buttonOpen.setLongClickTime(500);
        buttonOpen.setLongClickDetectedHandler(stopShutter);
        buttonOpen.setDoubleClickHandler(stopShutter);
      }
      else if (ActuatorDriver::COVER_DUAL_LATCH == driver)
      {
        buttonOpen.setChangedHandler(stopShutterLatch);
        buttonOpen.setPressedHandler(openShutter);
      }

      buttons.push_back(buttonOpen);
      Button2 buttonClose;
      buttonClose.begin(inputs[1]);
      buttonClose.setID(sequence);
      if (ActuatorDriver::COVER_DUAL_PUSH == driver)
      {
        buttonClose.setLongClickTime(500);
        buttonClose.setLongClickDetectedHandler(stopShutter);
        buttonClose.setDoubleClickHandler(stopShutter);
        buttonClose.setPressedHandler(closeShutter);
      }
      else if (ActuatorDriver::COVER_DUAL_LATCH == driver)
      {
        buttonClose.setChangedHandler(stopShutterLatch);
        buttonClose.setPressedHandler(closeShutter);
      }
      buttons.push_back(buttonClose);
    }
  }

  if (isKnxSupport())
  {
    knx.callback_unassign(knxIdAssign);
    knx.callback_deregister(knxIdRegister);
    static int ids = sequence;
    knxIdRegister = knx.callback_register(String(name), actuatoresCallback, &ids);
    knxIdAssign = knx.callback_assign(knxIdRegister, knx.GA_to_address(knxAddress[0], knxAddress[1], knxAddress[2]));
    knx.callback_assign(knxIdRegister, knx.GA_to_address(knxAddress[0], knxAddress[1], 0));
    knx.callback_assign(knxIdRegister, knx.GA_to_address(knxAddress[0], 0, 0));
  }
}

void Actuator::notifyState(StateOrigin origin)
{
  String stateStr = String(state);
  // Notify by MQTT/Homeassistant
  if (mqttConnected())
  {
    publishOnMqtt(readTopic, stateStr.c_str(), true);
  }

  // Notify by MQTT OnofreCloud
  if (cloudIOConnected())
  {
    notifyStateToCloudIO(cloudIOreadTopic, stateStr.c_str());
  }

  // Notify by SSW Webpanel
  sendToServerEvents(uniqueId, stateStr.c_str());

  // Notify by KNX
  if (StateOrigin::INTERNAL != origin && StateOrigin::KNX != origin && isKnxSupport())
  {
    knx.write_1byte_int(knx.GA_to_address(knxAddress[0], knxAddress[1], knxAddress[2]), state);
  }
}

Actuator *Actuator::changeState(StateOrigin origin, int state)
{
  if (!isGarage() && this->state == state)
    return this;
#ifdef DEBUG_ONOFRE
  Log.notice("%s Name:      %s" CR, tags::actuatores, name);
  Log.notice("%s State:     %d" CR, tags::actuatores, state);
  Log.notice("%s From : %d" CR, tags::actuatores, origin);
  Log.notice("%s Family : %d" CR, tags::actuatores, driver);
#endif
  if (typeControl == ActuatorControlType::GPIO_OUTPUT && outputs.size() > 0)
  {
    if (isCover())
    {
      int level = state;
      if (level == ActuatorState::STOP)
        shutter->stop();
      else
        shutter->setLevel(max(0, min(100, level)));
    }
    else if (isGarage())
    {
      writeToPIN(outputs[0], HIGH);
      delay(1000);
      writeToPIN(outputs[0], LOW);
    }
    else if ((isLight() || isSwitch()))
    {
      if (state == ActuatorState::ON_CLOSE || state == ActuatorState::OFF_OPEN)
        writeToPIN(outputs[0], state ? HIGH : LOW);
      else
        return this;
    }
  }
  else if (typeControl == ActuatorControlType::VIRTUAL)
  {
  }
  else
  {
    return this;
  }
  this->state = state;
  this->notifyState(origin);
  /*TODO if (isKnxGroup())
   {
     for (auto &sw : config.actuatores)
     {
       if (strcmp(sw.uniqueId, uniqueId) != 0 && (sw.knxAddress[0] == knxAddress[0] || (sw.knxAddress[0] == knxAddress[0] && sw.knxAddress[1] == knxAddress[1])))
       {
         sw.state = state;
         sw.notifyState(StateOrigin::INTERNAL);
       }
     }
   }*/
  return this;
}

void ConfigOnofre::loopActuators()
{
  for (auto &sw : config.actuatores)
  {
    if (sw.isCover())
    {
      sw.shutter->loop();
    }
    for (auto &button : sw.buttons)
    {
      button.loop();
    }
    if (wifiConnected() && sw.isKnxSupport())
    {
      knx.loop();
      if (sw.knxSync == 3)
      {
        knx.send_1byte_int(knx.GA_to_address(sw.knxAddress[0], sw.knxAddress[1], sw.knxAddress[2]), KNX_CT_READ, 0);
      }
      if (sw.knxSync < 4)
      {
        sw.knxSync++;
      }
    }
  }
}
void ConfigOnofre::loopSensors()
{
  for (auto &s : config.sensors)
  {
    s.loop();
  }
}
