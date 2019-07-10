#include "WebServer.h"
#define WEBSERVER_TAG "[WEBSERVER]"
// SKETCH BEGIN
AsyncWebServer server(80);
fauxmoESP fauxmo;

void loadUI()
{
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
  /** JS    **/

  server.on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/js", index_js, sizeof(index_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Expires", "Mon, 1 Jan 2222 10:10:10 GMT");
    request->send(response);
  });
  server.on("/js/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/js", jquery_min_js, sizeof(jquery_min_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Expires", "Mon, 1 Jan 2222 10:10:10 GMT");
    request->send(response);
  });

  /** CSS   **/
  server.on("/css/AdminLTE.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", AdminLTE_min_css, sizeof(AdminLTE_min_css));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Expires", "Mon, 1 Jan 2222 10:10:10 GMT");
    request->send(response);
  });
}

void setupWebserverAsync(){
  MDNS.begin(getAtualConfig().hostname);
  MDNS.addService("bhsystems", "tcp", 80);
  MDNS.addServiceTxt("bhsystems", "tcp", "nodeId", String(getAtualConfig().nodeId));
  MDNS.addServiceTxt("bhsystems", "tcp", "hardwareId", String(ESP.getChipId()));
  MDNS.addServiceTxt("bhsystems", "tcp", "type", String(FACTORY_TYPE));
  MDNS.addServiceTxt("bhsystems", "tcp", "wifiSignal", String(WiFi.RSSI()));
  MDNS.addServiceTxt("bhsystems", "tcp", "ssid", String(getAtualConfig().apName));
  MDNS.addServiceTxt("bhsystems", "tcp", "firmware", String(FIRMWARE_VERSION));
  loadUI();

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    requestWifiScan();
    request->send(200, "application/json", "{\"result\":\"OK\"}");
  });
  server.on("/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(wifiJSONStatus());
    request->send(response);
  });
    server.on("/switchs", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(getSwitchesConfigStatus());
    request->send(response);
  });
   server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(getConfigStatus());
    request->send(response);
  });
   AsyncCallbackJsonWebHandler *handlerNode = new AsyncCallbackJsonWebHandler("/save-config", [](AsyncWebServerRequest *request, JsonVariant json) {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      //SAVE CONFIG
      updateConfig(json, true);
      response->print(getConfigStatus());
      request->send(response);
    
  });

  server.addHandler(handlerNode);
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), F("PUT, GET"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("Content-Type, Origin, Referer, User-Agent"));
  server.begin();
}



void mDnsLoop()
{
  MDNS.update();
}