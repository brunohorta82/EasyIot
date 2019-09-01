#ifndef EMONCMS_H
#define EMONCMS_H
struct SensorT;
class String;
void publishOnEmoncms(SensorT &sensor, String &readings);
#endif