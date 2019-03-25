/*

BH FIRMWARE

Copyright (C) 2017-2018 by Bruno Horta <brunohorta82 at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.


You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <ESP8266httpUpdate.h>
#include <fauxmoESP.h>
fauxmoESP fauxmo;

const char * updateUrl="http://release.bhonofre.pt/release.bin";
#include "Config.h"
Timing timerStats;
void checkServices(){
  if(laodDefaults){
    SPIFFS.format();
    shouldReboot = true;
  }

  if(wifiUpdated){
    saveConfig();
    reloadWiFiConfig();
    wifiUpdated = false;
    }
  if(needScan()){
      scanNewWifiNetworks();
   }
   if(reloadMqttConfiguration){
     setupMQTT();
   }
}

void setup() {
  Serial.begin(115200);
  loadStoredConfiguration();
  #ifdef BHONOFRE
    loadStoredRelays();
    loadStoredSwitchs();
    loadStoredSensors();
  #endif
  setupWiFi(); 
  setupWebserver();
  #ifdef BHPZEM
    setupBHPzem();
    setupDisplay();
  #endif
   timerStats.begin(0);

   #ifdef BHONOFRE
JsonArray& _devices = getStoredSwitchs();
  for(int i  = 0 ; i < _devices.size() ; i++){ 
    JsonObject& switchJson = _devices[i];    
    if(switchJson.get<bool>("discoveryDisabled")){
      continue;
     }  
    String _name =switchJson.get<String>("name");
     fauxmo.addDevice(_name.c_str());
 
  }
    fauxmo.createServer(false);
    fauxmo.setPort(80); // required for gen3 devices
    fauxmo.enable(true);
   fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
     stateSwitchByName(String(device_name),state ? "ON" : "OFF");
    });
    #endif
    
}
void stats(){
   if (timerStats.onTimeout(60000) ){
     publishOnMqtt(getBaseTopic()+"/stats",String(ESP.getFreeHeap(),DEC),false);
   }
  
}
void loop() {
   MDNS.update();
   if(autoUpdate){
    autoUpdate = false;
    actualUpdate();
    }
    stats();
  if(adopted){
   saveConfig();
   shouldReboot = true;
   adopted = false;
  }
   if(shouldReboot){
    logger("Rebooting...");
    shouldReboot = false;
    ESP.restart();
    return;
  }
  #ifdef BHPZEM
    loopBHPzem();
   #endif
  #ifdef BHONOFRE
    loopSwitchs();
    loopSensors();
  #endif
  loopWiFi();
  checkServices();
  mqttMsgDigest();
  #ifdef BHONOFRE
  fauxmo.handle();
  #endif
}





bool actualUpdate(){
  Serial.println("Start Update");
  WiFiClient client;
     t_httpUpdate_return ret = ESPhttpUpdate.update(client, updateUrl);
     switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}
