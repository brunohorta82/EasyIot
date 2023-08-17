#include "Switches.h"
#include <Shutters.h>
#include "HomeAssistantMqttDiscovery.h"
#include "WebServer.h"
#include "constants.h"
#include "Config.h"
#include "Mqtt.h"
#include <esp-knx-ip.h>
#include "CloudIO.h"
#include "LittleFS.h"
extern Config config;
String Config::controlSwitch(const char *id, SwitchStateOrigin origin, String state)
{
  return "DONE";
}
const String SwitchT::getCurrentState()
{
  if (isCover())
  {
    return String(lastPercentage);
  }
  // TODO
  return "TODO";
}

void shuttersWriteStateHandler(Shutters *shutters, const char *state, byte length)
{
  for (byte i = 0; i < length; i++)
  {
    shutters->getSwitchT()->shutterState[i] = state[i];
  }

  // TODO getAtualSwitchesConfig().save();
  if (mqttConnected())
  {
    // TODO publishOnMqtt(shutters->getSwitchT()->stateTopic(), shutters->getSwitchT()->getCurrentState().c_str(), false);
  }
  /* if (shutters->getSwitchT()->cloudIOSupport)
   {
     if (shutters->getSwitchT()->getCurrentState().equalsIgnoreCase(constanstsSwitch::payloadStop))
     {
       // TODO  notifyStateToCloudIO(shutters->getSwitchT()->mqttCloudStateTopic, shutters->getSwitchT()->getCurrentState().c_str(), strlen(shutters->getSwitchT()->getCurrentState().c_str()));
     }
     else
     {
       char dump[4] = {0};
       int l = sprintf(dump, "%d", shutters->getSwitchT()->lastPercentage);
       // TODO notifyStateToCloudIO(shutters->getSwitchT()->mqttCloudStateTopic, dump, l);
     }
   }*/
}
void shuttersOperationHandler(Shutters *s, ShuttersOperation operation)
{
  /*  if (s->getSwitchT()->outputs.size() != 2)
    {
  #ifdef DEBUG_ONOFRE
      Log.error("%s No outputs configured" CR, tags::switches);
  #endif
      return;
    }
    switch (operation)
    {
    case ShuttersOperation::UP:
      if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
      {
  #ifdef ESP32
        writeToPIN(s->getSwitchT()->outputs[0], LOW);
        delay(1);
        writeToPIN(s->getSwitchT()->outputs[1], HIGH);
  #else
        writeToPIN(s->getSwitchT()->outputs[0], HIGH);
        delay(1);
        writeToPIN(s->getSwitchT()->outputs[1], HIGH);
  #endif
      }
      break;
    case ShuttersOperation::DOWN:
      if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
      {
        writeToPIN(s->getSwitchT()->outputs[0], HIGH);
        delay(1);
        writeToPIN(s->getSwitchT()->outputs[1], LOW);
      }

      break;
    case ShuttersOperation::HALT:
      if (s->getSwitchT()->typeControl == SwitchControlType::GPIO_OUTPUT)
      {

  #ifdef ESP32
        writeToPIN(s->getSwitchT()->outputs[0], LOW);
        delay(1);
        writeToPIN(s->getSwitchT()->outputs[1], LOW);
  #else
        writeToPIN(s->getSwitchT()->outputs[0], LOW);
  #endif
      }
      break;
    }
    */
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
  /* shutters->getSwitchT()->lastPercentage = level;

   char dump[4] = {0};
   int l = sprintf(dump, "%d", level);
   if (mqttConnected)
   {
     publishOnMqtt(shutters->getSwitchT()->stateTopic(), dump, false);
   }
   if (shutters->getSwitchT()->cloudIOSupport)
   {
     // TODO   notifyStateToCloudIO(shutters->getSwitchT()->mqttCloudStateTopic, dump, l);
   }*/
}

void switchesCallback(message_t const &msg, void *arg)
{
  auto s = static_cast<SwitchT *>(arg);
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
    s->changeState(SwitchStateOrigin::KNX, String(stateIdx));
    break;
  }
  };
}
void mqttSwitchControl(SwitchStateOrigin origin, const char *topic, const char *payload)
{
  for (auto &sw : config.switches)
  {
    if (strcmp(sw.writeTopic, topic) == 0)
    {
      sw.changeState(origin, String(payload));
    }
  }
}
void SwitchT::setup()
{
  if (isCover())
  {
    shutter = new Shutters(this);
    char storedShuttersState[shutter->getStateLength()];
    readLastShutterState(storedShuttersState, shutter->getStateLength(), shutter->getSwitchT()->shutterState);
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
  }
  for (auto input : inputs)
  {
    Bounce bounce = Bounce();
    bounce.attach(input, INPUT_PULLUP);
    bounce.interval(5);
    inputsBounced.push_back(bounce);
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

const void SwitchT::notifyState(bool dirty, const char *origin)
{
  if (strcmp("INTERNAL", origin) == 0)
  {
    return;
  }
  const String currentStateToSend = getCurrentState();
#ifdef DEBUG_ONOFRE
  Log.notice("%s %s current state: %s" CR, tags::switches, name, currentStateToSend.c_str());
#endif
  if (mqttConnected)
  {
    publishOnMqtt(readTopic, currentStateToSend.c_str(), true);
  }
  if (cloudIOSupport)
  {
    notifyStateToCloudIO(readTopic, currentStateToSend.c_str(), currentStateToSend.length());
  }
  sendToServerEvents(id, currentStateToSend.c_str());
  if (strcmp("KNX", origin) != 0 && isKnxSupport())
  {
    // TODO knx.write_1byte_int(knx.GA_to_address(knxAddress[0], knxAddress[1], knxAddress[1]), statePoolIdx);
  }
  if (isKnxGroup())
  {
    for (auto &sw : config.switches)
    {

      if (strcmp(sw.id, id) != 0)
      {
        sw.changeState(SwitchStateOrigin::INTERNAL, currentStateToSend.c_str());
      }
    }
  }
}
SwitchT *SwitchT::changeState(SwitchStateOrigin origin, String state)
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Name:      %s" CR, tags::switches, name);
  Log.notice("%s State:     %s" CR, tags::switches, state);
  Log.notice("%s From : %d" CR, tags::switches, origin);
  Log.notice("%s Family : %d" CR, tags::switches, family);
#endif
  if (isCover())
  {
    int level = state.toInt();
    if (level > 100)
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
  else if (isLight() || isSwitch())
  {
    writeToPIN(outputs[0], state.toInt() ? HIGH : LOW);
  }

  return this;
}

void Config::loopSwitches()
{
  for (auto &sw : config.switches)
  {
    if (sw.isCover())
    {
      sw.shutter->loop();
    }

    for (auto bounce : sw.inputsBounced)
    {
      bounce.read();
    }

    /*  switch (sw.mode)
      {
      case SWITCH:
        if (sw.primaryGpio != constantsConfig::noGPIO && sw.lastPrimaryGpioState != primaryGpioEvent)
        {
  #ifdef DEBUG_ONOFRE
          Log.notice("%s 1 lastPrimaryGpioState %d primaryGpioEvent %d" CR, tags::switches, sw.lastPrimaryGpioState, primaryGpioEvent);
  #endif
          sw.rotateState();
  #ifdef DEBUG_ONOFRE
          Log.notice("%s 2 lastPrimaryGpioState %d primaryGpioEvent %d" CR, tags::switches, sw.lastPrimaryGpioState, primaryGpioEvent);
  #endif
        }
        break;
      case PUSH:
        if (sw.primaryGpio != constantsConfig::noGPIO && !primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
        {
          sw.rotateState();
        }

        break;
      case GATE_SWITCH:
      {
        bool primaryStateGpioEvent = true;
        bool secondaryStateGpioEvent = true;
        if (sw.primaryStateGpio != constantsConfig::noGPIO)
        {
          primaryStateGpioEvent = readPIN(sw.primaryStateGpio);
          if (sw.lastPrimaryStateGpioState != primaryStateGpioEvent)
          {
            sw.lastPrimaryStateGpioState = primaryStateGpioEvent;
            if (primaryStateGpioEvent)
            {
              sw.statePoolIdx = constanstsSwitch::closeIdx;
            }
            else
            {
              sw.statePoolIdx = constanstsSwitch::openIdx;
            }
            sw.notifyState(true, "LOOP");
          }
        }
        if (sw.secondaryStateGpio != constantsConfig::noGPIO)
        {
          secondaryStateGpioEvent = readPIN(sw.secondaryStateGpio);
          if (sw.lastSecondaryStateGpioState != secondaryStateGpioEvent)
          {
            sw.lastSecondaryStateGpioState = secondaryStateGpioEvent;
            if (secondaryStateGpioEvent)
            {
              sw.statePoolIdx = constanstsSwitch::openIdx;
            }
            else
            {
              sw.statePoolIdx = constanstsSwitch::closeIdx;
            }
            sw.notifyState(true, "LOOP");
          }
        }
      }

      break;
      case DUAL_SWITCH:
        if (strcmp(sw.family, constanstsSwitch::familyCover) == 0 && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
        {
          if (primaryGpioEvent && secondaryGpioEvent && (sw.lastPrimaryGpioState != primaryGpioEvent || sw.lastSecondaryGpioState != secondaryGpioEvent))
          {
            sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::firtStopIdx].c_str(), "DUAL_SWITCH_STOP");
          }
          else if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
          {
            sw.changeState(STATES_POLL[constanstsSwitch::openIdx].c_str(), "DUAL_SWITCH_OPEN");
          }
          else if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
          {
            sw.changeState(STATES_POLL[constanstsSwitch::closeIdx].c_str(), "DUAL_SWITCH_CLOSE");
          }
        }
        break;
      case DUAL_PUSH:
        if (strcmp(sw.family, constanstsSwitch::familyCover) == 0 && sw.primaryGpio != constantsConfig::noGPIO && sw.secondaryGpio != constantsConfig::noGPIO)
        {
          if (!primaryGpioEvent && sw.lastPrimaryGpioState != primaryGpioEvent)
          { // PUSHED
            sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::openIdx ? constanstsSwitch::firtStopIdx : constanstsSwitch::openIdx].c_str(), "DUAL_PUSH");
          }
          if (!secondaryGpioEvent && sw.lastSecondaryGpioState != secondaryGpioEvent)
          { // PUSHED
            sw.changeState(STATES_POLL[sw.statePoolIdx == constanstsSwitch::closeIdx ? constanstsSwitch::secondStopIdx : constanstsSwitch::closeIdx].c_str(), "DUAL_PUSH");
          }
        }
        break;
      default:
        break;
      }
      sw.lastPrimaryGpioState = primaryGpioEvent;
      sw.lastSecondaryGpioState = secondaryGpioEvent;
      if (stateTimeout(sw))
      {

  #ifdef DEBUG_ONOFRE
        Log.notice("%s State Timeout set change switch to %s " CR, tags::switches, STATES_POLL[sw.statePoolIdx].c_str());
  #endif
        sw.changeState(STATES_POLL[findPoolIdx(sw.autoStateValue, sw.statePoolIdx, sw.family)].c_str(), "AUTO_TIMEOUT");
      }
      */
  }
}
