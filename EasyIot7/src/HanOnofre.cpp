#include "HanOnofre.hpp"
#include <ModbusMaster.h>
ModbusMaster node;

void configHan()
{
    Serial1.begin(9600, SERIAL_8N2, 13, 14);
    node.begin(1, Serial1);
}

void readHan()
{

    uint8_t result;
    double volt = 0;
    double power = 0;
    unsigned long freq;
    uint32_t activeEnergyImport;
    double current;
    /*  node.clearResponseBuffer();
     result = node.readInputRegisters(0x006c, 2);
     if (result == node.ku8MBSuccess)
     {
         volt = node.getResponseBuffer(0) / 10.0;
         current = node.getResponseBuffer(1) / 10.0;
     }
     Serial.print("Voltage: ");
     Serial.print(volt);
     Serial.println("V");
     node.clearResponseBuffer();
     result = node.readInputRegisters(0x0079, 1);
     if (result == node.ku8MBSuccess)
     {
         power = node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16;
     }
     Serial.print("Instantaneous Active power: ");
     Serial.print(power);
     Serial.println("W");
     result = node.readInputRegisters(0x006D, 1);

     Serial.print("Instantaneous Current: ");
     Serial.print(current);
     Serial.println("A");
     node.clearResponseBuffer();
     result = node.readInputRegisters(0x007F, 1);
     if (result == node.ku8MBSuccess)
     {
         freq = node.getResponseBuffer(0) / 10.0;
     }
     Serial.print("Instantaneous Frequency: ");
     Serial.print(freq);
     Serial.println("Hz");
     node.clearResponseBuffer();
     result = node.readInputRegisters(0x0016, 1);
     Serial.println(result);
     if (result == node.ku8MBSuccess)
     {
         activeEnergyImport = node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16;
         Serial.print("Active energy import: ");
         Serial.print(activeEnergyImport);
         Serial.println("Wh");
     }*/
    result = node.readInputRegisters(0x0016, 1);
    if (result == node.ku8MBSuccess)
    {
        Serial.print((node.getResponseBuffer(1) | node.getResponseBuffer(0) << 16) / 1000.0);
        Serial.println("kWh");
    }
    delay(5000);
}
