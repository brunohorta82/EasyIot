#include "CoreWiFi.h"
#include "DeviceClock.h"
#include "DeviceLog.h"
#include "Constants.h"
#if defined(ESP8266) || defined(LEGACY_PROVISON)
#include <JustWifi.h>
#else
#include "nvs_flash.h"
#endif
#include "ConfigOnofre.h"
#include "WebServer.h"
#include <esp-knx-ip.h>
int retryCount = 0;
unsigned long connectedOn = 0ul;
extern ConfigOnofre config;

namespace
{
struct WiFiConfigSnapshot
{
  char nodeId[sizeof(config.nodeId)];
  char chipId[sizeof(config.chipId)];
  char provisionId[sizeof(config.provisionId)];
  char wifiSSID[sizeof(config.wifiSSID)];
  char wifiSecret[sizeof(config.wifiSecret)];
  bool dhcp;
  char wifiIp[sizeof(config.wifiIp)];
  char wifiMask[sizeof(config.wifiMask)];
  char wifiGw[sizeof(config.wifiGw)];
  char accessPointPassword[sizeof(config.accessPointPassword)];
};

// Config scalars can be replaced by an AsyncWebServer callback while the Wi-Fi
// event task or main loop is reading them. Copy the complete Wi-Fi view under a
// short lease, then release it before any potentially blocking network call.
bool takeWiFiConfigSnapshot(WiFiConfigSnapshot &snapshot)
{
  if (!config.tryBeginFeatureAccess())
    return false;

  strlcpy(snapshot.nodeId, config.nodeId, sizeof(snapshot.nodeId));
  strlcpy(snapshot.chipId, config.chipId, sizeof(snapshot.chipId));
  strlcpy(snapshot.provisionId, config.provisionId, sizeof(snapshot.provisionId));
  strlcpy(snapshot.wifiSSID, config.wifiSSID, sizeof(snapshot.wifiSSID));
  strlcpy(snapshot.wifiSecret, config.wifiSecret, sizeof(snapshot.wifiSecret));
  snapshot.dhcp = config.dhcp;
  strlcpy(snapshot.wifiIp, config.wifiIp, sizeof(snapshot.wifiIp));
  strlcpy(snapshot.wifiMask, config.wifiMask, sizeof(snapshot.wifiMask));
  strlcpy(snapshot.wifiGw, config.wifiGw, sizeof(snapshot.wifiGw));
  strlcpy(snapshot.accessPointPassword, config.accessPointPassword, sizeof(snapshot.accessPointPassword));
  config.endFeatureAccess();
  return true;
}

String getApName(const WiFiConfigSnapshot &snapshot)
{
  String version = String(VERSION);
  version.replace(".", "x");
  if (strcmp(snapshot.nodeId, snapshot.chipId) == 0)
    return "OnOfre-" + String(snapshot.chipId) + "-" + version;
  return String(snapshot.nodeId) + "-" + version;
}

struct PendingWiFiCredentials
{
  char ssid[sizeof(config.wifiSSID)] = {};
  char secret[sizeof(config.wifiSecret)] = {};
  bool credentialsPending = false;
  bool savePending = false;
};

PendingWiFiCredentials pendingWiFiCredentials;

#ifdef ESP32
portMUX_TYPE pendingWiFiMux = portMUX_INITIALIZER_UNLOCKED;
#endif

void lockPendingWiFi()
{
#ifdef ESP32
  portENTER_CRITICAL(&pendingWiFiMux);
#endif
}

void unlockPendingWiFi()
{
#ifdef ESP32
  portEXIT_CRITICAL(&pendingWiFiMux);
#endif
}

// The caller-owned strings may point into an event payload. Copy them before
// entering the critical section so the protected interval remains bounded.
void queuePendingWiFiCredentials(const char *ssid, const char *secret)
{
  char ssidCopy[sizeof(pendingWiFiCredentials.ssid)] = {};
  char secretCopy[sizeof(pendingWiFiCredentials.secret)] = {};
  strlcpy(ssidCopy, ssid ? ssid : "", sizeof(ssidCopy));
  strlcpy(secretCopy, secret ? secret : "", sizeof(secretCopy));

  lockPendingWiFi();
  memcpy(pendingWiFiCredentials.ssid, ssidCopy, sizeof(pendingWiFiCredentials.ssid));
  memcpy(pendingWiFiCredentials.secret, secretCopy, sizeof(pendingWiFiCredentials.secret));
  pendingWiFiCredentials.credentialsPending = true;
  unlockPendingWiFi();
}

#if defined(ESP32) && !defined(LEGACY_PROVISON)
void queuePendingWiFiSave()
{
  lockPendingWiFi();
  pendingWiFiCredentials.savePending = true;
  unlockPendingWiFi();
}
#endif

bool hasPendingWiFiWork()
{
  lockPendingWiFi();
  const bool pending = pendingWiFiCredentials.credentialsPending || pendingWiFiCredentials.savePending;
  unlockPendingWiFi();
  return pending;
}

PendingWiFiCredentials takePendingWiFiWork()
{
  PendingWiFiCredentials pending;
  lockPendingWiFi();
  pending = pendingWiFiCredentials;
  pendingWiFiCredentials.credentialsPending = false;
  pendingWiFiCredentials.savePending = false;
  unlockPendingWiFi();
  return pending;
}

#ifdef ESP32
std::atomic<bool> mdnsRefreshPending{false};
#else
bool mdnsRefreshPending = false;
#endif

void requestMDNSRefresh()
{
#ifdef ESP32
  mdnsRefreshPending.store(true, std::memory_order_release);
#else
  mdnsRefreshPending = true;
#endif
}

bool takeMDNSRefreshRequest()
{
#ifdef ESP32
  return mdnsRefreshPending.exchange(false, std::memory_order_acq_rel);
#else
  if (!mdnsRefreshPending)
    return false;
  mdnsRefreshPending = false;
  return true;
#endif
}
} // namespace

#if defined(ESP32) && !defined(LEGACY_PROVISON)

// Provisioning callbacks run on the ESP event task. Always stage credentials
// in CoreWiFi-owned storage; a failed lease acquisition is retried by
// loopWiFi(), rather than silently losing the event.
static void drainPendingProvisioningCredentials()
{
  if (!hasPendingWiFiWork() || !config.tryBeginFeatureAccess())
    return;

  const PendingWiFiCredentials pending = takePendingWiFiWork();
  if (pending.credentialsPending)
  {
    strlcpy(config.wifiSSID, pending.ssid, sizeof(config.wifiSSID));
    strlcpy(config.wifiSecret, pending.secret, sizeof(config.wifiSecret));
  }
  if (pending.savePending && !config.persist())
  {
    // Keep retrying from the main loop. The atomic writer left the previous
    // file intact, and provisioning credentials must not be acknowledged only
    // in RAM and then disappear on the next power loss.
    queuePendingWiFiSave();
  }
  config.endFeatureAccess();
}

void SysProvEvent(arduino_event_t *sys_event)
{
  switch (sys_event->event_id)
  {
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Wi-Fi IP:  %s" CR, tags::wifi, IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr).toString());
#endif
    setupWebPanel();
    startWebserver();
    knx.start();
    config.requestCloudIOSync();
    requestMDNSRefresh();
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Disconnected. Connecting to the AP again... " CR, tags::wifi);
#endif
    break;
  case ARDUINO_EVENT_PROV_START:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Provisioning started\nGive Credentials of your access point using smartphone app" CR, tags::wifi);
#endif
    break;
  case ARDUINO_EVENT_PROV_CRED_RECV:
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Received Wi-Fi credentials." CR, tags::wifi);
#endif
    queuePendingWiFiCredentials((const char *)sys_event->event_info.prov_cred_recv.ssid,
                                (const char *)sys_event->event_info.prov_cred_recv.password);
    break;
  }
  case ARDUINO_EVENT_PROV_CRED_FAIL:
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s Provisioning failed! Please reset to factory and retry provisioning" CR, tags::wifi);
#endif
    if (sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
    {
#ifdef DEBUG_ONOFRE
      Log.notice("%s Wi-Fi AP password incorrect" CR, tags::wifi);
#endif
    }
    else
    {
#ifdef DEBUG_ONOFRE
      Log.notice("%s Wi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()" CR, tags::wifi);
#endif
    }

    ESP_ERROR_CHECK(nvs_flash_erase());
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_ERROR_CHECK(nvs_flash_init());
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
    WiFi.disconnect(true, true);
    config.requestRestart();
    break;
  }
  case ARDUINO_EVENT_PROV_CRED_SUCCESS:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Provisioning Successful" CR, tags::wifi);
#endif
    queuePendingWiFiSave();
    break;
  case ARDUINO_EVENT_PROV_END:
#ifdef DEBUG_ONOFRE
    Log.notice("%s Provisioning Ends" CR, tags::wifi);
#endif
    break;
  default:
    break;
  }
}
#endif
void scanNewWifiNetworks()
{

  unsigned char result = WiFi.scanNetworks();
  if (result == WIFI_SCAN_FAILED)
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s Scan Failed" CR, tags::wifi);
#endif
  }
  else if (result == 0)
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s No networks found " CR, tags::wifi);
#endif
  }
  else
  {

    JsonDocument doc;
    JsonArray object = doc.to<JsonArray>();
    for (int8_t i = 0; i < result; ++i)
    {
      String ssid_scan;
      int32_t rssi_scan;
      uint8_t sec_scan;
      uint8_t *BSSID_scan;
      int32_t chan_scan;
      bool hidden_scan;
#ifdef ESP32
      WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan);
#endif
#ifdef ESP8266
      WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);
#endif
      object.add(ssid_scan);
#ifdef DEBUG_ONOFRE
      Log.notice("%s Network found %s" CR, tags::wifi, ssid_scan.c_str());
#endif
    }
    String networks = "";
    serializeJson(doc, networks);
    sendToServerEvents("wifi-networks", networks.c_str());
  }
  WiFi.scanDelete();
}

#if defined(ESP8266) || defined(LEGACY_PROVISON)
static void drainPendingLegacyCredentials()
{
  if (!hasPendingWiFiWork() || !config.tryBeginFeatureAccess())
    return;

  const PendingWiFiCredentials pending = takePendingWiFiWork();
  // Preserve the legacy behavior: credentials learned by JustWifi only fill an
  // empty configuration. Explicitly configured credentials still win.
  if (pending.credentialsPending && config.wifiSSID[0] == '\0')
  {
    strlcpy(config.wifiSSID, pending.ssid, sizeof(config.wifiSSID));
    strlcpy(config.wifiSecret, pending.secret, sizeof(config.wifiSecret));
  }
  config.endFeatureAccess();
}

void reloadWiFiConfig()
{
  WiFiConfigSnapshot snapshot;
  if (!takeWiFiConfigSnapshot(snapshot))
  {
    config.requestReloadWifi();
    return;
  }

  jw.disconnect();
  jw.setHostname(snapshot.nodeId);
  jw.cleanNetworks();
  jw.setSoftAP(getApName(snapshot).c_str(), snapshot.accessPointPassword);

  if (strlen(snapshot.wifiSecret) > 0)
  {
    if (snapshot.dhcp)
    {
      jw.addNetwork(snapshot.wifiSSID, snapshot.wifiSecret);
    }
    else
    {
      jw.addNetwork(snapshot.wifiSSID, snapshot.wifiSecret, snapshot.wifiIp, snapshot.wifiGw, snapshot.wifiMask, snapshot.wifiGw, true);
    }
  }
  else
  {
    jw.addNetwork(snapshot.wifiSSID);
  }
}
void infoWifi()
{

  if (WiFi.isConnected())
  {
    connectedOn = millis();
#ifdef DEBUG_ONOFRE
    Log.notice("%s MODE STA" CR, tags::wifi);
    Log.notice("%s SSID  %s  " CR, tags::wifi, WiFi.SSID().c_str());
    Log.notice("%s CH    %d   " CR, tags::wifi, WiFi.channel());
    Log.notice("%s RSSI  %d " CR, tags::wifi, WiFi.RSSI());
    Log.notice("%s IP    %s  " CR, tags::wifi, WiFi.localIP().toString().c_str());
    Log.notice("%s MAC   %s  " CR, tags::wifi, WiFi.macAddress().c_str());
    Log.notice("%s GW    %s " CR, tags::wifi, WiFi.gatewayIP().toString().c_str());
    Log.notice("%s MASK  %s " CR, tags::wifi, WiFi.subnetMask().toString().c_str());
    Log.notice("%s DNS   %s " CR, tags::wifi, WiFi.dnsIP().toString().c_str());
    Log.notice("%s HOST  %s " CR, tags::wifi, WiFi.getHostname());
    Log.notice("----------------------------------------------" CR);
#endif
  }

  if (WiFi.getMode() & WIFI_AP)
  {
#ifdef DEBUG_ONOFRE
    Log.notice("%s MODE AP" CR, tags::wifi);
    Log.notice("%s SSID %s " CR, tags::wifi, jw.getAPSSID().c_str());
    Log.notice("%s IP  %s  " CR, tags::wifi, WiFi.softAPIP().toString().c_str());
    Log.notice("%s MAC  %s " CR, tags::wifi, WiFi.softAPmacAddress().c_str());
    Log.notice("----------------------------------------------" CR);
#endif
  }
}

void enableScan()
{
  jw.enableScan(true);
}

void dissableAP()
{
  jw.enableAP(false);
}

void infoCallback(justwifi_messages_t code, char *parameter)
{
  switch (code)
  {
  case MESSAGE_ACCESSPOINT_FAILED:
  case MESSAGE_WPS_START:
  case MESSAGE_WPS_SUCCESS:
  case MESSAGE_WPS_ERROR:
  case MESSAGE_SMARTCONFIG_START:
  case MESSAGE_SMARTCONFIG_SUCCESS:
  case MESSAGE_SMARTCONFIG_ERROR:
  case MESSAGE_HOSTNAME_ERROR:
  case MESSAGE_DISCONNECTED:
    deviceLog("wifi desligado");
  case MESSAGE_TURNING_OFF:
  case MESSAGE_TURNING_ON:
  case MESSAGE_SCANNING:
  case MESSAGE_SCAN_FAILED:
  case MESSAGE_NO_NETWORKS:
  case MESSAGE_NO_KNOWN_NETWORKS:
  case MESSAGE_FOUND_NETWORK:
  case MESSAGE_CONNECTING:
  case MESSAGE_CONNECT_WAITING:
  case MESSAGE_CONNECT_FAILED:
  case MESSAGE_ACCESSPOINT_DESTROYED:
  case MESSAGE_ACCESSPOINT_CREATING:
    break;

  case MESSAGE_CONNECTED:
    // JustWifi's disconnect() drops STA, and coming back up can restore the
    // default sleep setting, so this is asserted again where the link is real.
    WiFi.setSleep(false);
    // RSSI and channel are the two numbers every signal complaint needs, and the
    // only place they exist is the moment the link comes up.
    deviceLog("wifi ligado %s ch%d %ddBm ip %s", WiFi.SSID().c_str(), WiFi.channel(),
              WiFi.RSSI(), WiFi.localIP().toString().c_str());
  {
    // WiFi.SSID()/psk() return temporary Strings. Capture them before staging
    // so the pending buffers never retain framework-owned pointers.
    const String connectedSsid = WiFi.SSID();
    const String connectedSecret = WiFi.psk();
    queuePendingWiFiCredentials(connectedSsid.c_str(), connectedSecret.c_str());
    drainPendingLegacyCredentials();
    // The clock can only sync once there is a network, and anything scheduled
    // on the device depends on it.
    setupDeviceClock();
    setupWebPanel();
    startWebserver();
    knx.start();
    config.requestCloudIOSync();
    config.startCloudIOWatchdog();
#ifdef DEBUG_ONOFRE
    Log.notice("----------------------------------------------" CR);
#endif
    infoWifi();
    break;
  }

  case MESSAGE_ACCESSPOINT_CREATED:
    deviceLog("rede de configuracao criada");
    config.stopCloudIOWatchdog();
#ifdef DEBUG_ONOFRE
    Log.notice("----------------------------------------------" CR);
#endif
    infoWifi();
    setupCaptivePortal();
    startWebserver();
    break;
  }
}
void mdnsCallback(justwifi_messages_t code, char *parameter)
{

  if (code == MESSAGE_CONNECTED)
  {
    requestMDNSRefresh();
  }
}
#endif
bool wifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}
void refreshMDNS()
{
  WiFiConfigSnapshot snapshot;
  if (!takeWiFiConfigSnapshot(snapshot))
  {
    requestMDNSRefresh();
    return;
  }

  bool success = false;
#ifdef ESP32
  MDNS.end();
  success = MDNS.begin(snapshot.nodeId);
#endif
#ifdef ESP8266
  MDNS.removeService(snapshot.nodeId, "bhonofre", "tcp");
  MDNS.close();
  success = MDNS.begin(String(snapshot.nodeId), INADDR_ANY, 10);
#endif

  if (success)
  {
    MDNS.addService("bhonofre", "tcp", 80);
    MDNS.addServiceTxt("bhonofre", "tcp", "hardwareId", String(snapshot.chipId));
    MDNS.addServiceTxt("bhonofre", "tcp", "firmware", String(VERSION));
    MDNS.addServiceTxt("bhonofre", "tcp", "wifi", String(snapshot.wifiSSID));
#ifdef ESP32
    MDNS.addServiceTxt("bhonofre", "tcp", "mcu", "ESP32");
#endif
#ifdef ESP8266
    MDNS.addServiceTxt("bhonofre", "tcp", "wifi", "ESP8266");
#endif
  }
  else
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s MDNS Error" CR, tags::wifi);
#endif
  }
}

#if defined(ESP32) && !defined(LEGACY_PROVISON)
void beginBleProvison()
{
  WiFiConfigSnapshot snapshot;
  if (!takeWiFiConfigSnapshot(snapshot))
    return;

  WiFi.setSleep(true);
  WiFi.onEvent(SysProvEvent);
  const char *service_key;
  uint8_t uuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                      0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, Provision::pop, snapshot.provisionId, service_key, uuid, false);
  WiFiProv.printQR(snapshot.provisionId, Provision::pop, "ble");
#ifdef DEBUG_ONOFRE
  Log.info("%s Begin Provisioning using BLE" CR, tags::wifi);
#endif
}
#endif
void setupWiFi()
{
#if defined(ESP8266) || defined(LEGACY_PROVISON)
  // Bring the radio up before touching it. setSleep() was the first WiFi call in
  // this function, so the driver did not exist yet and it did nothing — modem
  // sleep stayed on, which is what a weak, slowly-improving signal looks like.
  // The boot log said so out loud a few lines later: reloadWiFiConfig() calls
  // jw.disconnect(), and that came back ESP_ERR_WIFI_NOT_INIT at 75 ms.
  // jw.enableSTA() only sets a flag of its own, so it cannot be relied on here.
  WiFi.enableSTA(true);
  WiFi.setSleep(false);
  jw.subscribe(infoCallback);
  jw.subscribe(mdnsCallback);
#endif
#if defined(ESP32) && !defined(LEGACY_PROVISON)
  beginBleProvison();
#endif
#if defined(ESP8266) || defined(LEGACY_PROVISON)
#if JUSTWIFI_ENABLE_SMARTCONFIG
  WiFiConfigSnapshot snapshot;
  if (takeWiFiConfigSnapshot(snapshot) && snapshot.wifiSSID[0] == '\0')
    jw.startSmartConfig();
#endif
  jw.enableAP(false);
  jw.enableAPFallback(true);
  jw.enableSTA(true);
  reloadWiFiConfig();
#endif
}

void loopWiFi()
{

#if defined(ESP32) && !defined(LEGACY_PROVISON)
  // Complete any provisioning write/save that could not acquire from the ESP
  // event callback. This path is fail-fast too and simply retries next loop.
  drainPendingProvisioningCredentials();
#endif

#if defined(ESP8266) || defined(LEGACY_PROVISON)
  if ((WiFi.getMode() & WIFI_AP) && WiFi.isConnected() && connectedOn + 60000 < millis())
  {
    dissableAP();
  }
  jw.loop();
  drainPendingLegacyCredentials();
#if defined(ESP8266)
  MDNS.update();
#endif
#endif

  if (takeMDNSRefreshRequest())
    refreshMDNS();
}
