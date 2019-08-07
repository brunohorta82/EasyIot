#include "WiFi.h"
#include "constants.h"

String getApName()
{
  String version = String(VERSION);
  version.replace(".", "x");
  return "EasyIot-" + String(ESP.getChipId()) + "-" + version;
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
    jw.addNetwork(getAtualConfig().wifiSSID2, getAtualConfig().wifiSecret2, getAtualConfig().wifiIp, getAtualConfig().wifiGw, getAtualConfig().wifiMask, getAtualConfig().wifiGw, false);
  }
  else
  {
    jw.addNetwork(getAtualConfig().wifiSSID, getAtualConfig().wifiSecret);
    jw.addNetwork(getAtualConfig().wifiSSID2, getAtualConfig().wifiSecret2);
  }
}
void infoWifi()
{

  if (WiFi.isConnected())
  {
    uint8_t *bssid = WiFi.BSSID();
    Log.notice("%s MODE STA -------------------------------------" CR, tags::wifi);
    Log.notice("%s SSID %s  " CR, tags::wifi, WiFi.SSID().c_str());
    Log.notice("%s BSSID %s:%s:%s:%s" CR, tags::wifi, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    Log.notice("%s CH %d   " CR, tags::wifi, WiFi.channel());
    Log.notice("%s RSSI %d " CR, tags::wifi, WiFi.RSSI());
    Log.notice("%s IP  %d  " CR, tags::wifi, WiFi.localIP().toString().c_str());
    Log.notice("%s MAC %s  " CR, tags::wifi, WiFi.macAddress().c_str());
    Log.notice("%s GW   %s " CR, tags::wifi, WiFi.gatewayIP().toString().c_str());
    Log.notice("%s MASK %s " CR, tags::wifi, WiFi.subnetMask().toString().c_str());
    Log.notice("%s DNS  %s " CR, tags::wifi, WiFi.dnsIP().toString().c_str());
    Log.notice("%s HOST %s " CR, tags::wifi, WiFi.hostname().c_str());
    Log.notice("%s ----------------------------------------------" CR);
  }

  if (WiFi.getMode() & WIFI_AP)
  {
    Log.notice("%s MODE AP --------------------------------------" CR, tags::wifi);
    Log.notice("%s SSID %s " CR, tags::wifi, jw.getAPSSID().c_str());
    Log.notice("%s IP  %s  " CR, tags::wifi, WiFi.softAPIP().toString().c_str());
    Log.notice("%s MAC  %s " CR, tags::wifi, WiFi.softAPmacAddress().c_str());
    Log.notice("----------------------------------------------" CR);
  }
}

void scanNewWifiNetworks()
{
  unsigned char result = WiFi.scanNetworks();
  if (result == WIFI_SCAN_FAILED)
  {
    //  publishOnEventSource("wifi-networks","Scan Failed");
    Log.error("%s Scan Failed" CR, tags::wifi);
  }
  else if (result == 0)
  {
    //   publishOnEventSource("wifi-networks","No networks found");
    Log.notice("%s No networks found" CR, tags::wifi);
  }
  else
  {
    for (int8_t i = 0; i < result; ++i)
    {
      String ssid_scan;
      int32_t rssi_scan;
      uint8_t sec_scan;
      uint8_t *BSSID_scan;
      int32_t chan_scan;
      bool hidden_scan;
      char buffer[128];
      WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);
      snprintf_P(buffer, sizeof(buffer),
                 PSTR("BSSID: %02X:%02X:%02X:%02X:%02X:%02X SEC: %s RSSI: %3d CH: %2d SSID: %s"),
                 BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], BSSID_scan[6],
                 (sec_scan != ENC_TYPE_NONE ? "YES" : "NO "),
                 rssi_scan,
                 chan_scan,
                 (char *)ssid_scan.c_str());
    }
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
  case MESSAGE_TURNING_OFF:
    Log.notice("%s Turning OFF" CR, tags::wifi);
    break;
  case MESSAGE_TURNING_ON:
    Log.notice("%s Turning ON" CR, tags::wifi);
    break;
  case MESSAGE_SCANNING:
    Log.notice("%s Scanning" CR, tags::wifi);
    break;
  case MESSAGE_SCAN_FAILED:
    Log.error("%s Scan failed" CR, tags::wifi);
    break;
  case MESSAGE_NO_NETWORKS:
    Log.warning("%s No networks found" CR, tags::wifi);

    break;
  case MESSAGE_NO_KNOWN_NETWORKS:
    Log.warning("%s No known networks found" CR, tags::wifi);
    break;
  case MESSAGE_FOUND_NETWORK:
    Log.warning("%s Network found %s" CR, tags::wifi, parameter);
    break;
  case MESSAGE_CONNECTING:
    Log.notice("%s Connecting to %s" CR, tags::wifi, parameter);
    break;
  case MESSAGE_CONNECT_WAITING:
    // too much noise
    break;
  case MESSAGE_CONNECT_FAILED:
    Log.error("%s Could not connect to %s" CR, tags::wifi, parameter);
    break;
  case MESSAGE_CONNECTED:
    infoWifi();
    break;
  case MESSAGE_DISCONNECTED:
    Log.warning("%s Disconnected" CR, tags::wifi);
    break;
  case MESSAGE_ACCESSPOINT_CREATED:
    infoWifi();
    break;
  case MESSAGE_ACCESSPOINT_DESTROYED:
    Log.notice("%s Disconnecting access point" CR, tags::wifi);
    break;
  case MESSAGE_ACCESSPOINT_CREATING:
    Log.notice("%s Creating access point" CR, tags::wifi);
    break;
  case MESSAGE_ACCESSPOINT_FAILED:
    Log.error("%s Could not create access point" CR, tags::wifi);
    break;
  case MESSAGE_WPS_START:
    Log.notice("%s WPS started" CR, tags::wifi);
    break;
  case MESSAGE_WPS_SUCCESS:
    Log.notice("%s WPS succeded!" CR, tags::wifi);
    break;
  case MESSAGE_WPS_ERROR:
    Log.error("%s WPS failed" CR, tags::wifi);
    break;
  case MESSAGE_SMARTCONFIG_START:
    Log.notice("%s Smart Config started" CR, tags::wifi);
    break;
  case MESSAGE_SMARTCONFIG_SUCCESS:
    Log.notice("%s mart Config succeded!" CR, tags::wifi);
    break;
  case MESSAGE_SMARTCONFIG_ERROR:
    Log.error("%s Smart Config failed" CR, tags::wifi);
  case MESSAGE_HOSTNAME_ERROR:
    Log.error("%s Hostname Error" CR, tags::wifi);
    break;
    break;
  }
}
void setupWiFi()
{
  jw.setHostname(getAtualConfig().nodeId);
  jw.subscribe(infoCallback);
  jw.setSoftAP(getAtualConfig().apName, getAtualConfig().apSecret);
  jw.enableAP(false);
  jw.enableAPFallback(true);
  jw.enableSTA(true);
  reloadWiFiConfig();
}

void loopWiFi()
{
  jw.loop();
}
size_t systemJSONStatus(Print &output)
{
  const size_t CAPACITY = JSON_OBJECT_SIZE(13) + 400;
  StaticJsonDocument<CAPACITY> doc;
  JsonObject object = doc.to<JsonObject>();
  object["wifiIp"] = WiFi.localIP().toString();
  object["wifiAPSSID"] = jw.getAPSSID();
  object["wifiSSID"] = WiFi.SSID();
  object["status"] = jw.connected();
  object["signal"] = WiFi.RSSI();
  object["wifiMask"] = WiFi.subnetMask().toString();
  object["wifiGw"] = WiFi.gatewayIP().toString();
  object["mac"] = WiFi.softAPmacAddress();
  object["channel"] = WiFi.channel();
  object["mode"] = (int)WiFi.getMode();
  object["mqttConnected"] = getMqttState();
  object["freeHeap"] = String(ESP.getFreeHeap());
  return serializeJson(doc, output);
}
