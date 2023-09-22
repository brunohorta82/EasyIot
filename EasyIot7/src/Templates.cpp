#include "Templates.hpp"
#include <Arduino.h>
#include "Switches.h"
#include "Sensors.h"
#include "Config.h"
extern Config config;
extern Switches switches;
void loadDefaultConfig()
{
    strlcpy(config.nodeId, getChipId().c_str(), sizeof(config.nodeId));
    strlcpy(config.chipId, config.nodeId, sizeof(config.chipId));
    config.mqttPort = constantsMqtt::defaultPort;
    config.staticIp = false;
    strlcpy(config.apSecret, constantsConfig::apSecret, sizeof(config.apSecret));
    strlcpy(config.apiUser, constantsConfig::apiUser, sizeof(config.apiUser));
    strlcpy(config.apiPassword, constantsConfig::apiPassword, sizeof(config.apiPassword));
#ifdef WIFI_SSID
    strlcpy(config.wifiSSID, WIFI_SSID, sizeof(config.wifiSSID));
#endif
#ifdef WIFI_SECRET
    strlcpy(config.wifiSecret, WIFI_SECRET, sizeof(config.wifiSecret));
#endif
    config.knxArea = 1;
    config.knxLine = 1;
    config.knxMember = 1;
    config.firmware = VERSION;
}
void preparePzem()
{
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
    getAtualSensorsConfig().items.push_back(pzem);
}

void prepareLight(String name, unsigned int output, unsigned int input)
{
    Serial.println(name);
    SwitchT one;
    one.firmware = VERSION;
    one.typeControl = SwitchControlType::GPIO_OUTPUT;
    one.primaryGpioControl = output;
    one.mode = PUSH;
    strlcpy(one.name, name.c_str(), sizeof(one.name));
    strlcpy(one.family, constanstsSwitch::familyLight, sizeof(one.family));
    one.primaryGpio = input;
    String idStr;
    generateId(idStr, name, 1, sizeof(one.id));
    strlcpy(one.id, name.c_str(), sizeof(one.id));
    one.statePoolIdx = findPoolIdx("", one.statePoolIdx, one.family);
    switches.items.push_back(one);
}
void prepareCover()
{
    SwitchT one;
    one.firmware = VERSION;
    one.typeControl = SwitchControlType::GPIO_OUTPUT;
    one.primaryGpioControl = 4u;
    one.secondaryGpioControl = 5u;
    one.lastPrimaryGpioState = false;
    one.lastSecondaryGpioState = false;
    one.mode = DUAL_PUSH;
    strlcpy(one.name, "Estore", sizeof(one.name));
    strlcpy(one.family, constanstsSwitch::familyCover, sizeof(one.family));
    one.secondaryGpio = 13u;
#ifdef ESP8266
    one.primaryGpio = 12u;
#endif
#ifdef ESP32
    one.primaryGpio = 14u;
#endif
    strlcpy(one.autoStateValue, "", sizeof(one.autoStateValue));
    String idStr;
    generateId(idStr, one.name, 1, sizeof(one.id));
    strlcpy(one.id, idStr.c_str(), sizeof(one.id));
    one.reloadMqttTopics();
    one.statePoolIdx = findPoolIdx("", one.statePoolIdx, one.family);
    switches.items.push_back(one);
}
void prepareGarage()
{
    SwitchT one;
    one.firmware = VERSION;
    one.typeControl = SwitchControlType::GPIO_OUTPUT;
    one.primaryGpioControl = 4u;
    one.secondaryGpioControl = 5u;
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
    String idStr;
    generateId(idStr, one.name, 1, sizeof(one.id));
    strlcpy(one.id, idStr.c_str(), sizeof(one.id));
    one.reloadMqttTopics();
    one.statePoolIdx = findPoolIdx("", one.statePoolIdx, one.family);
    switches.items.push_back(one);
}
void templateSelect(enum Template _template)
{
    switches.items.clear();

    switch (_template)
    {
    case Template::NO_TEMPLATE:
        break;
    case PZEM:
        preparePzem();
        break;
    case Template::DUAL_LIGHT:
    {
#ifdef CONFIG_LANG_PT
        String name1 = "Interruptor1";
        String name2 = "Interruptor2";
#elif CONFIG_LANG_EN
        String name1 = "Switch1";
        String name2 = "Switch2";
#elif CONFIG_LANG_RO
        String name1 = "Switch1";
        String name2 = "Switch2";
#endif
#ifdef ESP8266
        prepareLight(name1, 4u, 12u);
#endif
#ifdef ESP32
        prepareLight(name1, 4u, 14u);
#endif
        prepareLight(name2, 5u, 13u);
    }
    break;
    case Template::COVER:
        prepareCover();
        break;
    case Template::GARAGE:
        prepareGarage();
        break;
    default:
        return;
        break;
    }
}
