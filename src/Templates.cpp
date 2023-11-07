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
    sensor.inputs = {constantsConfig::HAN_RX, constantsConfig::HAN_TX};
    sensor.driver = HAN;
    sensor.delayRead = constantsConfig::energyReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareSHT3X(int hwAddress)
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::CLIMATIZATION, sizeof(sensor.name));
    sensor.inputs = {constantsConfig::SDA, constantsConfig::SCL};
    sensor.driver = SHT3x_SENSOR;
    sensor.hwAddress = hwAddress;
    sensor.delayRead = constantsConfig::climateReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void preparePzem(String name, unsigned int tx, unsigned int rx, int hwAddress)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.driver = PZEM_004T_V03;
    sensor.inputs = {tx, rx};
    sensor.hwAddress = hwAddress;
    sensor.delayRead = constantsConfig::energyReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareActuator(String name, unsigned int output, unsigned int input, ActuatoDriver driver)
{
    Actuator actuator;
    actuator.driver = driver;
    strncpy(actuator.name, name.c_str(), sizeof(actuator.name));
    actuator.typeControl = ActuatorControlType::GPIO_OUTPUT;
    actuator.outputs.push_back(output);
    actuator.inputs.push_back(input);
    String idStr;
    config.generateId(idStr, actuator.name, actuator.driver, sizeof(actuator.uniqueId));
    strlcpy(actuator.uniqueId, idStr.c_str(), sizeof(actuator.uniqueId));
    config.actuatores.push_back(actuator);
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
        prepareActuator(I18N::SWICTH_ONE, constantsConfig::OUTPUT_ONE, constantsConfig::INPUT_ONE, ActuatoDriver::SWITCH_PUSH);
        prepareActuator(I18N::SWICTH_TWO, constantsConfig::OUTPUT_TWO, constantsConfig::INPUT_TWO, ActuatoDriver::SWITCH_PUSH);
        break;
    case HAN_MODULE:
        prepareHAN();
        break;
    case Template::DUAL_LIGHT:
    {
        prepareActuator(I18N::SWICTH_ONE, constantsConfig::OUTPUT_ONE, constantsConfig::INPUT_ONE, ActuatoDriver::LIGHT_PUSH);
        prepareActuator(I18N::SWICTH_TWO, constantsConfig::OUTPUT_TWO, constantsConfig::INPUT_TWO, ActuatoDriver::LIGHT_PUSH);
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
