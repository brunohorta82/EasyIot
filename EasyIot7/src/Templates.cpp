#include <Arduino.h>
#include "Templates.h"
#include "Actuatores.h"
#include "Sensors.h"
#include "ConfigOnofre.h"
extern ConfigOnofre config;
void prepareHAN()
{
    Sensor han;
    strlcpy(han.name, I18N::T_HAN, sizeof(han.name));
    han.inputs = {13u, 14u};
    han.type = HAN;
    han.delayRead = 5000;
    String idStr;
    config.generateId(idStr, han.name, han.type, sizeof(han.uniqueId));
    strlcpy(han.uniqueId, idStr.c_str(), sizeof(han.uniqueId));
    config.sensors.push_back(han);
}
void preparePzem()
{
    Sensor pzem;
    strlcpy(pzem.name, I18N::T_ENERGY, sizeof(pzem.name));
    pzem.type = PZEM_004T_V03;
    pzem.inputs = {3u, 1u};
    pzem.delayRead = 5000;
    String idStr;
    config.generateId(idStr, pzem.name, pzem.type, sizeof(pzem.uniqueId));
    strlcpy(pzem.uniqueId, idStr.c_str(), sizeof(pzem.uniqueId));
    config.sensors.push_back(pzem);
}
void prepareLight(String name, unsigned int output, unsigned int input)
{
    Actuator light;
    light.type = LIGHT_PUSH;
    strncpy(light.name, name.c_str(), sizeof(light.name));
    light.typeControl = ActuatorControlType::GPIO_OUTPUT;
    light.outputs.push_back(output);
    light.inputs.push_back(input);
    String idStr;
    config.generateId(idStr, light.name, light.type, sizeof(light.uniqueId));
    strlcpy(light.uniqueId, idStr.c_str(), sizeof(light.uniqueId));
    config.actuatores.push_back(light);
}
void prepareCover()
{
    Actuator cover;
    cover.type = COVER_DUAL_PUSH;
    strlcpy(cover.name, I18N::T_COVER, sizeof(cover.name));
    cover.typeControl = ActuatorControlType::GPIO_OUTPUT;
    cover.outputs = {4u, 5u};
#ifdef ESP8266
    cover.inputs = {12u, 13u};
#endif
#ifdef ESP32
    cover.inputs = {13u, 14u};
#endif
    String idStr;
    config.generateId(idStr, cover.name, cover.type, sizeof(cover.uniqueId));
    strlcpy(cover.uniqueId, idStr.c_str(), sizeof(cover.uniqueId));
    config.actuatores.push_back(cover);
}
void prepareGarage()
{
    Actuator garage;
    garage.type = GARAGE_PUSH;
    strlcpy(garage.name, I18N::T_GARAGE, sizeof(garage.name));
    garage.typeControl = ActuatorControlType::GPIO_OUTPUT;
    garage.outputs = {4u, 5u};
#ifdef ESP8266
    garage.inputs = {12u, 13u};
#endif
#ifdef ESP32
    garage.inputs = {13u, 14u};
#endif
    String idStr;
    config.generateId(idStr, garage.name, garage.type, sizeof(garage.uniqueId));
    strlcpy(garage.uniqueId, idStr.c_str(), sizeof(garage.uniqueId));
    config.actuatores.push_back(garage);
}
void templateSelect(enum Template _template)
{
    switch (_template)
    {
    case Template::NO_TEMPLATE:
        break;
    case PZEM:
        preparePzem();
        break;
    case HAN_MODULE:
        prepareHAN();
        break;
    case Template::DUAL_LIGHT:
    {
#ifdef ESP8266
        prepareLight(I18N::T_LIGHT_ONE, 4u, 12u);
#endif
#ifdef ESP32
        prepareLight(I18N::T_LIGHT_ONE, 4u, 14u);
#endif
        prepareLight(I18N::T_LIGHT_TWO, 5u, 13u);
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
