#ifndef WEB_REQUESTS_H
#define WEB_REQUESTS_H
struct SensorT;
class String;
void publishOnEmoncms(SensorT &sensor, String &readings);
void controlMasterSwitch(const char *chipId, const char *switchId, const char *state);
#endif