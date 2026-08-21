#include <Arduino.h>
#include "Templates.h"
#include "Actuatores.h"
#include "Sensors.h"
#include "ConfigOnofre.h"
#include "Irrigation.h"
#include <algorithm>
extern ConfigOnofre config;

namespace
{
bool isSupportedActuatorDriverCode(int driverCode)
{
    switch (driverCode)
    {
    case ActuatorDriver::SWITCH_PUSH:
    case ActuatorDriver::SWITCH_LATCH:
    case ActuatorDriver::COVER_SINGLE_PUSH:
    case ActuatorDriver::COVER_DUAL_PUSH:
    case ActuatorDriver::COVER_DUAL_LATCH:
    case ActuatorDriver::LIGHT_PUSH:
    case ActuatorDriver::LIGHT_LATCH:
    case ActuatorDriver::GARAGE_PUSH:
    case ActuatorDriver::GARDEN_VALVE:
        return true;
    case ActuatorDriver::INVALID:
    default:
        return false;
    }
}

bool setActuatorName(Actuator &actuator, const String &name)
{
    // String::length() is the number of bytes copied into this C buffer. Leave
    // one byte for the terminator before generateId() treats it as a C string.
    if (name.isEmpty() || name.length() >= sizeof(actuator.name))
        return false;
    strlcpy(actuator.name, name.c_str(), sizeof(actuator.name));
    return true;
}

void clearLiveFeatureGraphSafely()
{
    // init() selects its first template with an empty graph. Do not touch GPIOs
    // in that path; teardown is only for replacing already-live features.
    if (config.actuatores.empty() && config.sensors.empty())
        return;

    if (irrigation.isRunning())
        irrigation.stop();

    std::vector<unsigned int> releasedOutputs;
    for (auto &actuator : config.actuatores)
    {
        // Covers need reset(), not stop(): reset emits HALT synchronously, while
        // stop normally waits for a later Shutters::loop() that will never run.
        actuator.deactivateForConfigUpdate();
        for (const auto output : actuator.outputs)
        {
            if (!config.validOutputPin(output) ||
                std::find(releasedOutputs.begin(), releasedOutputs.end(), output) != releasedOutputs.end())
                continue;
            configPIN(output, OUTPUT);
            writeToPIN(output, LOW);
            configPIN(output, INPUT);
            releasedOutputs.push_back(output);
        }
    }
    for (auto &sensor : config.sensors)
        sensor.deactivateForConfigUpdate();
}
} // namespace

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

void prepareLD2410(String name, unsigned int rx, unsigned int tx)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    // Sensor::loop passes slot 0 as UART RX and slot 1 as UART TX.
    sensor.inputs = {rx, tx};
    sensor.driver = LD2410;
    sensor.delayRead = constantsConfig::ld2410Delay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, rx, sizeof(sensor.uniqueId));
    strlcpy(sensor.uniqueId, idStr.c_str(), sizeof(sensor.uniqueId));
    config.sensors.push_back(sensor);
}
void prepareTMF882X(int hwAddress)
{
    Sensor sensor;
    strlcpy(sensor.name, "TEST TOF", sizeof(sensor.name));
    sensor.inputs = {DefaultPins::SDA, DefaultPins::SCL};
    sensor.driver = TMF882X;
    sensor.hwAddress = hwAddress;
    sensor.delayRead = constantsConfig::ld2410Delay;
    String idStr;
    config.generateId(idStr, sensor.name, sensor.driver, hwAddress, sizeof(sensor.uniqueId));
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
void preparePzem(String name, unsigned int tx, unsigned int rx, int hwAddress, SensorDriver driver)
{
    Sensor sensor;
    strlcpy(sensor.name, name.c_str(), sizeof(sensor.name));
    sensor.driver = driver;
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
    if (!setActuatorName(actuator, name))
        return;
    actuator.driver = driver;
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
    // Reject negative, unknown and inert values before converting an integer
    // supplied by HTTP into an enum used by the actuator runtime.
    if (isSupportedActuatorDriverCode(driverCode))
    {
        return prepareVirtualSwitch(name, input1, input2, static_cast<ActuatorDriver>(driverCode));
    }
    if (driverCode < 60)
        return 3;

    {
        // Sensors used to skip validation entirely, so a bogus or already-taken
        // pin was accepted and only showed up as a driver that never reads.
        if (name.isEmpty())
            return 1;
        const SensorDriver sensorDriver = static_cast<SensorDriver>(driverCode);
        switch (sensorDriver)
        {
        case SensorDriver::DHT_11:
        case SensorDriver::DHT_21:
        case SensorDriver::DHT_22:
        case SensorDriver::DS18B20:
        case SensorDriver::DOOR:
        case SensorDriver::WINDOW:
        case SensorDriver::PIR:
        case SensorDriver::HCSR04:
        case SensorDriver::RAIN:
        case SensorDriver::LD2410:
        case SensorDriver::PZEM_004T_V03:
        case SensorDriver::PZEM_004T_V01:
            break;
        default:
            return 3;
        }
        if (!Sensor::isSupportedOnCurrentTarget(sensorDriver))
            return 3;
        if (!config.validSensorPin(sensorDriver, 0, input1))
            return 2;
        if (config.pinInUse(input1))
            return 5;
        const bool dualPin = driverCode == SensorDriver::HCSR04 ||
                             driverCode == SensorDriver::LD2410 ||
                             driverCode == SensorDriver::PZEM_004T_V03 ||
                             driverCode == SensorDriver::PZEM_004T_V01;
        if (dualPin)
        {
            if (!config.validSensorPin(sensorDriver, 1, input2))
                return 4;
            if (input1 == input2 || config.pinInUse(input2))
                return 5;
        }
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
        case SensorDriver::LD2410:
            prepareLD2410(name, input1, input2);
            break;
        case SensorDriver::PZEM_004T_V03:
        case SensorDriver::PZEM_004T_V01:
            preparePzem(name, input1, input2, Discovery::MODBUS_PZEM_ADDRESS_DEFAULT, (SensorDriver)driverCode);
            break;
        default:
            return 3;
        }
    }
    return 0;
}
int prepareVirtualSwitch(String name, unsigned int input1, unsigned int input2, ActuatorDriver driver)
{
    Actuator actuator;
    if (!setActuatorName(actuator, name))
        return 1;
    if (!isSupportedActuatorDriverCode(static_cast<int>(driver)))
        return 3;
    if (!config.validInputPin(input1))
        return 2;
    if (config.pinInUse(input1))
        return 5;
    actuator.driver = driver;
    actuator.inputs.push_back(input1);
    if (actuator.requireDualInputs() && !config.validInputPin(input2))
    {
        return 4;
    }
    else if (actuator.requireDualInputs())
    {
        if (input1 == input2 || config.pinInUse(input2))
            return 5;
        actuator.inputs.push_back(input2);
    }
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
bool templateSelect(enum Template _template)
{
    if (config.templateId != Template::NO_TEMPLATE)
        return false;
    if (_template < Template::NO_TEMPLATE || _template > Template::GARDEN)
        return false;
#ifdef DEBUG_ONOFRE
    Log.info("%s Template selected: %d" CR, tags::webserver, _template);
#endif
    clearLiveFeatureGraphSafely();
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
#ifdef ESP32C6
        // Five zones, each with its own wall button, plus a rain sensor. Mirrors
        // the wiring of the ESPHome unit this replaces, so a board can be
        // reflashed without touching the panel. Watering time per zone is the
        // actuator's own autoOff (seconds), which the loop already enforces.
        prepareActuator(I18N::VALVE_ONE, DefaultPins::OUTPUT_ONE, DefaultPins::INPUT_ONE, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_TWO, DefaultPins::OUTPUT_TWO, DefaultPins::INPUT_TWO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_THREE, DefaultPins::OUTPUT_VALVE_THREE, DefaultPins::INPUT_THREE, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_FOUR, DefaultPins::OUTPUT_VALVE_FOUR, DefaultPins::INPUT_FOUR, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_FIVE, DefaultPins::OUTPUT_VALVE_FIVE, DefaultPins::INPUT_FIVE, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareRain(I18N::RAIN_SENSOR_NAME, DefaultPins::RAIN_SENSOR);
#else
        prepareActuator(I18N::VALVE_ONE, DefaultPins::OUTPUT_ONE, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_TWO, DefaultPins::OUTPUT_TWO, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
#ifdef ESP8266
        prepareActuator(I18N::VALVE_THREE, DefaultPins::OUTPUT_VALVE_THREE, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
        prepareActuator(I18N::VALVE_FOUR, DefaultPins::OUTPUT_VALVE_FOUR, DefaultPins::noGPIO, ActuatorDriver::GARDEN_VALVE, ActuatorControlType::GPIO_OUTPUT);
#endif
#endif
        break;
    default:
        return false;
    }
    return true;
}
