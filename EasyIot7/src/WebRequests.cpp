#include "WebRequests.h"
#include "Config.h"
#include "constants.h"
#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include "Sensors.h"

void publishOnEmoncms(SensorT &sensor, String &readings)
{
    if (!sensor.emoncmsSupport)
        return;
    if (WiFi.status() != WL_CONNECTED || strlen(getAtualConfig().emoncmsServer) == 0 || strlen(getAtualConfig().emoncmsApikey) == 0)
        return;
    WiFiClient client;
    HTTPClient http;
    String queryString;
    queryString.concat(getAtualConfig().emoncmsServer);
    queryString.concat(getAtualConfig().emoncmsPath);
    queryString.concat("/input/post.json?node=");
    queryString.concat(sensor.name);
    queryString.concat("&apikey=");
    queryString.concat(getAtualConfig().emoncmsApikey);
    queryString.concat("&json=");
    queryString.concat(readings);
    if (http.begin(client, queryString))
    {
        int httpCode = http.GET();
        if (httpCode < 0)
        {
            Log.error("%s GET Request, error %s" CR, tags::emoncms, http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    else
    {
        Log.error("%s Unable to connect" CR, tags::emoncms);
    }
}
void controlMasterSwitch(const char *chipId, const char *switchId, const char *state)
{

    if (WiFi.status() != WL_CONNECTED)
        return;
    WiFiClient client;
    HTTPClient http;
    String queryString;
    queryString.concat("http://");
    queryString.concat(chipId); //CHIP ID
    //queryString.concat(".local");
    queryString.concat("/state-switch?id=");
    queryString.concat(switchId); //SWICH ID
    queryString.concat("&state=");
    queryString.concat(state); //STATE
    Serial.println(queryString);
    if (http.begin(client, queryString))
    {
        int httpCode = http.GET();
        if (httpCode < 0)
        {
            Log.error("%s GET Request, error %s" CR, tags::emoncms, http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    else
    {
        Log.error("%s Unable to connect" CR, tags::emoncms);
    }
}