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
String ConfigOnofre::controlSwitch(const char *id, SwitchStateOrigin origin, String state)
{
  return "DONE";
}

void shuttersWriteStateHandler(Shutters *shutters, const char *state, byte length)
{
  for (byte i = 0; i < length; i++)
  {
    shutters->getActuatorT()->shutterState[i] = state[i];
  }
  config.save();
}
void shuttersOperationHandler(Shutters *s, ShuttersOperation operation)
{
  if (s->getActuatorT()->outputs.size() != 2)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s No outputs configured" CR, tags::actuatores);
#endif
    return;
  }
  switch (operation)
  {
  case ShuttersOperation::UP:
    if (s->getActuatorT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {
#ifdef ESP32
      writeToPIN(s->getActuatorT()->outputs[0], LOW);
      delay(1);
      writeToPIN(s->getActuatorT()->outputs[1], HIGH);
#else
      writeToPIN(s->getActuatorT()->outputs[0], HIGH);
      delay(1);
      writeToPIN(s->getActuatorT()->outputs[1], HIGH);
#endif
    }
    break;
  case ShuttersOperation::DOWN:
    if (s->getActuatorT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {
      writeToPIN(s->getActuatorT()->outputs[0], HIGH);
      delay(1);
      writeToPIN(s->getActuatorT()->outputs[1], LOW);
    }

    break;
  case ShuttersOperation::HALT:
    if (s->getActuatorT()->typeControl == SwitchControlType::GPIO_OUTPUT)
    {

#ifdef ESP32
      writeToPIN(s->getActuatorT()->outputs[0], LOW);
      delay(1);
      writeToPIN(s->getActuatorT()->outputs[1], LOW);
#else
      writeToPIN(s->getActuatorT()->outputs[0], LOW);
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
  shutters->getActuatorT()->lastPercentage = level;

  char dump[4] = {0};
  int l = sprintf(dump, "%d", level);
  if (mqttConnected)
  {
    publishOnMqtt(shutters->getActuatorT()->readTopic, dump, false);
  }
  if (cloudIOConnected())
  {
    notifyStateToCloudIO(shutters->getActuatorT()->cloudIOreadTopic, dump);
  }
}

void switchesCallback(message_t const &msg, void *arg)
{
  auto s = static_cast<ActuatorT *>(arg);
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
    s->changeState(SwitchStateOrigin::KNX, String(stateIdx).toInt());
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
      config.controlFeature(SwitchStateOrigin::GPIO_INPUT, a.uniqueId, SwitchState::TOGGLE);
    }
  }
}
void released(Button2 &btn)
{
  Serial.print("released: ");
  Serial.println(btn.wasPressedFor());
}
void changed(Button2 &btn)
{
  Serial.println("changed");
}
void click(Button2 &btn)
{
  Serial.println("click\n");
}
void longClickDetected(Button2 &btn)
{
  Serial.println("long click detected");
}
void longClick(Button2 &btn)
{
  Serial.println("long click\n");
}
void doubleClick(Button2 &btn)
{
  Serial.println("double click\n");
}
void tripleClick(Button2 &btn)
{
  Serial.println("triple click\n");
  Serial.println(btn.getNumberOfClicks());
}
void tap(Button2 &btn)
{
  Serial.println("tap");
}
void ActuatorT::setup()
{
  if (isCover())
  {
    shutter = new Shutters(this);
    char storedShuttersState[shutter->getStateLength()];
    readLastShutterState(storedShuttersState, shutter->getStateLength(), shutter->getActuatorT()->shutterState);
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
    knxIdRegister = knx.callback_register(String(name), switchesCallback, this);
    knxIdAssign = knx.callback_assign(knxIdRegister, knx.GA_to_address(knxAddress[0], knxAddress[1], knxAddress[2]));
    knx.callback_assign(knxIdRegister, knx.GA_to_address(knxAddress[0], knxAddress[1], 0));
    knx.callback_assign(knxIdRegister, knx.GA_to_address(knxAddress[0], 0, 0));
  }
}

void ActuatorT::notifyState(SwitchStateOrigin origin)
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
    notifyStateToCloudIO(readTopic, stateStr.c_str());
  }

  // Notify by SSW Webpanel
  sendToServerEvents(uniqueId, stateStr.c_str());

  // Notify by KNX
  if (SwitchStateOrigin::INTERNAL != origin && isKnxSupport())
  {
    knx.write_1byte_int(knx.GA_to_address(knxAddress[0], knxAddress[1], knxAddress[2]), state);
  }
}

ActuatorT *ActuatorT::changeState(SwitchStateOrigin origin, int state)
{
  if (this->state == state)
    return this;
#ifdef DEBUG_ONOFRE
  Log.notice("%s Name:      %s" CR, tags::actuatores, name);
  Log.notice("%s State:     %d" CR, tags::actuatores, state);
  Log.notice("%s From : %d" CR, tags::actuatores, origin);
  Log.notice("%s Family : %d" CR, tags::actuatores, family);
#endif
  if (outputs.size() == 0)
  {
    return this;
  }
  if (isCover())
  {
    int level = state;
    if (level == SwitchState::STOP)
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
    if (state == SwitchState::ON || state == SwitchState::OFF)
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
        sw.notifyState(SwitchStateOrigin::INTERNAL);
      }
    }
  }
  return this;
}

void ConfigOnofre::loopSwitches()
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
