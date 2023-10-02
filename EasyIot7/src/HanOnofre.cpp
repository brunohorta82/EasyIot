#include <ModbusMaster.h>
ModbusMaster node;

void config()
{
    Serial1.begin(9600, SERIAL_8N2, 13, 14);
    node.begin(1, Serial1);
}

void read()
{

    uint8_t result;
    double volt = 0;
    double power = 0;
    unsigned long freq;
    double current;
    result = node.readInputRegisters(0x006c, 1);
    if (result == node.ku8MBSuccess)
    {
        volt = node.getResponseBuffer(0) / 10.0;
    }
    Serial.print("Voltage: ");
    Serial.print(volt);
    Serial.println("V");
    result = node.readInputRegisters(0x0079, 1);
    if (result == node.ku8MBSuccess)
    {
        power = node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16;
    }
    Serial.print("Instantaneous Active power: ");
    Serial.print(power);
    Serial.println("W");
    result = node.readInputRegisters(0x006D, 1);
    if (result == node.ku8MBSuccess)
    {
        current = node.getResponseBuffer(0) / 10.0;
    }
    Serial.print("Instantaneous Current: ");
    Serial.print(current);
    Serial.println("A");
    result = node.readInputRegisters(0x007F, 1);
    if (result == node.ku8MBSuccess)
    {
        freq = node.getResponseBuffer(0) / 10.0;
    }
    Serial.print("Instantaneous Frequency: ");
    Serial.print(freq);
    Serial.println("Hz");
    result = node.readInputRegisters(0x0001, 1);
    if (result == node.ku8MBSuccess)
    {
        freq = node.getResponseBuffer(0) / 10.0;
    }
    Serial.print("Instantaneous Frequency: ");
    Serial.print(freq);
    Serial.println("Hz");
    delay(5000);
}
