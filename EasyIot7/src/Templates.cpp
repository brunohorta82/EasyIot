#include <Arduino.h>
#include "Templates.h"
#include "Actuatores.h"
#include "Sensors.h"
#include "ConfigOnofre.h"
extern ConfigOnofre config;
void prepareHAN()
{
    Sensor han;
    strlcpy(han.name, I18N::HAN, sizeof(han.name));
    han.inputs = {constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE};
    han.type = HAN;
    han.delayRead = constantsConfig::energyReadDelay;
    String idStr;
    config.generateId(idStr, han.name, han.type, sizeof(han.uniqueId));
    strlcpy(han.uniqueId, idStr.c_str(), sizeof(han.uniqueId));
    config.sensors.push_back(han);
}
void prepareSHT3X()
{
    Sensor sht;
    strlcpy(sht.name, I18N::CLIMATIZATION, sizeof(sht.name));
    sht.inputs = {constantsConfig::SDA, constantsConfig::SCL};
    sht.type = SHT3x_SENSOR;
    sht.delayRead = constantsConfig::climateReadDelay;
    String idStr;
    config.generateId(idStr, sht.name, sht.type, sizeof(sht.uniqueId));
    strlcpy(sht.uniqueId, idStr.c_str(), sizeof(sht.uniqueId));
    config.sensors.push_back(sht);
}
void preparePzem()
{
    Sensor pzem;
    strlcpy(pzem.name, I18N::ENERGY, sizeof(pzem.name));
    pzem.type = PZEM_004T_V03;
    pzem.inputs = {constantsConfig::PZEM_RX, constantsConfig::PZEM_TX};
    pzem.delayRead = constantsConfig::energyReadDelay;
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
    strlcpy(cover.name, I18N::COVER, sizeof(cover.name));
    cover.typeControl = ActuatorControlType::GPIO_OUTPUT;
    cover.outputs = {constantsConfig::OUTPUT_ONE, constantsConfig::OUTPUT_TWO};
    cover.inputs = {constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE};
    String idStr;
    config.generateId(idStr, cover.name, cover.type, sizeof(cover.uniqueId));
    strlcpy(cover.uniqueId, idStr.c_str(), sizeof(cover.uniqueId));
    config.actuatores.push_back(cover);
}
void prepareGarage()
{
    Actuator garage;
    garage.type = GARAGE_PUSH;
    strlcpy(garage.name, I18N::GARAGE, sizeof(garage.name));
    garage.typeControl = ActuatorControlType::GPIO_OUTPUT;
    garage.outputs = {constantsConfig::OUTPUT_ONE, constantsConfig::OUTPUT_TWO};
    garage.inputs = {constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE};
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
    case Template::SHT3X_CLIMATE:
        prepareSHT3X();
        break;
    case PZEM:
        preparePzem();
        break;
    case HAN_MODULE:
        prepareHAN();
        break;
    case Template::DUAL_LIGHT:
    {
        prepareLight(I18N::LIGHT_ONE, constantsConfig::OUTPUT_ONE, constantsConfig::INPUT_ONE);
        prepareLight(I18N::LIGHT_TWO, constantsConfig::OUTPUT_TWO, constantsConfig::INPUT_TWO);
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
