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
void prepareSHT4X(int hwAddress)
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::CLIMATIZATION, sizeof(sensor.name));
    sensor.inputs = {constantsConfig::SDA, constantsConfig::SCL};
    sensor.driver = SHT4X;
    sensor.hwAddress = hwAddress;
    sensor.delayRead = constantsConfig::climateReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareLTR303(int hwAddress)
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::ILLUMINANCE, sizeof(sensor.name));
    sensor.inputs = {constantsConfig::SDA, constantsConfig::SCL};
    sensor.driver = LTR303X;
    sensor.hwAddress = hwAddress;
    sensor.delayRead = constantsConfig::illuminanceReadDelay;
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
void prepareActuator(String name, unsigned int output, unsigned int input, ActuatorDriver driver, ActuatorControlType type)
{
    Actuator actuator;
    actuator.driver = driver;
    strncpy(actuator.name, name.c_str(), sizeof(actuator.name));
    actuator.typeControl = type;
    actuator.outputs.push_back(output);
    actuator.inputs.push_back(input);
    String idStr;
    config.generateId(idStr, actuator.name, actuator.driver, sizeof(actuator.uniqueId));
    strlcpy(actuator.uniqueId, idStr.c_str(), sizeof(actuator.uniqueId));
    config.actuatores.push_back(actuator);
}
void prepareVirtualSwitch(String name, unsigned int input, ActuatorDriver driver)
{
    Actuator actuator;
    actuator.driver = driver;
    strncpy(actuator.name, name.c_str(), sizeof(actuator.name));
    actuator.typeControl = ActuatorControlType::VIRTUAL;
    actuator.outputs.clear();
    actuator.inputs.push_back(input);
    String idStr;
    config.generateId(idStr, actuator.name, actuator.driver, sizeof(actuator.uniqueId));
    strlcpy(actuator.uniqueId, idStr.c_str(), sizeof(actuator.uniqueId));
    config.actuatores.push_back(actuator);
}
void prepareCover(String name, unsigned int outputDown, unsigned int outputUp, unsigned int inputDown, unsigned int inputUp, ActuatorDriver driver, ActuatorControlType type)
{
    Actuator cover;
    cover.driver = driver;
    strlcpy(cover.name, name.c_str(), sizeof(cover.name));
    cover.typeControl = type;
    cover.outputs = {outputDown, outputUp};
    cover.inputs = {inputDown, inputUp};
    String idStr;
    config.generateId(idStr, cover.name, cover.driver, sizeof(cover.uniqueId));
    strlcpy(cover.uniqueId, idStr.c_str(), sizeof(cover.uniqueId));
    config.actuatores.push_back(cover);
}
void prepareGarage(String name, unsigned int gateOne, unsigned int gateTwo, unsigned int openCloseSensor, unsigned int pushSwitch, ActuatorDriver driver, ActuatorControlType type)
{
    Actuator garage;
    garage.driver = driver;
    strlcpy(garage.name, name.c_str(), sizeof(garage.name));
    garage.typeControl = type;
    garage.outputs = {gateOne, gateTwo};
    garage.inputs = {openCloseSensor, pushSwitch};
    String idStr;
    config.generateId(idStr, garage.name, garage.driver, sizeof(garage.uniqueId));
    strlcpy(garage.uniqueId, idStr.c_str(), sizeof(garage.uniqueId));
    config.actuatores.push_back(garage);
}
void templateSelect(enum Template _template)
{
    config.actuatores.clear();
    config.sensors.clear();
    switch (_template)
    {
    case Template::NO_TEMPLATE:
        break;
    case Template::DUAL_LIGHT:
    {
        prepareActuator(I18N::SWICTH_ONE, constantsConfig::OUTPUT_ONE, constantsConfig::INPUT_ONE, ActuatorDriver::LIGHT_PUSH, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::SWICTH_TWO, constantsConfig::OUTPUT_TWO, constantsConfig::INPUT_TWO, ActuatorDriver::LIGHT_PUSH, ActuatorControlType::GPIO_OUTPUT);
    }
    break;
    case Template::DUAL_SWITCH:
    {
        prepareActuator(I18N::SWICTH_ONE, constantsConfig::OUTPUT_ONE, constantsConfig::INPUT_ONE, ActuatorDriver::SWITCH_PUSH, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::SWICTH_TWO, constantsConfig::OUTPUT_TWO, constantsConfig::INPUT_TWO, ActuatorDriver::SWITCH_PUSH, ActuatorControlType::GPIO_OUTPUT);
    }
    break;
    case Template::COVER:
        prepareCover(I18N::COVER, constantsConfig::OUTPUT_ONE, constantsConfig::OUTPUT_TWO, constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE, COVER_DUAL_PUSH, ActuatorControlType::GPIO_OUTPUT);
        break;
    case Template::GARAGE:
        prepareGarage(I18N::GARAGE, constantsConfig::OUTPUT_ONE, constantsConfig::OUTPUT_TWO, constantsConfig::INPUT_TWO, constantsConfig::INPUT_ONE, GARAGE_PUSH, ActuatorControlType::GPIO_OUTPUT);
        break;
    case HAN_MODULE:
        prepareHAN();
        break;
    default:
        return;
        break;
    }
}
