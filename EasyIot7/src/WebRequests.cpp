#include "WebRequests.h"
#include "Config.h"
#include "constants.h"
#include "CoreWiFi.h"

#include "Sensors.h"
#include "Switches.h"
#include "ArduinoJson.h"
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
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
    String name = String(sensor.name);
    normalize(name);
    queryString.concat(name);
    queryString.concat("&apikey=");
    queryString.concat(getAtualConfig().emoncmsApikey);
    queryString.concat("&json=");
    queryString.concat(readings);
    if (http.begin(client, queryString))
    {
        int httpCode = http.GET();

        if (httpCode < 0)
        {
#ifdef DEBUG_ONOFRE
            Log.error("%s GET Request, error %s" CR, tags::emoncms, http.errorToString(httpCode).c_str());
#endif
        }

        http.end();
    }
#ifdef DEBUG_ONOFRE
    else
    {
        Log.error("%s Unable to connect" CR, tags::emoncms);
    }
#endif
}
