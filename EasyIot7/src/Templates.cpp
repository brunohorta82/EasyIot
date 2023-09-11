#ifndef TEMPLATES_h
#define TEMPLATES_h
#include "Templates.h"
#include <Arduino.h>
#include "Switches.h"
#include "Sensors.h"
#include "Config.h"
void loadDefaultConfig()
{
    strlcpy(getAtualConfig().nodeId, getChipId().c_str(), sizeof(getAtualConfig().nodeId));
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
#ifdef NO_FEATURES
    return;
#endif
#if defined BHPZEM
    SensorT pzem;
    pzem.firmware = VERSION;
    strlcpy(pzem.name, "Consumo", sizeof(pzem.name));
    String idStr;
    generateId(idStr, pzem.name, 2, sizeof(pzem.id));
    strlcpy(pzem.id, idStr.c_str(), sizeof(pzem.id));
    strlcpy(pzem.family, constantsSensor::familySensor, sizeof(pzem.name));
    pzem.primaryGpio = constantsConfig::noGPIO;
    pzem.secondaryGpio = constantsConfig::noGPIO;
    pzem.tertiaryGpio = constantsConfig::noGPIO;
    pzem.type = PZEM_004T_V03;
    pzem.primaryGpio = 3;
    pzem.secondaryGpio = 1;
    pzem.tertiaryGpio = constantsConfig::noGPIO;
    pzem.mqttRetain = true;
    pzem.haSupport = true;
    pzem.emoncmsSupport = true;
    pzem.cloudIOSupport = true;
    pzem.delayRead = 5000;

    strlcpy(pzem.payloadOn, "ON", sizeof(pzem.payloadOn));
    strlcpy(pzem.payloadOff, "OFF", sizeof(pzem.payloadOff));
    strlcpy(pzem.mqttPayload, "", sizeof(pzem.mqttPayload));
    strlcpy(pzem.deviceClass, constantsSensor::powerMeterClass, sizeof(pzem.deviceClass));
    pzem.reloadMqttTopics();
    getAtualSensorsConfig().items.push_back(pzem);
#endif
}
void loadSwitchDefaults()
{
#ifdef NO_FEATURES
    return;
#endif
    SwitchT one;
    one.firmware = VERSION;
    one.typeControl = SwitchControlType::GPIO_OUTPUT;
    one.knxSupport = false;
    one.haSupport = false;
    one.mqttSupport = false;
    one.cloudIOSupport = true;
    one.pullup = true;
    one.mqttRetain = false;
    one.inverted = false;
    one.primaryGpioControl = 4u;
    one.secondaryGpioControl = 5u;
    one.lastPrimaryGpioState = false;
    one.lastSecondaryGpioState = false;

#if defined DUAL_LIGHT
    one.mode = SWITCH;
    strlcpy(one.name, "Interruptor1", sizeof(one.name));
    strlcpy(one.family, constanstsSwitch::familyLight, sizeof(one.family));
#endif

#if defined COVER
    one.mode = DUAL_PUSH;
    strlcpy(one.name, "Estore", sizeof(one.name));
    strlcpy(one.family, constanstsSwitch::familyCover, sizeof(one.family));
    one.secondaryGpio = 13u;
#endif

#if defined GATE
    one.mode = SwitchMode::PUSH;
    strlcpy(one.name, "Garagem", sizeof(one.name));
    strlcpy(one.family, constanstsSwitch::familyGarage, sizeof(one.family));
    one.primaryStateGpio = 13;
    one.secondaryGpioControl = constantsConfig::noGPIO;
#ifdef ESP8266
    one.primaryGpio = 12u;
#endif
#ifdef ESP32
    one.primaryGpio = 14u;
#endif
    one.autoStateDelay = 0;
#endif

#if defined DUAL_LIGHT || COVER
#ifdef ESP8266
    one.primaryGpio = 12u;
#endif
#ifdef ESP32
    one.primaryGpio = 14u;
#endif

    one.autoStateDelay = 0ul;
    strlcpy(one.autoStateValue, "", sizeof(one.autoStateValue));
#endif

#if defined DUAL_LIGHT
    one.secondaryGpio = constantsConfig::noGPIO;
    one.secondaryGpioControl = constantsConfig::noGPIO;
#endif
    String idStr;
    generateId(idStr, one.name, 1, sizeof(one.id));
    strlcpy(one.id, idStr.c_str(), sizeof(one.id));
    one.reloadMqttTopics();
    one.statePoolIdx = findPoolIdx("", one.statePoolIdx, one.family);
    getAtualSwitchesConfig().items.push_back(one);

#if defined DUAL_LIGHT
    SwitchT two;
    two.firmware = VERSION;
    two.lastPrimaryGpioState = false;
    two.lastSecondaryGpioState = false;
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
