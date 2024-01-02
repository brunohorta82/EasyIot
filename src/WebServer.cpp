#include "WebServer.h"
#include <DNSServer.h>
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>

#include <ConfigOnofre.h>
#include "Templates.h"
// STATIC WEBPANEL
#include "CaptivePortal.h"
#include "DevicesHtml.h"
#include "NodeHtml.h"
#include "IndexHtml.h"
#include "StylesMinCss.h"
#include "IndexJs.h"

#ifdef ESP32
#include <WebServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "Update.h"
#endif
#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#endif
extern ConfigOnofre config;

DNSServer dnsServer;
static AsyncWebServer server(80);
static AsyncEventSource events("/events");

void performUpdate()
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Starting auto update make sure if this device is connected to the internet.", tags::system);
#endif
  WiFiClient client;
  t_httpUpdate_return ret;
#ifdef ESP8266
  ret = ESPhttpUpdate.update(client, constanstsCloudIO::otaUrl, String(VERSION));
#endif
#ifdef ESP32
  ret = httpUpdate.update(client, constanstsCloudIO::otaUrl, String(VERSION));
#endif
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
#ifdef DEBUG_ONOFRE
#ifdef ESP8266
    Log.notice("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#endif
#ifdef ESP32
    Log.notice("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
#endif
#endif
    break;
  case HTTP_UPDATE_NO_UPDATES:
#ifdef DEBUG_ONOFRE
    Log.notice("HTTP_UPDATE_NO_UPDATES");
#endif
    break;
  case HTTP_UPDATE_OK:
#ifdef DEBUG_ONOFRE
    Log.notice("HTTP_UPDATE_OK");
#endif
    break;
  }
}

class CaptiveRequestHandler : public AsyncWebHandler
{
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request)
  {
    return true;
  }
  void restart()
  {
    config.requestRestart();
  }
  void handleRequest(AsyncWebServerRequest *request)
  {

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    bool store = false;
    response->print(FPSTR(HTTP_HEADER));
    response->print(FPSTR(HTTP_SCRIPT));
    response->print(FPSTR(HTTP_STYLE));
    response->print(FPSTR(HTTP_HEADER_END));
    if (request->hasArg("s") && request->hasArg("i") && request->arg("s").length() > 0 && request->arg("i").length() > 0 && request->arg("t").length() > 0)
    {
      String n_name = config.chipId;
      if (request->hasArg("i"))
      {
        n_name = String(request->arg("i"));
        normalize(n_name);
        if (n_name.isEmpty())
          n_name = config.chipId;
      }

      strlcpy(config.nodeId, n_name.c_str(), sizeof(config.nodeId));
      strlcpy(config.wifiSSID, request->arg("s").c_str(), sizeof(config.wifiSSID));
      if (request->hasArg("t"))
      {
        config.pauseFeatures();
        config.loadTemplate(request->arg("t").toInt());
      }

      if (request->hasArg("p"))
      {
        strlcpy(config.wifiSecret, request->arg("p").c_str(), sizeof(config.wifiSecret));
      }
      else
      {
        strlcpy(config.wifiSecret, "", sizeof(config.wifiSecret));
      }
      String storedR = FPSTR(HTTP_SAVED);
      storedR.replace("{o}", String("http://" + String(config.nodeId) + ".local").c_str());
      response->print(storedR.c_str());
      response->print(FPSTR(HTTP_END));
      request->send(response);
      store = true;
    }

    if (request->hasArg("sc"))
    {
      int n = WiFi.scanComplete();
      if (n == -2)
      {
        WiFi.scanNetworks(true);
      }
      else if (n)

      {
        int indices[n];
        for (int i = 0; i < n; i++)
        {
          indices[i] = i;
        }
        for (int i = 0; i < n; i++)
        {
          for (int j = i + 1; j < n; j++)
          {
            if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
            {
              std::swap(indices[i], indices[j]);
            }
          }
        }

        // display networks in page
        String scan = "<div class=\"sc\">";
        for (int i = 0; i < n; i++)
        {
          if (indices[i] == -1)
            continue; // skip dups
          String item = FPSTR(HTTP_ITEM);
          String rssiQ;
          rssiQ += rssiToWiFiQuality(WiFi.RSSI(indices[i]));
          item.replace("{v}", WiFi.SSID(indices[i]));
          item.replace("{r}", rssiQ);

#ifdef ESP32
          uint8_t encType = WIFI_AUTH_OPEN;
#endif
#ifdef ESP8266
          uint8_t encType = ENC_TYPE_NONE;
#endif
          if (WiFi.encryptionType(indices[i]) != encType)
          {
            item.replace("{i}", "l");
          }
          else
          {
            item.replace("{i}", "");
          }
          scan += item;
        }
        WiFi.scanDelete();
        if (WiFi.scanComplete() == -2)
        {
          WiFi.scanNetworks(true);
        }
        scan += "</div>";
        response->print(scan.c_str());
      }
    }
    if (!store)
    {
      String form = FPSTR(HTTP_FORM_START);
      form.replace("{n}", config.nodeId);
      if (config.templateId == Template::NO_TEMPLATE)
      {
        form.replace("class='hide'", "");
      }
      response->print(form);
      response->print(FPSTR(HTTP_END));
      request->send(response);
    }
    if (store)
    {
      config.save().requestRestart();
    }
  }
};

AsyncJsonResponse *errorResponse(const char *cause)
{
  AsyncJsonResponse *responseError = new AsyncJsonResponse();
  JsonVariant &root = responseError->getRoot();
  root["cause"] = cause;
  responseError->setCode(400);
  responseError->setLength();
  return responseError;
}

void loadWebPanel()
{
  // HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
#if WEB_SECURE_ON
              if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
                return request->requestAuthentication(REALM);
#endif
              AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, sizeof(index_html));
              response->addHeader("Content-Encoding", "gzip");
              response->addHeader("Cache-Control", "max-age=600");
              request->send(response); });

  server.on("/node.html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", node_html, sizeof(node_html));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response); });

  server.on("/devices.html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", devices_html, sizeof(devices_html));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response); });

  // JS
  server.on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", index_js, sizeof(index_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response); });

  // CSS
  server.on("/css/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", styles_min_css, sizeof(styles_min_css));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response); });
}

void loadAPI()
{
  /*GET CONFIG*/
  server
      .on("/config", HTTP_GET, [](AsyncWebServerRequest *request)
          {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    config.json(root);
    response->setLength();
    request->send(response); });

  /*SAVE CONFIG*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    JsonObject configJson = json.as<JsonObject>();
    config.update(configJson).json(root);
    response->setLength();
    request->send(response); }));

  /*CREATE NEW VIRTUAL SENSOR*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/switches/virtual", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    JsonObject actuatorJson = json.as<JsonObject>();
    config.pauseFeatures();
    int result =  prepareVirtualSwitch(actuatorJson["name"] | "", actuatorJson["input1"] |  DefaultPins::noGPIO, actuatorJson["input2"] | DefaultPins::noGPIO, actuatorJson["driver"] | ActuatorDriver::INVALID);
      if(result == 0){
        config.save().json(root);
      }else{
      response->setCode(400);
      root["result"] = result;
      }
      config.resumeFeatures();
      response->setLength();
      request->send(response); }));
  /*CREATE NEW VIRTUAL SENSOR*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/sensors", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    JsonObject actuatorJson = json.as<JsonObject>();
    config.pauseFeatures();
    int result = prepareSensor(actuatorJson["name"] | "", actuatorJson["input1"] | DefaultPins::noGPIO, actuatorJson["input2"] | DefaultPins::noGPIO, actuatorJson["driver"] | SensorDriver::INVALID_SENSOR);
    if (result == 0)
    {
      config.save().json(root);
      }else{
      response->setCode(400);
      root["result"] = result;
      }
      config.resumeFeatures();
      response->setLength();
      request->send(response); }));

  /*CONTROL ACTUATOR*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/actuators/control", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    JsonObject action = json.as<JsonObject>();
    config.controlFeature(StateOrigin::WEBPANEL,action,root);
    response->setLength();
    request->send(response); }));

  /*REBOOT DEVICE*/
  server
      .on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
          {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    root["result"] = "Reboot requested";
    response->setLength();
    request->send(response);
    config.requestRestart(); });

  /*CHANGE FEATURES TEMPLATE*/
  server
      .on("/templates/change", HTTP_GET, [](AsyncWebServerRequest *request)
          {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    root["result"] = "Template changed";
    response->setLength();
    request->send(response);
    templateSelect((Template)request->getParam("t")->value().toInt());
    config.save(); });

  server
      .on("/load-defaults", HTTP_GET, [](AsyncWebServerRequest *request)
          {
#if WEB_SECURE_ON
    if (!request->authenticate(config.apiUser, config.apiPassword, REALM))
      return request->requestAuthentication(REALM);
#endif
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    root["result"] = "Load defaults requested";
    response->setLength();
    request->send(response);
    config.requestLoadDefaults(); });

  server
      .on("/auto-update", HTTP_POST, [](AsyncWebServerRequest *request)
          {
#if WEB_SECURE_ON
       if (!request->authenticate(config.apiUser, config.apiPassword,REALM))
       return request->requestAuthentication(REALM);
#endif
       AsyncWebServerResponse *response = request->beginResponse(200, "text/html", REDIRECT_PAGE);
       response->addHeader("Connection", "close");
       request->send(response);
       config.requestAutoUpdate(); });

  server
      .on(
          "/update", HTTP_POST, [](AsyncWebServerRequest *request)
          {
#if WEB_SECURE_ON
       if (!request->authenticate(config.apiUser, config.apiPassword,REALM))
       return request->requestAuthentication(REALM);
#endif
     bool error = Update.hasError();
     if(error)
       config.requestRestart();
     AsyncWebServerResponse *response = request->beginResponse(200, "text/html", !error? REDIRECT_PAGE : UPDATE_FAILED);
     response->addHeader("Connection", "close");
     request->send(response); },
          [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
          {
            if (!index)
            {
#ifdef DEBUG_ONOFRE
              Log.notice("%s Update Start: %s" CR, tags::system, filename.c_str());
#endif
#ifdef ESP8266
              Update.runAsync(true);
#endif
              if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
              {
                Update.printError(Serial);
              }
            }
            if (!Update.hasError())
            {
              if (Update.write(data, len) != len)
              {
                Update.printError(Serial);
              }
            }
            if (final)
            {
              if (Update.end(true))
              {
#ifdef DEBUG_ONOFRE
                Log.notice("%s Update Success: %d" CR, tags::system, index + len);
#endif
                config.requestRestart();
              }
              else
              {
                config.requestRestart();
              }
            }
          });
}

void setupCaptivePortal()
{
  server.reset();
  WiFi.scanNetworks(true);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
}
void stopWebserver()
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s WEBSERVER STOP" CR, tags::system);
#endif
  DefaultHeaders::Instance().end();
  server.end();
}
void startWebserver()
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s WEBSERVER START" CR, tags::system);
#endif
  server.begin();
}
void setupCors()
{
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), F("POST,PUT,DELETE,GET"));
  DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), F("Authorization, Content-Type, Origin, Referer, User-Agent"));
}
void setupWebPanel()
{
  server.reset();
  server.addHandler(&events);
  loadWebPanel();
  loadAPI();
  server.onNotFound([](AsyncWebServerRequest *request)
                    {
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      request->send(404);
    } });
}

void sendToServerEvents(String topic, String payload)
{
  if (events.count() > 0)
    events.send(payload.c_str(), topic.c_str(), millis());
}

void webserverServicesLoop()
{
#ifdef ESP32
  if (WiFi.getMode() == WIFI_MODE_APSTA || WiFi.getMode() == WIFI_MODE_AP)
#endif
#ifdef ESP8266
    if (WiFi.getMode() == WIFI_AP_STA || WiFi.getMode() == WIFI_AP)
#endif
      dnsServer.processNextRequest();
    else
      dnsServer.stop();
}