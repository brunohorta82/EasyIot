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
    sensor.inputs = {DefaultPins::HAN_RX, DefaultPins::HAN_TX};
    sensor.driver = HAN;
    sensor.delayRead = constantsConfig::energyReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, DefaultPins::HAN_RX, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareSHT4X(int hwAddress)
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::CLIMATIZATION, sizeof(sensor.name));
    sensor.inputs = {DefaultPins::SDA, DefaultPins::SCL};
    sensor.driver = SHT4X;
    sensor.hwAddress = hwAddress;
    sensor.delayRead = constantsConfig::climateReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, hwAddress, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareHCSR04(String name, unsigned int triggerPin, unsigned int echoPin)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.inputs = {triggerPin, echoPin};
    sensor.driver = SensorDriver::HCSR04;
    sensor.hwAddress = triggerPin;
    sensor.delayRead = constantsConfig::hcsr04Delay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, triggerPin, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareDoorOrWindow(String name, unsigned int inputPin, SensorDriver driver)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.inputs = {inputPin};
    sensor.driver = driver;
    sensor.hwAddress = inputPin;
    sensor.delayRead = constantsConfig::hallsensorDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, inputPin, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareRain(String name, unsigned int inputPin)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.inputs = {inputPin};
    sensor.driver = SensorDriver::RAIN;
    sensor.hwAddress = inputPin;
    sensor.delayRead = constantsConfig::rainDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, inputPin, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void preparePir(String name, unsigned int inputPin)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.inputs = {inputPin};
    sensor.driver = SensorDriver::PIR;
    sensor.hwAddress = inputPin;
    sensor.delayRead = constantsConfig::pirDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, inputPin, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareDHT(String name, unsigned int inputPin, SensorDriver driver)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.inputs = {inputPin};
    sensor.driver = driver;
    sensor.hwAddress = inputPin;
    sensor.delayRead = constantsConfig::climateReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, inputPin, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}

void prepareDS18B20(String name, unsigned int inputPin)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.inputs = {inputPin};
    sensor.driver = SensorDriver::DS18B20;
    sensor.hwAddress = inputPin;
    sensor.delayRead = constantsConfig::climateReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, inputPin, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareLTR303(int hwAddress)
{
    Sensor sensor;
    strlcpy(sensor.name, I18N::ILLUMINANCE, sizeof(sensor.name));
    sensor.inputs = {DefaultPins::SDA, DefaultPins::SCL};
    sensor.driver = LTR303X;
    sensor.hwAddress = hwAddress;
    sensor.delayRead = constantsConfig::illuminanceReadDelay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, DefaultPins::SDA, sizeof(sensor.uniqueId));
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
    config.generateId(idStr, sensor.name, sensor.driver, tx, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareActuator(String name, unsigned int output, unsigned int input, ActuatorDriver driver, ActuatorControlType type)
{
    Actuator actuator;
    actuator.driver = driver;
    strncpy(actuator.name, name.c_str(), sizeof(actuator.name));
    actuator.typeControl = type;
    if (output != DefaultPins::noGPIO)
        actuator.outputs.push_back(output);
    if (input != DefaultPins::noGPIO)
        actuator.inputs.push_back(input);
    String idStr;
    config.generateId(idStr, actuator.name, actuator.driver, output, sizeof(actuator.uniqueId));
    strlcpy(actuator.uniqueId, idStr.c_str(), sizeof(actuator.uniqueId));
    config.actuatores.push_back(actuator);
}
int prepareNewFeature(String name, unsigned int input1, unsigned int input2, int driverCode)
{
    if (driverCode < 60)
    {
        return prepareVirtualSwitch(name, input1, input2, (ActuatorDriver)driverCode);
    }
    else
    {
        switch (driverCode)
        {
        case SensorDriver::DHT_11:
        case SensorDriver::DHT_21:
        case SensorDriver::DHT_22:
            prepareDHT(name, input1, (SensorDriver)driverCode);
            break;
        case SensorDriver::DS18B20:
            prepareDS18B20(name, input1);
            break;
        case SensorDriver::DOOR:
        case SensorDriver::WINDOW:
            prepareDoorOrWindow(name, input1, (SensorDriver)driverCode);
            break;
        case SensorDriver::PIR:
            preparePir(name, input1);
            break;
        case SensorDriver::HCSR04:
            prepareHCSR04(name, input1, input2);
            break;
        case SensorDriver::RAIN:
            prepareRain(name, input1);
            break;
        case SensorDriver::PZEM_004T_V03:
            preparePzem(name, input1, input2, Discovery::MODBUS_PZEM_ADDRESS_DEFAULT);
            break;
        default:
            break;
        }
    }
    return 0;
}
int prepareVirtualSwitch(String name, unsigned int input1, unsigned int input2, ActuatorDriver driver)
{
    if (name.isEmpty())
        return 1;
    if (!config.validPin(input1))
        return 2;
    if (ActuatorDriver::INVALID == driver)
        return 3;
    Actuator actuator;
    actuator.driver = driver;
    actuator.inputs.push_back(input1);
    if (actuator.requireDualInputs() && !config.validPin(input2))
    {
        return 4;
    }
    else if (actuator.requireDualInputs())
    {
        actuator.inputs.push_back(input2);
    }
    strncpy(actuator.name, name.c_str(), sizeof(actuator.name));
    actuator.typeControl = ActuatorControlType::VIRTUAL;
    actuator.outputs.clear();
    String idStr;
    config.generateId(idStr, actuator.name, actuator.driver, input1, sizeof(actuator.uniqueId));
    strlcpy(actuator.uniqueId, idStr.c_str(), sizeof(actuator.uniqueId));
    actuator.setup();
    config.actuatores.push_back(actuator);
    return 0;
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
    config.generateId(idStr, cover.name, cover.driver, outputDown, sizeof(cover.uniqueId));
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
    config.generateId(idStr, garage.name, garage.driver, openCloseSensor, sizeof(garage.uniqueId));
    strlcpy(garage.uniqueId, idStr.c_str(), sizeof(garage.uniqueId));
    config.actuatores.push_back(garage);
}
void templateSelect(enum Template _template)
{
    if (config.templateId != Template::NO_TEMPLATE)
        return;
#ifdef DEBUG_ONOFRE
    Log.info("%s Template selected: %d" CR, tags::webserver, _template);
#endif
    config.actuatores.clear();
    config.sensors.clear();
    switch (_template)
    {
    case Template::NO_TEMPLATE:
        break;
    case Template::DUAL_LIGHT:
    {
        prepareActuator(I18N::SWICTH_ONE, DefaultPins::OUTPUT_ONE, DefaultPins::INPUT_ONE, ActuatorDriver::LIGHT_PUSH, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::SWICTH_TWO, DefaultPins::OUTPUT_TWO, DefaultPins::INPUT_TWO, ActuatorDriver::LIGHT_PUSH, ActuatorControlType::GPIO_OUTPUT);
    }
    break;
    case Template::DUAL_SWITCH:
    {
        prepareActuator(I18N::SWICTH_ONE, DefaultPins::OUTPUT_ONE, DefaultPins::INPUT_ONE, ActuatorDriver::SWITCH_PUSH, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::SWICTH_TWO, DefaultPins::OUTPUT_TWO, DefaultPins::INPUT_TWO, ActuatorDriver::SWITCH_PUSH, ActuatorControlType::GPIO_OUTPUT);
    }
    break;
    case Template::COVER:
        prepareCover(I18N::COVER, DefaultPins::OUTPUT_ONE, DefaultPins::OUTPUT_TWO, DefaultPins::INPUT_TWO, DefaultPins::INPUT_ONE, COVER_DUAL_PUSH, ActuatorControlType::GPIO_OUTPUT);
        break;
    case Template::GARAGE:
        prepareGarage(I18N::GARAGE, DefaultPins::OUTPUT_ONE, DefaultPins::OUTPUT_TWO, DefaultPins::INPUT_TWO, DefaultPins::INPUT_ONE, GARAGE_PUSH, ActuatorControlType::GPIO_OUTPUT);
        break;
    case HAN_MODULE:
        prepareHAN();
        break;
    case GARDEN:
        prepareActuator(I18N::VALVE_ONE, DefaultPins::OUTPUT_ONE, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_TWO, DefaultPins::OUTPUT_TWO, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
#ifdef ESP8266
        prepareActuator(I18N::VALVE_THREE, DefaultPins::OUTPUT_VALVE_THREE, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_FOUR, DefaultPins::OUTPUT_VALVE_FOUR, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
#endif
        break;
    default:
        return;
        break;
    }
}
