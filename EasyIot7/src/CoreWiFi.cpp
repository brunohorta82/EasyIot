#include "CoreWiFi.h"
#include "constants.h"
#include <JustWifi.h>
#include "Config.h"
#include "WebServer.h"
#include "Mqtt.h"
#include "CloudIO.h"
#include <esp-knx-ip.h>
int retryCount = 0;
unsigned long connectedOn = 0ul;
String getApName()
{
  String version = String(VERSION, 3);
  version.replace(".", "x");
  return "OnOfre-" + String(getAtualConfig().chipId) + "-" + version;
}
bool wifiConnected()
{
  return jw.connectable();
}
void reloadWiFiConfig()
{
  jw.disconnect();
  jw.setHostname(getAtualConfig().nodeId);
  jw.cleanNetworks();
  jw.setSoftAP(getApName().c_str(), getAtualConfig().apSecret);
  if (getAtualConfig().staticIp)
  {
    jw.addNetwork(getAtualConfig().wifiSSID, getAtualConfig().wifiSecret, getAtualConfig().wifiIp, getAtualConfig().wifiGw, getAtualConfig().wifiMask, getAtualConfig().wifiGw, true);
  }
  else if (strlen(getAtualConfig().wifiSecret) > 0)
  {
    jw.addNetwork(getAtualConfig().wifiSSID, getAtualConfig().wifiSecret);
  }
  else
  {
    jw.addNetwork(getAtualConfig().wifiSSID);
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
    refreshMDNS(getAtualConfig().nodeId);
  }
}
void enableScan()
{
  jw.enableScan(true);
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
    if (strlen(getAtualConfig().wifiSSID) == 0)
    {
      strlcpy(getAtualConfig().wifiSSID, WiFi.SSID().c_str(), sizeof(getAtualConfig().wifiSSID));
      strlcpy(getAtualConfig().wifiSecret, WiFi.psk().c_str(), sizeof(getAtualConfig().wifiSecret));
    }

    knx.start();
    infoWifi();
    connectoToCloudIO();
    setupWebserverAsync();
    break;

  case MESSAGE_ACCESSPOINT_CREATED:
    infoWifi();
    setupWebserverAsync();
    break;
  }
}

void refreshMDNS(const char *lastName)
{
  bool success = false;
#ifdef ESP32
  success = MDNS.begin(String(getAtualConfig().nodeId).c_str());
#endif
#ifdef ESP8266
  MDNS.removeService(lastName, "bhonofre", "tcp");
  MDNS.close();
  success = MDNS.begin(String(getAtualConfig().nodeId), INADDR_ANY, 10);
#endif

  if (success)
  {
    MDNS.addService("bhonofre", "tcp", 80);
    MDNS.addServiceTxt("bhonofre", "tcp", "hardwareId", String(getAtualConfig().chipId));
    MDNS.addServiceTxt("bhonofre", "tcp", "firmware", String(VERSION, 3));
    MDNS.addServiceTxt("bhonofre", "tcp", "wifi", String(getAtualConfig().wifiSSID));
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
void mdnsCallback(justwifi_messages_t code, char *parameter)
{

  if (code == MESSAGE_CONNECTED)
  {
    refreshMDNS(getAtualConfig().nodeId);
  }
}
void setupWiFi()
{
  WiFi.setSleep(false);
  jw.setHostname(getAtualConfig().nodeId);
  jw.subscribe(infoCallback);
  jw.subscribe(mdnsCallback);

#if JUSTWIFI_ENABLE_SMARTCONFIG
  if (strlen(getAtualConfig().wifiSSID) == 0)
    jw.startSmartConfig();
#endif
  jw.setSoftAP(getApName().c_str(), getAtualConfig().apSecret);
  jw.enableAP(false);
  jw.enableAPFallback(true);
  jw.enableSTA(true);
  reloadWiFiConfig();
}

void loopWiFi()
{
  if ((WiFi.getMode() & WIFI_AP) && WiFi.isConnected() && connectedOn + 60000 < millis())
  {
    dissableAP();
  }
  jw.loop();
#ifdef ESP8266
  MDNS.update();
#endif
  webserverServicesLoop();
}
