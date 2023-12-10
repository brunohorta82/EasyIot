#include "CoreWiFi.h"
#include "Constants.h"
#if defined(ESP8266) || defined(LEGACY_PROVISON)
#include <JustWifi.h>
#endif
#include "ConfigOnofre.h"
#include "WebServer.h"
#include <esp-knx-ip.h>
int retryCount = 0;
unsigned long connectedOn = 0ul;
extern ConfigOnofre config;
#if defined(ESP32) && !defined(LEGACY_PROVISON)
const char *pop = "abcd1234";           // Proof of possession - otherwise called a PIN - string provided by the device, entered by user in the phone app
const char *service_name = "ONOFRE_13"; // Name of your device (the Espressif apps expects by default device name starting with "Prov_")
const char *service_key = NULL;         // Password used for SofAP method (NULL = no password needed)
bool reset_provisioned = true;          // When true the library will automatically delete previously provisioned data.
void SysProvEvent(arduino_event_t *sys_event)
{
  switch (sys_event->event_id)
  {
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    Serial.print("\nConnected IP address : ");
    Serial.println(IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr));
    setupWebPanel();
    startWebserver();
    knx.start();
    config.requestCloudIOSync();
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    Serial.println("\nDisconnected. Connecting to the AP again... ");
    break;
  case ARDUINO_EVENT_PROV_START:
    Serial.println("\nProvisioning started\nGive Credentials of your access point using smartphone app");
    break;
  case ARDUINO_EVENT_PROV_CRED_RECV:
  {
    Serial.println("\nReceived Wi-Fi credentials");
    Serial.print("\tSSID : ");
    Serial.println((const char *)sys_event->event_info.prov_cred_recv.ssid);
    Serial.print("\tPassword : ");
    Serial.println((char const *)sys_event->event_info.prov_cred_recv.password);
    // strlcpy(config.wifiSSID, (const char *)sys_event->event_info.prov_cred_recv.ssid, sizeof(config.wifiSSID));
    // strlcpy(config.wifiSecret, (char const *)sys_event->event_info.prov_cred_recv.password, sizeof(config.wifiSecret));
    break;
  }
  case ARDUINO_EVENT_PROV_CRED_FAIL:
  {
    Serial.println("\nProvisioning failed!\nPlease reset to factory and retry provisioning\n");
    if (sys_event->event_info.prov_fail_reason == WIFI_PROV_STA_AUTH_ERROR)
      Serial.println("\nWi-Fi AP password incorrect");
    else
      Serial.println("\nWi-Fi AP not found....Add API \" nvs_flash_erase() \" before beginProvision()");
    break;
  }
  case ARDUINO_EVENT_PROV_CRED_SUCCESS:
    Serial.println("\nProvisioning Successful");
    // config.save();
    break;
  case ARDUINO_EVENT_PROV_END:
    Serial.println("\nProvisioning Ends");
    break;
  default:
    break;
  }
}
#endif
String getApName()
{
  String version = String(VERSION, 3);
  version.replace(".", "x");
  if (strcmp(config.nodeId, config.chipId) == 0)
    return "OnOfre-" + String(config.chipId) + "-" + version;
  return String(config.nodeId) + "-" + version;
}
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
    const size_t CAPACITY = JSON_ARRAY_SIZE(result) + 200;
    DynamicJsonDocument doc(CAPACITY);
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
void reloadWiFiConfig()
{
  jw.disconnect();
  jw.setHostname(config.nodeId);
  jw.cleanNetworks();
  jw.setSoftAP(getApName().c_str(), config.accessPointPassword);

  if (strlen(config.wifiSecret) > 0)
  {
    if (config.dhcp)
    {
      jw.addNetwork(config.wifiSSID, config.wifiSecret);
    }
    else
    {
      jw.addNetwork(config.wifiSSID, config.wifiSecret, config.wifiIp, config.wifiGw, config.wifiMask, config.wifiGw, true);
    }
  }
  else
  {
    jw.addNetwork(config.wifiSSID);
  }
}
void infoWifi()
{

  if (WiFi.isConnected())
  {
    connectedOn = millis();
#ifdef DEBUG_ONOFRE
    Log.notice("%s MODE STA -------------------------------------" CR, tags::wifi);
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
    Log.notice("%s MODE AP --------------------------------------" CR, tags::wifi);
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
    if (strlen(config.wifiSSID) == 0)
    {
      strlcpy(config.wifiSSID, WiFi.SSID().c_str(), sizeof(config.wifiSSID));
      strlcpy(config.wifiSecret, WiFi.psk().c_str(), sizeof(config.wifiSecret));
    }
    setupWebPanel();
    startWebserver();
    knx.start();
    config.requestCloudIOSync();
    infoWifi();
    break;

  case MESSAGE_ACCESSPOINT_CREATED:
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
    refreshMDNS(config.nodeId);
  }
}
#endif
bool wifiConnected()
{
  return WiFi.status() == WL_CONNECTED;
}
void refreshMDNS(const char *lastName)
{
  bool success = false;
#ifdef ESP32
  MDNS.end();
  success = MDNS.begin(String(config.nodeId).c_str());
#endif
#ifdef ESP8266
  MDNS.removeService(lastName, "bhonofre", "tcp");
  MDNS.close();
  success = MDNS.begin(String(config.nodeId), INADDR_ANY, 10);
#endif

  if (success)
  {
    MDNS.addService("bhonofre", "tcp", 80);
    MDNS.addServiceTxt("bhonofre", "tcp", "hardwareId", String(config.chipId));
    MDNS.addServiceTxt("bhonofre", "tcp", "firmware", String(VERSION, 3));
    MDNS.addServiceTxt("bhonofre", "tcp", "wifi", String(config.wifiSSID));
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

void setupWiFi()
{
#if defined(ESP8266) || defined(LEGACY_PROVISON)
  WiFi.setSleep(false);
  jw.setHostname(config.nodeId);
  jw.subscribe(infoCallback);
  jw.subscribe(mdnsCallback);
#endif
#if defined(ESP32) && !defined(LEGACY_PROVISON)
  WiFi.setSleep(true);
  WiFi.onEvent(SysProvEvent);
  Serial.println("Begin Provisioning using BLE");
  // Sample uuid that user can pass during provisioning using BLE
  uint8_t uuid[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
                      0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name, service_key, uuid, true);
  log_d("ble qr");
  WiFiProv.printQR(service_name, pop, "ble");
#endif
#if defined(ESP8266) || defined(LEGACY_PROVISON)
#if JUSTWIFI_ENABLE_SMARTCONFIG
  if (strlen(config.wifiSSID) == 0)
    jw.startSmartConfig();
#endif
  jw.setSoftAP(getApName().c_str(), config.accessPointPassword);
  jw.enableAP(false);
  jw.enableAPFallback(true);
  jw.enableSTA(true);
  reloadWiFiConfig();
#endif
}

void loopWiFi()
{

#if defined(ESP8266) || defined(LEGACY_PROVISON)
  if ((WiFi.getMode() & WIFI_AP) && WiFi.isConnected() && connectedOn + 60000 < millis())
  {
    dissableAP();
  }
  jw.loop();
#if defined(ESP8266)
  MDNS.update();
#endif
#endif
}
