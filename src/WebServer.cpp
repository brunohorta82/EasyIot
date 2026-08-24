#include "WebServer.h"
#include "CloudIO.h"
#include <cstdlib>
#include <DNSServer.h>
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>

#include <ConfigOnofre.h>
#include "Templates.h"
#include "Irrigation.h"
#include "DeviceLog.h"
// STATIC WEBPANEL
#include "CaptivePortal.h"
#include "IndexHtml.h"
#include "StylesMinCss.h"
#include "IndexJs.h"

#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "Update.h"
#endif
#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <WiFiClientSecureBearSSL.h>
#endif
extern ConfigOnofre config;

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncEventSource events("/events");

/**
 * Serve a PROGMEM blob (the gzipped web panel) through whichever
 * ESPAsyncWebServer is linked.
 *
 * The archived me-no-dev library only offers beginResponse_P; the maintained
 * ESP32Async fork dropped the _P variants, because on an ESP32 the flash is
 * memory-mapped and they no longer buy anything. Detecting the fork by its
 * version header keeps one source building against both, which matters while
 * the ESP8266 targets stay on the old one.
 */
static AsyncWebServerResponse *beginProgmemResponse(AsyncWebServerRequest *request, int code,
                                                   const char *contentType,
                                                   const uint8_t *content, size_t len)
{
#if defined(ASYNCWEBSERVER_VERSION_MAJOR)
  return request->beginResponse(code, contentType, content, len);
#else
  return request->beginResponse_P(code, contentType, content, len);
#endif
}

/* An over-the-air update used to be invisible: the panel asked for it, the reply
   came back immediately, and whatever happened next happened in the main loop with
   nobody watching. A failure looked exactly like a button that did nothing. This
   keeps enough state for the panel to show progress and, more importantly, to show
   the reason when it fails. */
OtaStatus otaStatus;

void otaStatusJson(JsonVariant &root)
{
  JsonObject ota = root["ota"].to<JsonObject>();
  switch (otaStatus.state)
  {
  case OtaState::RUNNING:
    ota["state"] = "running";
    break;
  case OtaState::FAILED:
    ota["state"] = "failed";
    break;
  case OtaState::DONE:
    ota["state"] = "done";
    break;
  default:
    ota["state"] = "idle";
  }
  ota["percent"] = otaStatus.percent;
  if (otaStatus.error[0] != '\0')
    ota["error"] = otaStatus.error;
}

namespace
{
char webApiUser[sizeof(config.apiUser)] = {};
char webApiPassword[sizeof(config.apiPassword)] = {};

struct ManualUpdateState
{
  bool ownsUpdate;
  bool ownsFeatureAccess;
  bool authenticated;
  bool busy;
  bool failed;
  bool finalSeen;
  bool success;
};

AsyncWebServerRequest *manualUpdateOwner = nullptr;

ManualUpdateState *manualUpdateState(AsyncWebServerRequest *request)
{
  return static_cast<ManualUpdateState *>(request->_tempObject);
}

void abortManualUpdate()
{
#ifdef ESP8266
  // ESP8266's Updater has no public abort(). end(false) resets an incomplete,
  // failed or empty update and leaves the previous firmware bootable.
  (void)Update.end(false);
#else
  Update.abort();
#endif
}

bool isCaptiveTemplateAllowed(int templateId)
{
  // Keep this list aligned with the choices rendered by the non-HAN captive
  // form. A contiguous enum range would also admit HAN_MODULE, which is not a
  // valid user-selectable template on a general-purpose build.
  switch (static_cast<Template>(templateId))
  {
  case Template::NO_TEMPLATE:
  case Template::DUAL_LIGHT:
  case Template::DUAL_SWITCH:
  case Template::COVER:
  case Template::GARAGE:
  case Template::GARDEN:
    return true;
  default:
    return false;
  }
}
} // namespace

AutoUpdateResult performUpdate()
{
#ifdef DEBUG_ONOFRE
  Log.notice("%s Starting auto update make sure if this device is connected to the internet.", tags::system);
#endif
  otaStatus.state = OtaState::RUNNING;
  deviceLog("atualizacao pedida");
  otaStatus.percent = 0;
  otaStatus.error[0] = '\0';
#ifdef ESP8266
  ESPhttpUpdate.onProgress([](int done, int total)
                           { otaStatus.percent = total > 0 ? (int)((int64_t)done * 100 / total) : 0; });
#else
  httpUpdate.onProgress([](int done, int total)
                        { otaStatus.percent = total > 0 ? (int)((int64_t)done * 100 / total) : 0; });
#endif
  const String otaUrl = String(constanstsCloudIO::otaUrl);
  const bool useHttps = otaUrl.startsWith("https://");
  t_httpUpdate_return ret;
#ifdef ESP8266
  int otaTlsErrorCode = 0;
  char otaTlsError[sizeof(otaStatus.error)] = {};
#endif
  if (useHttps)
  {
#ifdef ESP8266
    if (ESP.getFreeHeap() >= constanstsCloudIO::otaTlsMinimumFreeHeap &&
        ESP.getMaxFreeBlockSize() >= constanstsCloudIO::otaTlsMinimumMaxBlock)
    {
#ifdef DEBUG_ONOFRE
      const uint32_t heapBeforeTls = ESP.getFreeHeap();
      const uint32_t maxBlockBeforeTls = ESP.getMaxFreeBlockSize();
      const uint8_t fragmentationBeforeTls = ESP.getHeapFragmentation();
      Log.notice("%s OTA TLS before: heap=%u maxBlock=%u fragmentation=%u%% rssi=%d" CR,
                 tags::system,
                 heapBeforeTls,
                 maxBlockBeforeTls,
                 fragmentationBeforeTls,
                 WiFi.RSSI());
#endif
      BearSSL::WiFiClientSecure client;
      client.setInsecure();
      client.setBufferSizes(constanstsCloudIO::otaTlsReceiveBufferSize,
                            constanstsCloudIO::tlsTransmitBufferSize);
      ret = ESPhttpUpdate.update(client, otaUrl, String(VERSION));
      otaTlsErrorCode = client.getLastSSLError(otaTlsError, sizeof(otaTlsError));
#ifdef DEBUG_ONOFRE
      Log.notice("%s OTA TLS after: heap=%u maxBlock=%u fragmentation=%u%% sslError=%d detail=%s" CR,
                 tags::system,
                 ESP.getFreeHeap(),
                 ESP.getMaxFreeBlockSize(),
                 ESP.getHeapFragmentation(),
                 otaTlsErrorCode,
                 otaTlsError);
#endif
    }
    else
    {
      ret = HTTP_UPDATE_FAILED;
      otaTlsErrorCode = -1000;
      strlcpy(otaTlsError, "Memória insuficiente para ligação TLS", sizeof(otaTlsError));
    }
#else
    WiFiClientSecure client;
    client.setInsecure();
#ifdef ESP32
    ret = httpUpdate.update(client, otaUrl, String(VERSION));
#endif
#endif
  }
  else
  {
    WiFiClient client;
#ifdef ESP8266
    ret = ESPhttpUpdate.update(client, otaUrl, String(VERSION));
#endif
#ifdef ESP32
    ret = httpUpdate.update(client, otaUrl, String(VERSION));
#endif
  }
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    otaStatus.state = OtaState::FAILED;
    deviceLog("atualizacao falhou");
#ifdef ESP8266
    if (otaTlsErrorCode != 0 && otaTlsError[0] != '\0')
      strlcpy(otaStatus.error, otaTlsError, sizeof(otaStatus.error));
    else
      strlcpy(otaStatus.error, ESPhttpUpdate.getLastErrorString().c_str(), sizeof(otaStatus.error));
#else
    strlcpy(otaStatus.error, httpUpdate.getLastErrorString().c_str(), sizeof(otaStatus.error));
#endif
#ifdef DEBUG_ONOFRE
#ifdef ESP8266
    Log.notice("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#endif
#ifdef ESP32
    Log.notice("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
#endif
#endif
    return AutoUpdateResult::FAILED;
  case HTTP_UPDATE_NO_UPDATES:
    otaStatus.state = OtaState::FAILED;
    strlcpy(otaStatus.error, "O servidor não ofereceu atualização", sizeof(otaStatus.error));
#ifdef DEBUG_ONOFRE
    Log.notice("HTTP_UPDATE_NO_UPDATES");
#endif
    return AutoUpdateResult::NO_UPDATE;
  case HTTP_UPDATE_OK:
    otaStatus.state = OtaState::DONE;
    otaStatus.percent = 100;
    deviceLog("atualizacao gravada");
#ifdef DEBUG_ONOFRE
    Log.notice("HTTP_UPDATE_OK");
#endif
    return AutoUpdateResult::UPDATED;
  }
  otaStatus.state = OtaState::FAILED;
  strlcpy(otaStatus.error, "Resultado de atualização desconhecido", sizeof(otaStatus.error));
  return AutoUpdateResult::FAILED;
}

class CaptiveRequestHandler : public AsyncWebHandler
{
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  // Both generations again, and this one decides whether the handler runs at all:
  // in the archived fork canHandle() is non-const, in ESP32Async 3.x it is const.
  // With only the non-const version the C6 build overrides nothing, the base
  // returns false, and since setupCaptivePortal() resets the server and leaves
  // this handler alone on the AP, every request there answers 404 — the access
  // point comes up and the configuration page cannot be reached.
  bool canHandle(AsyncWebServerRequest *request)
  {
    return true;
  }
  bool canHandle(AsyncWebServerRequest *request) const
  {
    return true;
  }
  // AsyncWebServer 1.x uses a non-const virtual method, while the newer
  // ESP32Async API used by Arduino 3.x makes it const. Keep both overloads so
  // POST form bodies are parsed on every supported framework generation.
  bool isRequestHandlerTrivial()
  {
    return false;
  }
  bool isRequestHandlerTrivial() const
  {
    return false;
  }
  void restart()
  {
    config.requestRestart();
  }
  void handleRequest(AsyncWebServerRequest *request)
  {
    // One modest allocation avoids the repeated grow/copy cycle that could
    // silently truncate the ESP8266 response before the form was appended.
    AsyncResponseStream *response = request->beginResponseStream("text/html", 4096);
    response->addHeader("Cache-Control", "no-store");
    if (!config.tryBeginFeatureAccess())
    {
      response->setCode(409);
      response->print("Feature configuration is busy; retry shortly.");
      request->send(response);
      return;
    }

    bool store = false;
    const bool isSubmission = request->method() == HTTP_POST;
    const AsyncWebParameter *ssidParam = isSubmission ? request->getParam("s", true) : nullptr;
    const AsyncWebParameter *nameParam = isSubmission ? request->getParam("i", true) : nullptr;
    const AsyncWebParameter *passwordParam = isSubmission ? request->getParam("p", true) : nullptr;
    const AsyncWebParameter *templateParam = isSubmission ? request->getParam("t", true) : nullptr;
#ifdef DEBUG_ONOFRE
    if (isSubmission)
    {
      Log.notice("[CAPTIVE] POST fields: wifi=%d name=%d password=%d template=%d" CR,
                 ssidParam != nullptr, nameParam != nullptr,
                 passwordParam != nullptr, templateParam != nullptr);
    }
#endif
    response->print(FPSTR(HTTP_HEADER));
    response->print(FPSTR(HTTP_SCRIPT));
    response->print(FPSTR(HTTP_STYLE));
    response->print(FPSTR(HTTP_CAPTIVE_BODY_START));
    if (ssidParam != nullptr && nameParam != nullptr &&
        ssidParam->value().length() > 0 && nameParam->value().length() > 0)
    {
      String n_name = config.chipId;
      n_name = nameParam->value();
      normalize(n_name);
      if (n_name.isEmpty())
        n_name = config.chipId;

      // A configured device also enters captive mode when only its Wi-Fi is
      // unavailable. Its selector is hidden, but browsers still submit the
      // hidden field with value 0. Preserve the installed feature graph in that
      // recovery flow; templates are selectable only during first setup.
      bool invalidTemplate = false;
      if (config.templateId == Template::NO_TEMPLATE)
      {
        // A first-time setup must explicitly submit one of the choices shown by
        // the form. Configured devices skip this block and preserve their
        // installed feature graph even though browsers submit hidden t=0.
        invalidTemplate = templateParam == nullptr;
        if (!invalidTemplate)
        {
          String templateValue = templateParam->value();
          templateValue.trim();
          const int templateId = templateValue.toInt();
          invalidTemplate = templateValue != String(templateId) ||
                            !isCaptiveTemplateAllowed(templateId) ||
                            !config.loadTemplate(templateId);
        }
      }
      if (invalidTemplate)
      {
        config.endFeatureAccess();
        response->setCode(400);
        response->print(FPSTR(HTTP_CAPTIVE_INVALID));
        response->print(FPSTR(HTTP_END));
        request->send(response);
        return;
      }

      // Commit identity and Wi-Fi fields only after every submitted option has
      // validated. An invalid template must not leave a partial in-memory
      // configuration that a later, unrelated save could persist.
      strlcpy(config.nodeId, n_name.c_str(), sizeof(config.nodeId));
      strlcpy(config.wifiSSID, ssidParam->value().c_str(), sizeof(config.wifiSSID));

      if (passwordParam != nullptr)
      {
        strlcpy(config.wifiSecret, passwordParam->value().c_str(), sizeof(config.wifiSecret));
      }
      else
      {
        strlcpy(config.wifiSecret, "", sizeof(config.wifiSecret));
      }
      store = true;
    }

    if (!isSubmission && request->hasArg("sc"))
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
      if (isSubmission)
      {
        response->setCode(400);
        response->print(FPSTR(HTTP_CAPTIVE_INVALID));
      }
      String form = FPSTR(HTTP_FORM_START);
      form.replace("{n}", config.nodeId);
      if (config.templateId == 0)
      {
        form.replace("class='hide'", "");
      }
      response->print(form);
      response->print(FPSTR(HTTP_END));
    }
    if (store)
    {
      if (config.persist())
      {
        String storedR = FPSTR(HTTP_SAVED);
        storedR.replace("{o}", String("http://" + String(config.nodeId) + ".local").c_str());
        response->print(storedR.c_str());
      }
      else
      {
        response->setCode(507);
        response->print(F("<p>Não foi possível guardar. A configuração anterior será reposta.</p>"));
      }
      response->print(FPSTR(HTTP_END));
      response->addHeader("Connection", "close");
      request->onDisconnect([]()
                            { config.requestRestart(); });
      // Keep ownership until the response closes and the requested restart
      // rebuilds every feature from the stored configuration.
    }
    else
    {
      config.endFeatureAccess();
    }
    request->send(response);
  }
};

AsyncJsonResponse *errorResponse(const char *cause, int status = 400)
{
  AsyncJsonResponse *responseError = new AsyncJsonResponse();
  JsonVariant &root = responseError->getRoot();
  root["cause"] = cause;
  responseError->setCode(status);
  responseError->setLength();
  return responseError;
}

void sendFeatureBusy(AsyncWebServerRequest *request)
{
  request->send(errorResponse("Feature configuration is busy; retry shortly", 409));
}

bool authorizeRequest(AsyncWebServerRequest *request, bool sendFailure = true,
                      bool *featureBusy = nullptr)
{
#if WEB_SECURE_ON
  if (featureBusy != nullptr)
    *featureBusy = false;
  // These buffers are captured before feature tasks start and remain immutable
  // for the boot. API credential edits force a controlled restart, so static
  // Web assets never contend with sensor/actuator work merely to authenticate.
  if (!request->authenticate(webApiUser, webApiPassword, REALM))
  {
    if (sendFailure)
      request->requestAuthentication(REALM);
    return false;
  }
#else
  (void)request;
  (void)sendFailure;
  (void)featureBusy;
#endif
  return true;
}

void initializeWebAuthCredentials()
{
  strlcpy(webApiUser, config.apiUser, sizeof(webApiUser));
  strlcpy(webApiPassword, config.apiPassword, sizeof(webApiPassword));
}

void loadWebPanel()
{
  // HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (!authorizeRequest(request))
                return;
              AsyncWebServerResponse *response = beginProgmemResponse(request, 200, "text/html", index_html, sizeof(index_html));
              response->addHeader("Content-Encoding", "gzip");
              response->addHeader("Cache-Control", "max-age=30");
              request->send(response); });

  // JS
  server.on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!authorizeRequest(request))
      return;
    AsyncWebServerResponse *response = beginProgmemResponse(request, 200, "application/javascript", index_js, sizeof(index_js));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response); });

  // CSS
  server.on("/css/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!authorizeRequest(request))
      return;
    AsyncWebServerResponse *response = beginProgmemResponse(request, 200, "text/css", styles_min_css, sizeof(styles_min_css));
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=600");
    request->send(response); });
}

void loadAPI()
{
  /*EXPORT FULL NON-SECRET BACKUP*/
  server.on("/backup", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (!authorizeRequest(request))
      return;
    if (!config.tryBeginFeatureAccess())
    {
      sendFeatureBusy(request);
      return;
    }
    JsonDocument document;
    JsonVariant root = document.to<JsonObject>();
    config.backup(root);
    if (document.overflowed())
    {
      config.endFeatureAccess();
      request->send(errorResponse("Not enough memory to create backup", 507));
      return;
    }
    String payload;
    serializeJsonPretty(document, payload);
    config.endFeatureAccess();
    AsyncWebServerResponse *response = request->beginResponse(
        200, "application/json", payload);
    response->addHeader("Cache-Control", "no-store");
    response->addHeader("Content-Disposition",
                        "attachment; filename=easyiot-backup.json");
    request->send(response); });

  /*STAGE FULL RECOVERY*/
  server.addHandler(new AsyncCallbackJsonWebHandler(
      "/restore", [](AsyncWebServerRequest *request, JsonVariant json)
      {
        if (!authorizeRequest(request))
          return;
        if (!json.is<JsonObject>())
        {
          request->send(errorResponse("Restore request must be a JSON object"));
          return;
        }
        if (!config.tryBeginFeatureAccess())
        {
          sendFeatureBusy(request);
          return;
        }
        AsyncJsonResponse *response = new AsyncJsonResponse();
        JsonVariant &responseRoot = response->getRoot();
        JsonObject restore = json.as<JsonObject>();
        const ConfigUpdateResult result = config.stageRestore(restore);
        if (result != ConfigUpdateResult::OK)
        {
          responseRoot["result"] = static_cast<int>(result);
          response->setCode(result == ConfigUpdateResult::PERSISTENCE_FAILED
                                ? 507
                                : 400);
          config.endFeatureAccess();
        }
        else
        {
          responseRoot["restartRequired"] = true;
          response->addHeader("Connection", "close");
          request->onDisconnect([]()
                                { config.requestRestart(); });
          // Keep the lease until reboot applies the staged transaction before
          // any feature loop can observe the replacement graph.
        }
        response->setLength();
        request->send(response);
      }));

  /*GET CONFIG*/
  server
      .on("/config", HTTP_GET, [](AsyncWebServerRequest *request)
          {
    if (!authorizeRequest(request))
      return;
    if (!config.tryBeginFeatureAccess())
    {
      sendFeatureBusy(request);
      return;
    }
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    config.json(root,true);
    String payload;
    serializeJsonPretty(root, payload);
    config.endFeatureAccess();
    delete response;
    AsyncWebServerResponse *configResponse = request->beginResponse(200, "application/json", payload);
    configResponse->addHeader("Cache-Control", "no-store");
    request->send(configResponse); });

  /*SAVE CONFIG*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
    if (!authorizeRequest(request))
      return;
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    ConfigUpdateResult result = ConfigUpdateResult::INVALID_REQUEST;
    if (json.is<JsonObject>())
    {
      JsonObject configJson = json.as<JsonObject>();
      result = config.update(configJson, root);
    }
    if (result != ConfigUpdateResult::OK)
    {
      root["result"] = static_cast<int>(result);
      if (result == ConfigUpdateResult::BUSY)
        response->setCode(409);
      else if (result == ConfigUpdateResult::PERSISTENCE_FAILED)
        response->setCode(507);
      else
        response->setCode(400);
    }
    const bool restartRequired = root["restartRequired"] | false;
    response->setLength();
    if (restartRequired)
    {
      // Close this response explicitly and queue the restart only after the
      // asynchronous response has drained or the peer has disconnected.
      response->addHeader("Connection", "close");
      request->onDisconnect([]()
                            { config.requestRestart(); });
    }
    request->send(response); }));

  /*CREATE NEW FEATURE*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/features", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
    if (!authorizeRequest(request))
      return;
    if (!json.is<JsonObject>())
    {
      request->send(errorResponse("Feature request must be a JSON object"));
      return;
    }
    if (!config.tryBeginFeatureAccess())
    {
      sendFeatureBusy(request);
      return;
    }
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    JsonObject featureJson = json.as<JsonObject>();
    int result = prepareNewFeature(featureJson["name"] | "", featureJson["input1"] | DefaultPins::noGPIO, featureJson["input2"] | DefaultPins::noGPIO, featureJson["driver"] | 999);
    if (result == 0)
    {
      if (!config.persist())
      {
        root["result"] = static_cast<int>(ConfigUpdateResult::PERSISTENCE_FAILED);
        response->setCode(507);
        response->addHeader("Connection", "close");
        request->onDisconnect([]()
                              { config.requestRestart(); });
        // Keep the lease until reboot restores the previous atomic file.
      }
      else
      {
        config.reloadFeatures().json(root, true);
        config.endFeatureAccess();
      }
    }
    else
    {
      response->setCode(400);
      root["result"] = result;
      config.endFeatureAccess();
    }
    response->setLength();
    request->send(response); }));

  /*CONTROL ACTUATOR*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/actuators/control", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
    if (!authorizeRequest(request))
      return;
    if (!json.is<JsonObject>())
    {
      request->send(errorResponse("Actuator request must be a JSON object"));
      return;
    }
    if (!config.tryBeginFeatureAccess())
    {
      sendFeatureBusy(request);
      return;
    }
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    JsonObject action = json.as<JsonObject>();
    config.controlFeature(StateOrigin::WEBPANEL,action,root);
    config.endFeatureAccess();
    response->setLength();
    request->send(response); }));

  /*RESET A METER'S ACCUMULATED ENERGY*/
  server
      .addHandler(new AsyncCallbackJsonWebHandler("/sensors/reset-energy", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
    if (!authorizeRequest(request))
      return;
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    const String id = json["id"] | "";
    bool found = false;

    if (id.length() == 0)
    {
      root["result"] = "Meter id is required";
      response->setCode(400);
    }
    else
    {
      if (!config.tryBeginFeatureAccess())
      {
        delete response;
        sendFeatureBusy(request);
        return;
      }
      for (auto &sensor : config.sensors)
      {
        if (id.equals(sensor.uniqueId) && sensor.supportsEnergyReset())
        {
          // Queued, not performed here: the meter is on a serial bus the sensor
          // loop owns, and reaching for it from this context corrupts a read.
          sensor.requestEnergyReset();
          found = true;
          break;
        }
      }
      config.endFeatureAccess();
      root["result"] = found ? "Energy reset queued" : "Unknown or unsupported meter";
      response->setCode(found ? 202 : 404);
    }
    response->setLength();
    request->send(response); }));

  auto irrigationRunHandler = [](AsyncWebServerRequest *request, JsonVariant json)
  {
    if (!authorizeRequest(request))
      return;
    if (!config.tryBeginFeatureAccess())
    {
      sendFeatureBusy(request);
      return;
    }
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    const bool started = irrigation.runProgram((uint8_t)(json["programId"] | 0));
    irrigation.jsonBody(root);
    config.endFeatureAccess();
    if (!started)
      response->setCode(404);
    response->setLength();
    request->send(response);
  };

  auto irrigationStopHandler = [](AsyncWebServerRequest *request)
  {
    if (!authorizeRequest(request))
      return;
    if (!config.tryBeginFeatureAccess())
    {
      sendFeatureBusy(request);
      return;
    }
    irrigation.stop();
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    irrigation.jsonBody(root);
    config.endFeatureAccess();
    response->setLength();
    request->send(response);
  };

  /* A firmware update does not reload an open browser tab, so a device on a new
     build is routinely driven by the previous panel. Those panels call
     /irrigation/run and /irrigation/stop, which the schedule handler below would
     otherwise claim — deleting every program on a run and failing on a stop. The
     old paths are therefore registered here, ahead of it, pointing at the right
     code. Order is the mechanism: the server takes the first handler that accepts
     the request. */
  server.on("/irrigation/stop", HTTP_POST, irrigationStopHandler);
  server.on("/irrigation/stop", HTTP_GET, irrigationStopHandler);
  server.addHandler(new AsyncCallbackJsonWebHandler("/irrigation/run", irrigationRunHandler));

  /*IRRIGATION SCHEDULE*/
  server
      // The action aliases are registered before this path on purpose.
      // AsyncWebServer matches a plain URI as "exact, or prefix with a trailing
      // slash", so registering this schedule handler first would make it answer
      // action requests as schedule replacements. A run request carries no
      // "programs" key, so the schedule used to parse as empty and be saved that
      // way: pressing "Regar agora" deleted every program, and stopping failed.
      .addHandler(new AsyncCallbackJsonWebHandler("/irrigation", [](AsyncWebServerRequest *request, JsonVariant json)
                                                  {
    if (!authorizeRequest(request))
      return;
    if (!json.is<JsonObject>())
    {
      request->send(errorResponse("Irrigation request must be a JSON object"));
      return;
    }
    if (!config.tryBeginFeatureAccess())
    {
      sendFeatureBusy(request);
      return;
    }
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    JsonObject body = json.as<JsonObject>();
    if (!irrigation.update(body))
    {
      root["result"] = "Invalid irrigation schedule";
      response->setCode(400);
      config.endFeatureAccess();
    }
    else if (!irrigation.save())
    {
      // The old atomic file is still valid, but RAM already contains the new
      // schedule. Keep the lease and reboot after this error response so boot
      // reloads the last durable schedule before any valve can use the draft.
      root["result"] = "Failed to store irrigation schedule";
      response->setCode(507);
      response->addHeader("Connection", "close");
      request->onDisconnect([]()
                            { config.requestRestart(); });
    }
    else
    {
      // Answer with what was stored, not with what was sent: the panel shows
      // the schedule actually kept, including zones it dropped.
      irrigation.jsonBody(root);
      config.endFeatureAccess();
      // A schedule change moves no valve, so nothing else would tell the apps.
      notifyIrrigationToCloudIO();
    }
    response->setLength();
    request->send(response); }));

  /*FORCE A PROGRAM NOW*/
  server.addHandler(new AsyncCallbackJsonWebHandler("/irrigation-run", irrigationRunHandler));

  auto rebootHandler = [](AsyncWebServerRequest *request)
  {
    if (!authorizeRequest(request))
      return;
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    root["result"] = "Reboot requested";
    response->setLength();
    request->send(response);
    config.requestRestart();
  };

  auto templateChangeHandler = [](AsyncWebServerRequest *request)
  {
    if (!authorizeRequest(request))
      return;
    const AsyncWebParameter *templateParam = nullptr;
    if (request->hasParam("t", true))
      templateParam = request->getParam("t", true);
    else if (request->hasParam("t"))
      templateParam = request->getParam("t");

    if (templateParam == nullptr)
    {
      request->send(errorResponse("Template id is missing"));
      return;
    }

    String templateValue = templateParam->value();
    templateValue.trim();
    const int templateId = templateValue.toInt();
    if (templateValue != String(templateId) ||
        templateId < Template::DUAL_LIGHT || templateId > Template::GARDEN)
    {
      request->send(errorResponse("Template id is invalid"));
      return;
    }

    // The async callback may run while a feature loop is active. Queue the
    // replacement for the main loop instead of mutating live vectors here.
    if (!config.requestTemplateChange(templateId))
    {
      sendFeatureBusy(request);
      return;
    }

    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    root["result"] = "Template change queued";
    response->setCode(202);
    response->setLength();
    request->send(response);
  };

  auto loadDefaultsHandler = [](AsyncWebServerRequest *request)
  {
    if (!authorizeRequest(request))
      return;
    AsyncJsonResponse *response = new AsyncJsonResponse();
    JsonVariant &root = response->getRoot();
    root["result"] = "Load defaults requested";
    response->setLength();
    request->send(response);
    config.requestLoadDefaults();
  };

  // POST is the preferred method for state-changing endpoints.
  // Keep GET temporarily for backward compatibility with old clients.
  /* Plain text, not JSON: this exists to be selected, copied and pasted into a
     message by whoever is holding the device. Wrapping it in JSON would make the
     one thing it is for harder. */
  server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // authorizeRequest(), not request->authenticate() with config.*: credentials
    // are snapshotted at boot so an async handler never reads mutable state, and
    // the test suite enforces it. It caught this line.
    if (!authorizeRequest(request))
      return;
    request->send(200, "text/plain; charset=utf-8", deviceLogText()); });

  server.on("/reboot", HTTP_POST, rebootHandler);
  server.on("/reboot", HTTP_GET, rebootHandler);
  // GET as well as POST: stopping the watering is the one thing someone may need
  // to do from a phone browser bar, with a wet lawn and no app.
  server.on("/irrigation-stop", HTTP_POST, irrigationStopHandler);
  server.on("/irrigation-stop", HTTP_GET, irrigationStopHandler);

  server.on("/templates/change", HTTP_POST, templateChangeHandler);
  server.on("/templates/change", HTTP_GET, templateChangeHandler);

  server.on("/load-defaults", HTTP_POST, loadDefaultsHandler);
  server.on("/load-defaults", HTTP_GET, loadDefaultsHandler);

  // The only sibling without a GET: /reboot, /load-defaults and
  // /templates/change all accept both, so pasting this one into a browser
  // answered 404 — the obvious thing to try when talking someone through an
  // update over the phone.
  auto autoUpdateHandler = [](AsyncWebServerRequest *request)
  {
    if (!authorizeRequest(request))
      return;
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", REDIRECT_PAGE);
    response->addHeader("Connection", "close");
    request->onDisconnect([]()
                          { config.requestAutoUpdate(); });
    request->send(response);
  };
  server.on("/auto-update", HTTP_POST, autoUpdateHandler);
  server.on("/auto-update", HTTP_GET, autoUpdateHandler);

  server
      .on(
          "/update", HTTP_POST, [](AsyncWebServerRequest *request)
          {
#if WEB_SECURE_ON
            ManualUpdateState *authState = manualUpdateState(request);
            if ((authState == nullptr ||
                 (!authState->authenticated && !authState->busy)) &&
                !authorizeRequest(request))
              return;
#endif

            ManualUpdateState *state = manualUpdateState(request);
            const bool success = state != nullptr && state->finalSeen && state->success;
            const int status = success ? 200 : (state == nullptr ? 400 : (state->busy ? 409 : 500));
            AsyncWebServerResponse *response = request->beginResponse(status, "text/html", success ? REDIRECT_PAGE : UPDATE_FAILED);
            response->addHeader("Connection", "close");
            request->send(response); },
          [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
          {
#if WEB_SECURE_ON
            // The upload callback receives the body before the request callback.
            // It records authentication without sending a response; the normal
            // request handler must be the only code that sends a challenge or
            // error after the multipart body has finished.
            if (manualUpdateState(request) == nullptr && index != 0)
              return;
#endif
            if (!index)
            {
              // Only the first file part belongs to this firmware request.
              // Ignore additional multipart files without replacing the state
              // whose disconnect callback owns the updater and access lease.
              if (manualUpdateState(request) != nullptr)
                return;

              ManualUpdateState *state = static_cast<ManualUpdateState *>(
                  std::calloc(1, sizeof(ManualUpdateState)));
              if (state == nullptr)
                return;

              request->_tempObject = state;
              request->onDisconnect([request, state]()
                                    {
                if (!state->ownsFeatureAccess)
                  return;

                if (state->ownsUpdate && manualUpdateOwner == request)
                  manualUpdateOwner = nullptr;
                if (state->success)
                {
                  config.requestRestart();
                  // Keep the lease until the restart. No old feature may run
                  // against an image that has just been replaced.
                  return;
                }

                if (state->ownsUpdate)
                  abortManualUpdate();
                if (state->ownsFeatureAccess)
                {
                  config.endFeatureAccess();
                  state->ownsFeatureAccess = false;
                } });

#if WEB_SECURE_ON
              bool authBusy = false;
              if (!authorizeRequest(request, false, &authBusy))
              {
                state->busy = authBusy;
                return;
              }
#endif
              state->authenticated = true;

              // The updater and the feature graph are process-global. A second
              // upload or any competing feature operation must retry instead
              // of waiting inside the AsyncWebServer callback.
              if (!config.tryBeginFeatureAccess())
              {
                state->busy = true;
                return;
              }
              state->ownsFeatureAccess = true;

              if (manualUpdateOwner != nullptr)
              {
                state->busy = true;
                config.endFeatureAccess();
                state->ownsFeatureAccess = false;
                return;
              }

              manualUpdateOwner = request;
              state->ownsUpdate = true;
#ifdef DEBUG_ONOFRE
              Log.notice("%s Update Start: %s" CR, tags::system, filename.c_str());
#endif
#ifdef ESP8266
              Update.runAsync(true);
#endif
              if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
              {
                state->failed = true;
                Update.printError(Serial);
              }
            }

            ManualUpdateState *state = manualUpdateState(request);
#if WEB_SECURE_ON
            if (state != nullptr && !state->authenticated)
              return;
#endif
            if (state == nullptr || !state->ownsUpdate || state->finalSeen)
              return;

            if (!state->failed && !Update.hasError())
            {
              if (Update.write(data, len) != len)
              {
                state->failed = true;
                Update.printError(Serial);
              }
            }
            if (final)
            {
              state->finalSeen = true;
              state->success = !state->failed && Update.end(true);
              state->failed = !state->success;
              if (state->success)
              {
#ifdef DEBUG_ONOFRE
                Log.notice("%s Update Success: %d" CR, tags::system, index + len);
#endif
              }
              else
              {
                Update.printError(Serial);
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
