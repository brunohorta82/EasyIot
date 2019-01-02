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

#include "Config.h"

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
}

void loop() {
  
   if(shouldReboot){
    logger("Rebooting...");
    delay(100);
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
/* Serial.println("Sending mDNS query");
  int n = MDNS.queryService("bhboard", "tcp"); // Send out query for esp tcp services
  Serial.println("mDNS query done");
  if (n == 0) {
    Serial.println("no services found");
  } else {
    Serial.print(n);
    Serial.println(" service(s) found");
    for (int i = 0; i < n; ++i) {
      // Print details for each service found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(MDNS.hostname(i));
      Serial.print(" (");
      Serial.print(MDNS.IP(i));
      Serial.print(":");
      Serial.print(MDNS.port(i));
      Serial.println(")");
    }
  }
  Serial.println();

  Serial.println("loop() next");*/
}
