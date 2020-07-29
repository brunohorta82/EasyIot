#ifndef TEMPLATES_h
#define TEMPLATES_h
#include "Templates.h"
#include <Arduino.h>
#include "Switches.h"
#include "Sensors.h"
#include "Config.h"
void loadDefaultConfig()
{
    strlcpy(getAtualConfig().nodeId, String(ESP.getChipId()).c_str(), sizeof(getAtualConfig().nodeId));
    strlcpy(getAtualConfig().chipId, getAtualConfig().nodeId, sizeof(getAtualConfig().chipId));
    getAtualConfig().mqttPort = constantsMqtt::defaultPort;
    getAtualConfig().staticIp = false;
    strlcpy(getAtualConfig().apSecret, constantsConfig::apSecret, sizeof(getAtualConfig().apSecret));
    strlcpy(getAtualConfig().apiUser, constantsConfig::apiUser, sizeof(getAtualConfig().apiUser));
    strlcpy(getAtualConfig().apiPassword, constantsConfig::apiPassword, sizeof(getAtualConfig().apiPassword));
#ifdef WIFI_SSID
    strlcpy(getAtualConfig().wifiSSID, WIFI_SSID, sizeof(getAtualConfig().wifiSSID));
#endif
#ifdef WIFI_SECRET
    strlcpy(getAtualConfig().wifiSecret, WIFI_SECRET, sizeof(getAtualConfig().wifiSecret));
#endif
    getAtualConfig().knxArea = 1;
    getAtualConfig().knxLine = 1;
    getAtualConfig().knxMember = 1;
    getAtualConfig().firmware = VERSION;
}
void loadSensorsDefaults()
{
#if defined BHPZEM_004T || BHPZEM_004T_2_0 || BHPZEM_004T_V03 || BHPZEM_004T_V03_2_0 || BHPZEM_017
    SensorT pzem;
    strlcpy(pzem.name, "Consumo", sizeof(pzem.name));
    String idStr;
    generateId(idStr, pzem.name,2, sizeof(pzem.id));
    strlcpy(pzem.id, idStr.c_str(), sizeof(pzem.id));
    strlcpy(pzem.family, constantsSensor::familySensor, sizeof(pzem.name));
#if defined BHPZEM_004T_2_0 || BHPZEM_004T
    pzem.type = PZEM_004T;
#endif
#if defined BHPZEM_017
    pzem.type = PZEM_017;
#endif
#if defined BHPZEM_004T_V03 || BHPZEM_004T_V03_2_0
    pzem.type = PZEM_004T_V03;
#endif

#if defined BHPZEM_004T || BHPZEM_004T_V03
    pzem.primaryGpio = 4;
    pzem.secondaryGpio = 5;
#endif
#if defined BHPZEM_004T_2_0 || BHPZEM_004T_V03_2_0 || BHPZEM_017
    pzem.primaryGpio = 3;
    pzem.secondaryGpio = 1;
#endif
    pzem.tertiaryGpio = constantsConfig::noGPIO;
    pzem.mqttRetain = true;
    pzem.haSupport = true;
    pzem.emoncmsSupport = true;
    pzem.delayRead = 5000;
    strlcpy(pzem.payloadOn, "ON", sizeof(pzem.payloadOn));
    strlcpy(pzem.payloadOff, "OFF", sizeof(pzem.payloadOff));
    strlcpy(pzem.mqttPayload, "", sizeof(pzem.mqttPayload));
    strlcpy(pzem.deviceClass, constantsSensor::powerMeterClass, sizeof(pzem.deviceClass));
    getAtualSensorsConfig().items.push_back(pzem);
#endif
}
void loadSwitchDefaults()
{
#if defined SINGLE_SWITCH || DUAL_LIGHT || COVER || GATE
    SwitchT one;
    one.firmware = VERSION;
#if defined DUAL_LIGHT || SINGLE_SWITCH
    one.mode = SWITCH;
    strlcpy(one.name, "Interruptor1", sizeof(one.name));
#endif
#if defined COVER
    strlcpy(one.name, "Estore", sizeof(one.name));
    strlcpy(one.family, constanstsSwitch::familyCover, sizeof(one.family));
    one.mode = DUAL_SWITCH;
    one.secondaryGpio = 13u;
    one.secondaryGpioControl = 5u;
#endif
#if defined GATE
    one.mode = PUSH;
    strlcpy(one.name, "Portão", sizeof(one.name));
    strlcpy(one.family, constanstsSwitch::familyLock, sizeof(one.family));
    one.primaryGpio = constantsConfig::noGPIO;
    one.autoStateDelay = 1000;
    strlcpy(one.autoStateValue, constanstsSwitch::payloadReleased, sizeof(one.autoStateValue));
#endif

    String idStr;
    generateId(idStr, one.name, 1, sizeof(one.id));
    strlcpy(one.id, idStr.c_str(), sizeof(one.id));
#if defined DUAL_LIGHT
    strlcpy(one.family, constanstsSwitch::familyLight, sizeof(one.family));
#endif
#if defined SINGLE_SWITCH
    strlcpy(one.family, constanstsSwitch::familySwitch, sizeof(one.family));
#endif

#if defined DUAL_LIGHT || SINGLE_SWITCH || COVER
    one.primaryGpio = 12u;
    one.autoStateDelay = 0ul;
    strlcpy(one.autoStateValue, "", sizeof(one.autoStateValue));
#endif
#if defined DUAL_LIGHT || SINGLE_SWITCH || GATE
    one.secondaryGpio = constantsConfig::noGPIO;
    one.secondaryGpioControl = constantsConfig::noGPIO;
#endif

    one.typeControl = SwitchControlType::GPIO_OUTPUT;
    one.knxSupport = false;
    one.haSupport = false;
    one.mqttSupport = false;
    one.cloudIOSupport = true;
    one.pullup = true;
    one.mqttRetain = false;
    one.inverted = false;
    one.reloadMqttTopics();
    one.statePoolIdx = findPoolIdx("", one.statePoolIdx, one.family);
    one.primaryGpioControl = 4u;
    getAtualSwitchesConfig().items.push_back(one);
#endif
#if defined DUAL_LIGHT
    SwitchT two;
    two.firmware = VERSION;
    strlcpy(two.name, "Interruptor2", sizeof(two.name));
    String idStr2;
    generateId(idStr2, two.name, 1, sizeof(two.id));
    strlcpy(two.id, idStr2.c_str(), sizeof(two.id));
    strlcpy(two.family, constanstsSwitch::familyLight, sizeof(two.family));
    two.primaryGpio = 13u;
    two.secondaryGpio = constantsConfig::noGPIO;
    two.autoStateDelay = 0ul;
    strlcpy(two.autoStateValue, "", sizeof(two.autoStateValue));
    two.typeControl = SwitchControlType::GPIO_OUTPUT;
    two.mode = SWITCH;
    two.knxSupport = false;
    two.haSupport = false;
    two.mqttSupport = false;
    two.cloudIOSupport = true;
    two.pullup = true;
    two.mqttRetain = false;
    two.inverted = false;
    two.reloadMqttTopics();
    two.statePoolIdx = findPoolIdx("", two.statePoolIdx, two.family);
    two.primaryGpioControl = 5u;
    two.secondaryGpioControl = constantsConfig::noGPIO;
    getAtualSwitchesConfig().items.push_back(two);
#endif
}
#endif