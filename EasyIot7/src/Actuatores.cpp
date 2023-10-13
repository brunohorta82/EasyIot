#include "Actuatores.h"
#include <Shutters.h>
#include "HomeAssistantMqttDiscovery.h"
#include "WebServer.h"
#include "constants.h"
#include "ConfigOnofre.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "CloudIO.h"
#include "LittleFS.h"
extern ConfigOnofre config;
void shuttersWriteStateHandler(Shutters *shutters, const char *state, byte length)
{
  for (byte i = 0; i < length; i++)
  {
    shutters->getActuator()->shutterState[i] = state[i];
  }
  config.save();
}
void shuttersOperationHandler(Shutters *s, ShuttersOperation operation)
{
  if (s->getActuator()->outputs.size() != 2)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s No outputs configured" CR, tags::actuatores);
#endif
    return;
  }
  switch (operation)
  {
  case ShuttersOperation::UP:
    if (s->getActuator()->typeControl == ActuatorControlType::GPIO_OUTPUT)
    {
#ifdef ESP32
      writeToPIN(s->getActuator()->outputs[0], LOW);
      delay(1);
      writeToPIN(s->getActuator()->outputs[1], HIGH);
#else
      writeToPIN(s->getActuator()->outputs[0], HIGH);
      delay(1);
      writeToPIN(s->getActuator()->outputs[1], HIGH);
#endif
    }
    break;
  case ShuttersOperation::DOWN:
    if (s->getActuator()->typeControl == ActuatorControlType::GPIO_OUTPUT)
    {
      writeToPIN(s->getActuator()->outputs[0], HIGH);
      delay(1);
      writeToPIN(s->getActuator()->outputs[1], LOW);
    }

    break;
  case ShuttersOperation::HALT:
    if (s->getActuator()->typeControl == ActuatorControlType::GPIO_OUTPUT)
    {

#ifdef ESP32
      writeToPIN(s->getActuator()->outputs[0], LOW);
      delay(1);
      writeToPIN(s->getActuator()->outputs[1], LOW);
#else
      writeToPIN(s->getActuator()->outputs[0], LOW);
#endif
    }
    break;
  }
}
void readLastShutterState(char *dest, byte length, char *value)
{
  for (byte i = 0; i < length; i++)
  {
    dest[i] = value[i];
  }
}

void onShuttersLevelReached(Shutters *shutters, uint8_t level)
{
  shutters->getActuator()->lastPercentage = level;

  char dump[4] = {0};
  int l = sprintf(dump, "%d", level);
  if (mqttConnected)
  {
    publishOnMqtt(shutters->getActuator()->readTopic, dump, false);
  }
  if (cloudIOConnected())
  {
    notifyStateToCloudIO(shutters->getActuator()->cloudIOreadTopic, dump);
  }
}

void actuatoresCallback(message_t const &msg, void *arg)
{
  auto s = static_cast<Actuator *>(arg);
  switch (msg.ct)
  {
  case KNX_CT_ADC_ANSWER:
  case KNX_CT_ADC_READ:
  case KNX_CT_ANSWER:
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
  case KNX_CT_RESTART:
    break;

  case KNX_CT_WRITE:
  {
    uint16_t stateIdx = msg.data[1] | (msg.data[0] << 8);
    s->changeState(StateOrigin::KNX, String(stateIdx).toInt());
    break;
  }
  };
}
void pressed(Button2 &btn)
{
  for (auto a : config.actuatores)
  {
    if (a.sequence == btn.getID())
    {
      config.controlFeature(StateOrigin::GPIO_INPUT, a.uniqueId, ActuatorState::TOGGLE);
    }
  }
}
void released(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s released %d" CR, tags::actuatores, btn.wasPressedFor());
#endif
}
void changed(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Changed" CR, tags::actuatores);
#endif
}
void click(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Click" CR, tags::actuatores);
#endif
}
void longClickDetected(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Long click detected" CR, tags::actuatores);
#endif
}
void longClick(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Long click" CR, tags::actuatores);
#endif
}
void doubleClick(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Double click" CR, tags::actuatores);
#endif
}
void tripleClick(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Triple Click: %d" CR, tags::actuatores, btn.getNumberOfClicks());
#endif
}
void tap(Button2 &btn)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s TAP" CR, tags::actuatores);
#endif
}
void Actuator::setup()
{
  if (isCover())
  {
    shutter = new Shutters(this);
    char storedShuttersState[shutter->getStateLength()];
    readLastShutterState(storedShuttersState, shutter->getStateLength(), shutter->getActuator()->shutterState);
    shutter->setOperationHandler(shuttersOperationHandler)
        .setWriteStateHandler(shuttersWriteStateHandler)
        .restoreState(storedShuttersState)
        .setCourseTime(upCourseTime, downCourseTime)
        .onLevelReached(onShuttersLevelReached)
        .begin();
  }
  for (auto output : outputs)
  {
    configPIN(output, OUTPUT);
    if (isLight() || isSwitch())
    {
      writeToPIN(output, state);
    }
  }
  for (auto input : inputs)
  {
    Button2 button;
    button.begin(input);
    button.setID(sequence);
    button.setPressedHandler(pressed);
    button.setClickHandler(click);
    button.setDoubleClickHandler(doubleClick);
    buttons.push_back(button);
  }
  if (isKnxSupport())
  {
    knx.callback_unassign(knxIdAssign);
    knx.callback_deregister(knxIdRegister);
    knxIdRegister = knx.callback_register(String(name), actuatoresCallback, this);
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
    publishOnMqtt(readTopic, stateStr, false);
  }

  // Notify by MQTT OnofreCloud
  if (cloudIOConnected())
  {
    notifyStateToCloudIO(cloudIOreadTopic, stateStr.c_str());
  }

  // Notify by SSW Webpanel
  sendToServerEvents(uniqueId, stateStr.c_str());

  // Notify by KNX
  if (StateOrigin::INTERNAL != origin && isKnxSupport())
  {
    knx.write_1byte_int(knx.GA_to_address(knxAddress[0], knxAddress[1], knxAddress[2]), state);
  }
}

Actuator *Actuator::changeState(StateOrigin origin, int state)
{
  if (this->state == state)
    return this;
#ifdef DEBUG_ONOFRE
  Log.notice("%s Name:      %s" CR, tags::actuatores, name);
  Log.notice("%s State:     %d" CR, tags::actuatores, state);
  Log.notice("%s From : %d" CR, tags::actuatores, origin);
  Log.notice("%s Family : %d" CR, tags::actuatores, type);
#endif
  if (outputs.size() == 0)
  {
    return this;
  }
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
    if (state == ActuatorState::ON || state == ActuatorState::OFF)
      writeToPIN(outputs[0], state ? HIGH : LOW);
    else
      return this;
  }
  this->state = state;
  this->notifyState(origin);
  if (isKnxGroup())
  {
    for (auto &sw : config.actuatores)
    {
      if (strcmp(sw.uniqueId, uniqueId) == 0)
      {
        sw.state = state;
        sw.notifyState(StateOrigin::INTERNAL);
      }
    }
  }
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
  }
}
void ConfigOnofre::loopSensors()
{
  for (auto &s : config.sensors)
  {
    s.loop();
  }
}
