#ifndef WEB_REQUESTS_H
#define WEB_REQUESTS_H
struct SensorT;
class String;
void publishOnEmoncms(SensorT &sensor, String &readings);
#endif