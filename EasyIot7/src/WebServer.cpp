#include "WebServer.h"
#include "constants.h"

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncEventSource events("/events");
fauxmoESP fauxmo;

void loadUI()
{
  //HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, sizeof(index_html));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/firmware.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", firmware_html, sizeof(firmware_html));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/mqtt.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", mqtt_html, sizeof(mqtt_html));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/node.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", node_html, sizeof(node_html));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/wifi.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", wifi_html, sizeof(wifi_html));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/devices.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", devices_html, sizeof(devices_html));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  //JS
  server.on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", index_js, sizeof(index_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Expires", "Mon, 1 Jan 2222 10:10:10 GMT");
    request->send(response);
  });

  server.on("/js/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", jquery_min_js, sizeof(jquery_min_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Expires", "Mon, 1 Jan 2222 10:10:10 GMT");
    request->send(response);
  });

  //CSS
  server.on("/css/styles.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css",styles_min_css, sizeof(styles_min_css));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Expires", "Mon, 1 Jan 2222 10:10:10 GMT");
    request->send(response);
  });
}

void startAlexaDiscovery()
{
  fauxmo.createServer(false);
  fauxmo.setPort(80); // required for gen3 devices
  fauxmo.enable(true);
  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
    String valueStr =state ? String(map((int)value,0,255,0,100)+1) : "0";
    Log.notice("%s Device id: %s, name: %s, value: %s, state: %t" CR, tags::alexa, device_id,device_name,valueStr.c_str(),state);
    stateSwitchByName(device_name, state ? "ON" : "OFF",valueStr.c_str() );
  });
}

void setupWebserverAsync()
{
  MDNS.begin(getAtualConfig().nodeId);
  MDNS.addService("easyiot", "tcp", 80);
  MDNS.addServiceTxt("easyiot", "tcp", "nodeId", String(getAtualConfig().nodeId));
  MDNS.addServiceTxt("easyiot", "tcp", "hardwareId", String(ESP.getChipId()));
  MDNS.addServiceTxt("easyiot", "tcp", "wifiSignal", String(WiFi.RSSI()));
  MDNS.addServiceTxt("easyiot", "tcp", "firmware", String(VERSION));
  server.addHandler(&events);
  loadUI();

  //-------- API ---------

  //SYSTEM
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    requestRestart();
    request->send(200, "application/json", "{\"result\":\"OK\"}");
  });
  server.on("/load-defaults", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"result\":\"OK\"}");
    requestLoadDefaults();
  });
  server.on("/system-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    systemJSONStatus(*response);
    request->send(response);
  });
  server.on("/auto-update", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"result\":\"OK\"}");
    requestAutoUpdate();
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    bool error = Update.hasError();
    if(error){
      requestRestart();
      }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", !error? "<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <title>Update</title> <style>body{background-color: rgb(34, 34, 34); color: white; font-size: 18px; padding: 10px; font-weight: lighter;}</style> <script type=\"text/javascript\">function Redirect(){window.location=\"/\";}document.write(\"Update successfully, will be redirected automatically in 20 seconds. Please Wait...\"); setTimeout('Redirect()', 20000); </script></head><body></body></html>":"<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <title>Atualização</title> <style>body{background-color: #cc0000; color: white; font-size: 18px; padding: 10px; font-weight: lighter;}</style> <script type=\"text/javascript\">function Redirect(){window.location=\"/\";}document.write(\"Update failed, it may be necessary to manually reset the device and try again.\"); setTimeout('Redirect()', 10000); </script></head><body></body></html>");
    response->addHeader("Connection", "close");
    request->send(response); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(!index){
      Log.notice("%s Update Start: %s" CR, tags::system ,filename.c_str());
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
        Log.notice("%s Update Success: %d" CR, tags::system, index+len);
        requestRestart();
      } else {
         requestRestart();
      }
    } });
  //CONFIG
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeConfigStatus(*response);
    request->send( response);
  });
  server.addHandler(new AsyncCallbackJsonWebHandler("/save-config", [](AsyncWebServerRequest *request, JsonVariant json) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    updateConfig(json, true);
    serializeConfigStatus(*response);
    request->send( response);
  }));
  //FEATURES
  server.on("/switches", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
   serializeFile(SWITCHES_CONFIG_FILENAME,*response);
    request->send( response);
  });
  server.addHandler(new AsyncCallbackJsonWebHandler("/save-switch", [](AsyncWebServerRequest *request, JsonVariant json) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    updateSwitch(json,true);
    serializeJson(json,*response);
    request->send(response);
  }));
  server.on("/remove-switch", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasArg("id"))
    {
      removeSwitch(request->arg("id").c_str(), true);
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeFile(SWITCHES_CONFIG_FILENAME,*response);
    request->send(response);
  });
  server.on("/state-switch", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasArg("id") && request->hasArg("state"))
    {
      stateSwitchById(request->arg("id").c_str(), request->arg("state").c_str());
      request->send(200, "application/json", "{\"result\":\"OK\"}");
    }
    else
    {
      request->send(400, "application/json", "{\"result\":\"MISSING PARAMS\"}");
    }
  });
server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeFile(SENSORS_CONFIG_FILENAME,*response);
    request->send(response);
  });
  server.addHandler(new AsyncCallbackJsonWebHandler("/save-sensor", [](AsyncWebServerRequest *request, JsonVariant json) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    updateSensor(json,true);
    serializeJson(json,*response);
    request->send(response);
  }));
  server.on("/remove-sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasArg("id"))
    {
      removeSensor(request->arg("id").c_str(), true);
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeFile(SENSORS_CONFIG_FILENAME,*response);
    request->send(response);
  });
  //ALEXA SUPPORT
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data)))
      return;
  });
  server.onNotFound([](AsyncWebServerRequest *request) {
    //ALEXA
    String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
    if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body))
      return;
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
  startAlexaDiscovery();
}
unsigned char addSwitchToAlexa(char *name){
    fauxmo.removeDevice(name);
   return fauxmo.addDevice(name);
}
void removeSwitchFromAlexa( char* name){
  fauxmo.removeDevice(name);
}
void sendToServerEvents(String topic, String payload)
{
  events.send(payload.c_str(), topic.c_str(), millis());
}
void webserverServicesLoop()
{
  MDNS.update();
  fauxmo.handle();
}