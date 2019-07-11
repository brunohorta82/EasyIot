#include "WiFi.h"
#define WIFI_TAG "[WIFI]"

void reloadWiFiConfig(){
       jw.disconnect(); 
       jw.setHostname(getAtualConfig().hostname);
       jw.cleanNetworks();
       jw.setSoftAP(getAtualConfig().hostname,getAtualConfig().apSecret);
       
       if(getAtualConfig().staticIp){
        jw.addNetwork(getAtualConfig().wifiSSID, getAtualConfig().wifiSecret,getAtualConfig().wifiIp,getAtualConfig().wifiGw,getAtualConfig().wifiMask,getAtualConfig().wifiGw);
        jw.addNetwork(getAtualConfig().wifiSSID2, getAtualConfig().wifiSecret2,getAtualConfig().wifiIp,getAtualConfig().wifiGw,getAtualConfig().wifiMask,getAtualConfig().wifiGw);
       }else{
        jw.addNetwork(getAtualConfig().wifiSSID, getAtualConfig().wifiSecret);
        jw.addNetwork(getAtualConfig().wifiSSID2, getAtualConfig().wifiSecret2);
      }
 }
 void infoWifi() {

    if (WiFi.isConnected()) {
        uint8_t * bssid = WiFi.BSSID();
        logger(WIFI_TAG,"MODE STA -------------------------------------");
        logger(WIFI_TAG,"SSID  "+String(WiFi.SSID().c_str()));
        logger(WIFI_TAG,"BSSID "+String(bssid[0])+":"+String(bssid[1])+":"+String(bssid[2])+":"+String(bssid[3])+":"+String(bssid[4])+":"+String(bssid[5]));
        logger(WIFI_TAG,"CH  "+ String(WiFi.channel()));
        logger(WIFI_TAG,"RSSI  "+String(WiFi.RSSI()));
        logger(WIFI_TAG,"IP   "+WiFi.localIP().toString());
        logger(WIFI_TAG,"MAC   "+String( WiFi.macAddress().c_str()));
        logger(WIFI_TAG,"GW    "+WiFi.gatewayIP().toString());
        logger(WIFI_TAG,"MASK  "+WiFi.subnetMask().toString());
        logger(WIFI_TAG,"DNS   "+WiFi.dnsIP().toString());
        logger(WIFI_TAG,"HOST  "+String( WiFi.hostname().c_str()));
        logger(WIFI_TAG,"----------------------------------------------");
       // updateNetworkConfig();
    }

    if (WiFi.getMode() & WIFI_AP) {
        logger(WIFI_TAG,"MODE AP --------------------------------------");
        logger(WIFI_TAG,"SSID  "+String( jw.getAPSSID().c_str()));
        logger(WIFI_TAG,"IP    "+String( WiFi.softAPIP().toString().c_str()));
        logger(WIFI_TAG,"MAC   "+String( WiFi.softAPmacAddress().c_str()));
        logger(WIFI_TAG,"----------------------------------------------");

    }

}

void scanNewWifiNetworks(){
    unsigned char result = WiFi.scanNetworks();
    if (result == WIFI_SCAN_FAILED) {
    //  publishOnEventSource("wifi-networks","Scan Failed");
       logger(WIFI_TAG,"Scan Failed");
    } else if (result == 0) {
    //   publishOnEventSource("wifi-networks","No networks found");
       logger(WIFI_TAG,"No networks found");
    } else {
        for (int8_t i = 0; i < result; ++i) {
            String ssid_scan;
            int32_t rssi_scan;
            uint8_t sec_scan;
            uint8_t* BSSID_scan;
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
                (char *) ssid_scan.c_str()
            );
            String out = String(buffer);
          logger(WIFI_TAG,out);
        }
    }
    WiFi.scanDelete();
  
 }
 void dissableAP(){
  jw.enableAP(false);
  }

void infoCallback(justwifi_messages_t code, char * parameter) {
    String msg = "";
    switch (code){
      case MESSAGE_TURNING_OFF:
      msg = "Turning OFF";
      break;
      case MESSAGE_TURNING_ON:
      msg = "Turning ON";
      break;
      case MESSAGE_SCANNING:
      msg = "Scanning";
      break;
      case MESSAGE_SCAN_FAILED:
      msg = "Scan failed";
      break;
      case MESSAGE_NO_NETWORKS:
      msg = "No networks found";
      break;
      case MESSAGE_NO_KNOWN_NETWORKS:
      msg = "No known networks found";
      break;
      case MESSAGE_FOUND_NETWORK:
      msg = String(parameter);
      break;
      case MESSAGE_CONNECTING:
       msg = String("Connecting to ")+String(parameter);
      break;
      case MESSAGE_CONNECT_WAITING:
        // too much noise
      break;
      case MESSAGE_CONNECT_FAILED:
       msg = "Could not connect to "+String(parameter);
      break;
      case MESSAGE_CONNECTED:
        infoWifi();
      break;
      case MESSAGE_DISCONNECTED:
       msg = "Disconnected";
      break;
      case MESSAGE_ACCESSPOINT_CREATED:
       infoWifi();
      break;
      case MESSAGE_ACCESSPOINT_DESTROYED:
       msg = "Disconnecting access point";
      break;
      case MESSAGE_ACCESSPOINT_CREATING:
       msg = "Creating access point";
      break;
      case MESSAGE_ACCESSPOINT_FAILED:
       msg = "Could not create access point";
      break;
      case MESSAGE_WPS_START:
       msg = "WPS started";
      break;
      case MESSAGE_WPS_SUCCESS:
       msg = "WPS succeded!";
      break;
      case MESSAGE_WPS_ERROR:
       msg = "WPS failed";
      break;
      case MESSAGE_SMARTCONFIG_START:
       msg = "Smart Config started";
      break;
      case MESSAGE_SMARTCONFIG_SUCCESS:
       msg = "Smart Config succeded!";
      break;
      case MESSAGE_SMARTCONFIG_ERROR:
       msg = "Smart Config failed";
       case MESSAGE_HOSTNAME_ERROR:
       msg = "Hostname Error";
       break;
      break;
      
      }
      logger(WIFI_TAG,msg);
}
void setupWiFi(){
  jw.setHostname(getAtualConfig().hostname);
  jw.subscribe(infoCallback);
  jw.setSoftAP(getAtualConfig().apName,getAtualConfig().apSecret);
  jw.enableAP(false);
  jw.enableAPFallback(true);
  jw.enableSTA(true);
  reloadWiFiConfig();
}


void loopWiFi(){
  jw.loop();
}
String wifiJSONStatus(){
  String wifi = "";
  const size_t CAPACITY = JSON_OBJECT_SIZE(11)+350;
  StaticJsonDocument<CAPACITY> doc;
  JsonObject object = doc.to<JsonObject>();
  object["wifiIp"] = WiFi.localIP().toString();
  object["wifiAPSSID"] =jw.getAPSSID();
  object["wifiSSID"] = WiFi.SSID();
  object["status"] = jw.connected();
  object["signal"] = WiFi.RSSI();
  object["wifiMask"] = WiFi.subnetMask().toString();
  object["wifiGw"] =WiFi.gatewayIP().toString();
  object["mac"] =  WiFi.softAPmacAddress();
  object["channel"] =  WiFi.channel();
  object["mode"] = (int) WiFi.getMode();
  serializeJson(doc,wifi);
  return wifi;
}


