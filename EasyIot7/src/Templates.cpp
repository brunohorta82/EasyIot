#include <Arduino.h>
#include "Templates.h"
#include "Actuatores.h"
#include "Sensors.h"
#include "ConfigOnofre.h"
extern ConfigOnofre config;
void prepareHAN()
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::HAN, sizeof(sensor.name));
    sensor.inputs = {constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE};
    sensor.driver = HAN;
    sensor.delayRead = constantsConfig::energyReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareSHT3X()
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::CLIMATIZATION, sizeof(sensor.name));
    sensor.inputs = {constantsConfig::SDA, constantsConfig::SCL};
    sensor.driver = SHT3x_SENSOR;
    sensor.delayRead = constantsConfig::climateReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void preparePzem()
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::ENERGY, sizeof(sensor.name));
    sensor.driver = PZEM_004T_V03;
    sensor.inputs = {constantsConfig::PZEM_TX, constantsConfig::PZEM_RX};
    sensor.delayRead = constantsConfig::energyReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareLight(String name, unsigned int output, unsigned int input)
{
    Actuator light;
    light.driver = LIGHT_PUSH;
    strncpy(light.name, name.c_str(), sizeof(light.name));
    light.typeControl = ActuatorControlType::GPIO_OUTPUT;
    light.outputs.push_back(output);
    light.inputs.push_back(input);
    String idStr;
    config.generateId(idStr, light.name, light.driver, sizeof(light.uniqueId));
    strlcpy(light.uniqueId, idStr.c_str(), sizeof(light.uniqueId));
    config.actuatores.push_back(light);
}
void prepareCover()
{
    Actuator cover;
    cover.driver = COVER_DUAL_PUSH;
    strlcpy(cover.name, I18N::COVER, sizeof(cover.name));
    cover.typeControl = ActuatorControlType::GPIO_OUTPUT;
    cover.outputs = {constantsConfig::OUTPUT_ONE, constantsConfig::OUTPUT_TWO};
    cover.inputs = {constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE};
    String idStr;
    config.generateId(idStr, cover.name, cover.driver, sizeof(cover.uniqueId));
    strlcpy(cover.uniqueId, idStr.c_str(), sizeof(cover.uniqueId));
    config.actuatores.push_back(cover);
}
void prepareGarage()
{
    Actuator garage;
    garage.driver = GARAGE_PUSH;
    strlcpy(garage.name, I18N::GARAGE, sizeof(garage.name));
    garage.typeControl = ActuatorControlType::GPIO_OUTPUT;
    garage.outputs = {constantsConfig::OUTPUT_ONE, constantsConfig::OUTPUT_TWO};
    garage.inputs = {constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE};
    String idStr;
    config.generateId(idStr, garage.name, garage.driver, sizeof(garage.uniqueId));
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
