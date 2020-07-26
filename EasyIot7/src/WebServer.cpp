#include "WebServer.h"
#include "constants.h"

#include "AsyncJson.h"
#include "Switches.h"
#include "Sensors.h"
#include "StaticSite.h"
#include "StaticCss.h"
#include "StaticJs.h"

#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#if EMULATE_ALEXA
#include <fauxmoESP.h>
#endif
#include <Config.h>
#include "WiFi.h"

// SKETCH BEGIN
static AsyncWebServer server(80);
static AsyncEventSource events("/events");
#if EMULATE_ALEXA
static fauxmoESP fauxmo;
#endif

void loadUI()
{
  //HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, sizeof(index_html));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");

    request->send(response);
  });

  server.on("/integrations.html", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", integrations_html, sizeof(integrations_html));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response);
  });

  server.on("/node.html", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", node_html, sizeof(node_html));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response);
  });

  server.on("/wifi.html", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", wifi_html, sizeof(wifi_html));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response);
  });

  server.on("/devices.html", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", devices_html, sizeof(devices_html));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response);
  });

  //JS
  server.on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", index_js, sizeof(index_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response);
  });

  server.on("/js/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", jquery_min_js, sizeof(jquery_min_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response);
  });

  //CSS
  server.on("/css/styles.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", styles_min_css, sizeof(styles_min_css));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response);
  });
}
void setupWebserverAsync()
{
  server.addHandler(&events);
  loadUI();

  //-------- API ---------

  //SYSTEM
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    requestRestart();
    request->send(200, "application/json", "{\"result\":\"OK\"}");
  });
  server.on("/load-defaults", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    request->send(200, "application/json", "{\"result\":\"OK\"}");
    requestLoadDefaults();
  });
  server.on("/system-status", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    systemJSONStatus(*response);
    request->send(response);
  });
  server.on("/auto-update", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    request->send(200, "application/json", "{\"result\":\"OK\"}");
    requestAutoUpdate();
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    bool error = Update.hasError();
    if(error){
      requestRestart();
      }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", !error? "<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <title>Update</title> <style>body{background-color: rgb(34, 34, 34); color: white; font-size: 18px; padding: 10px; font-weight: lighter;}</style> <script type=\"text/javascript\">function Redirect(){window.location=\"/\";}document.write(\"Update successfully, will be redirected automatically in 20 seconds. Please Wait...\"); setTimeout('Redirect()', 20000); </script></head><body></body></html>":"<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <title>Atualização</title> <style>body{background-color: #cc0000; color: white; font-size: 18px; padding: 10px; font-weight: lighter;}</style> <script type=\"text/javascript\">function Redirect(){window.location=\"/\";}document.write(\"Update failed, it may be necessary to manually reset the device and try again.\"); setTimeout('Redirect()', 10000); </script></head><body></body></html>");
    response->addHeader("Connection", "close");
    request->send(response); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(!index){
#ifdef DEBUG
      Log.notice("%s Update Start: %s" CR, tags::system ,filename.c_str());
#endif
      Update.runAsync(true);
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
#ifdef DEBUG
        Log.notice("%s Update Success: %d" CR, tags::system, index+len);
#endif
        requestRestart();
      } else {
         requestRestart();
      }
    } });
  //CONFIG
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    getAtualConfig().serializeToJson(*response);

    request->send(response);
  });

  //CONFIG
  server.on("/assign", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    if (request->hasArg("configKey"))
    {
      strlcpy(getAtualConfig().configkey, request->arg("configKey").c_str(), sizeof(getAtualConfig().configkey));
      response->setCode(200);
      requestCloudIOSync();
    }
    else
    {
      response->setCode(400);
    }

    request->send(response);
  });

  server.addHandler(new AsyncCallbackJsonWebHandler("/save-config", [](AsyncWebServerRequest *request, JsonVariant json) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    getAtualConfig().updateFromJson(json).saveConfigurationOnDefaultFile().serializeToJson(*response);
    requestCloudIOSync();
    request->send(response);
  }));

  //FEATURES
  server.on("/switches", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    getAtualSwitchesConfig().serializeToJson(*response);
    request->send(response);
  });

  server.addHandler(new AsyncCallbackJsonWebHandler("/save-switch", [](AsyncWebServerRequest *request, JsonVariant json) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    if (!request->hasArg("id"))
    {
      request->send(400, "Invalid id");
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    update(getAtualSwitchesConfig(), request->arg("id").c_str(), json);
    serializeJson(json, *response);
    requestCloudIOSync();
    request->send(response);
  }));

  server.on("/remove-switch", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    if (request->hasArg("id"))
    {
      remove(getAtualSwitchesConfig(), request->arg("id").c_str());
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    getAtualSwitchesConfig().serializeToJson(*response);
        requestCloudIOSync();
    request->send(response);
  });

  server.on("/state-switch", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    if (request->hasArg("id") && request->hasArg("state"))
    {
      stateSwitchById(getAtualSwitchesConfig(), request->arg("id").c_str(), request->arg("state").c_str());
      request->send(200, "application/json", "{\"result\":\"OK\"}");
    }
    else
    {
      request->send(400, "application/json", "{\"result\":\"MISSING PARAMS\"}");
    }
  });

  server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    getAtualSensorsConfig().serializeToJson(*response);
    request->send(response);
  });

  server.addHandler(new AsyncCallbackJsonWebHandler("/save-sensor", [](AsyncWebServerRequest *request, JsonVariant json) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    if (!request->hasArg("id"))
    {
      request->send(400, "Invalid id");
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    update(getAtualSensorsConfig(), request->arg("id").c_str(), json);
    serializeJson(json, *response);
    requestCloudIOSync();
    request->send(response);
  }));

  server.on("/remove-sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
#if WEB_SECURE_ON
    if (!request->authenticate(getAtualConfig().apiUser, getAtualConfig().apiPassword))
      return request->requestAuthentication();
#endif
    if (request->hasArg("id"))
    {
      remove(getAtualSensorsConfig(), request->arg("id").c_str());
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    getAtualSensorsConfig().serializeToJson(*response);
        requestCloudIOSync();
    request->send(response);
  });
#if EMULATE_ALEXA
  //ALEXA SUPPORT
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data)))
      return;
  });
#endif
  server.onNotFound([](AsyncWebServerRequest *request) {
#if EMULATE_ALEXA
    //ALEXA
    String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
    if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body))
      return;
#endif
    //CORS
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      request->send(404);
    }
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), F("POST, PUT, GET"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("Content-Type, Origin, Referer, User-Agent"));
  server.begin();
#if EMULATE_ALEXA
  startAlexaDiscovery();
#endif
}
#if EMULATE_ALEXA
void addSwitchToAlexa(const char *name)
{
  fauxmo.removeDevice(name);
  fauxmo.addDevice(name);
}
void removeSwitchFromAlexa(const char *name)
{
  fauxmo.removeDevice(name);
}
#endif
void sendToServerEvents(const String &topic, const char *payload)
{
  events.send(payload, topic.c_str(), millis());
}

void webserverServicesLoop()
{
#if EMULATE_ALEXA
  fauxmo.handle();
#endif
}