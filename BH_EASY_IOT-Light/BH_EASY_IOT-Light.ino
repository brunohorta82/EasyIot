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


#include "Libs.h"
#include "Config.h"
Timing timerStats;

void checkServices() {
    if (laodDefaults) {
        SPIFFS.format();
        shouldReboot = true;
    }

    if (wifiUpdated) {
        saveConfig();
        reloadWiFiConfig();
        wifiUpdated = false;
    }
    if (needScan()) {
        scanNewWifiNetworks();
    }
    if (reloadMqttConfiguration) {
        setupMQTT();
    }
}

void setup() {
    Serial.begin(115200);
    loadStoredConfiguration();
    loadStoredRelays();
    loadStoredSwitchs();
    loadStoredSensors();
    setupWiFi();
    setupWebserver();
    timerStats.begin(0);
    startDiscovery();
}

void stats() {
    if (timerStats.onTimeout(60000)) {
        publishOnMqtt(getBaseTopic() + "/stats", String(ESP.getFreeHeap(), DEC), false);
        if (WiFi.isConnected() && (WiFi.getMode() & WIFI_AP)) {
          dissableAP();  
          logger("[WI-FI] AUTO TURN OFF AP") ;
        }
        }
    }
void loop() {
    MDNS.update();
    if (autoUpdate) {
        autoUpdate = false;
        actualUpdate();
    }
    stats();
   
    if (shouldReboot) {
        logger("Rebooting...");
        shouldReboot = false;
        ESP.restart();
        return;
    }

    loopSwitchs();
    loopSensors();
    loopWiFi();
    checkServices();
    mqttMsgDigest();
    loopDiscovery();

}


void actualUpdate() {
    WiFiClient client;
    ESPhttpUpdate.update(client, getUpdateUrl());
}
