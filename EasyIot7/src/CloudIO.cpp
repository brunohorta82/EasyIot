#include "CloudIO.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "constants.h"
#include "Switches.h"
#include <ESP8266HTTPClient.h>
AsyncMqttClient mqttClient;

Ticker mqttReconnectTimer;
void connectToCloundIO() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
   publishFeactures();
}

bool mqttCloudIOConnected(){
    return mqttClient.connected();
}
void subscribeOnMqttCloudIO(const char *topic)
{

    if (!mqttCloudIOConnected())
    {
#ifdef DEBUG
        Log.warning("%s Required Mqtt connection" CR, tags::mqtt);
#endif
        return;
    }
    mqttClient.subscribe(topic,0);
}
void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  for (auto &sw : getAtualSwitchesConfig().items)
        {
            Serial.println("GO");
            String topic;
            topic.reserve(200);
            topic.concat("brunohorta82");
            topic.concat("/");
            topic.concat(getAtualConfig().chipId);
            topic.concat("/");
            topic.concat(sw.family);
            topic.concat("/");
            topic.concat(sw.id);
            topic.concat("/set");
            subscribeOnMqttCloudIO(topic.c_str());
            Serial.println(topic);
            topic.replace("/set","/status");
            mqttClient.publish(topic.c_str(),0,false, sw.mqttPayload);
        }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
    Serial.println("Bad server fingerprint.");
  }
if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToCloundIO);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setupCloudIO() {
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  mqttClient.setClientId(getAtualConfig().chipId);
  mqttClient.setCredentials("brunohorta82","xptoxpto");
}

void publishFeactures()
{
    if (WiFi.status() != WL_CONNECTED)
        return;
    WiFiClient client;
    HTTPClient http;

    String queryString;
    queryString.reserve(200);
    queryString.concat(constanstsCloudIO::apiDns);
    queryString.concat("/devices/features/add-or-update/");
    queryString.concat(getAtualConfig().chipId);
    Serial.println(queryString);
     String payload ="";
    if(getAtualSwitchesConfig().items.empty()){
      payload= "[]";
    }
         
        size_t s = getAtualSwitchesConfig().items.size();
  const size_t CAPACITY = JSON_ARRAY_SIZE(s) + s* (JSON_OBJECT_SIZE(7) + sizeof(SwitchT));
  DynamicJsonDocument doc(CAPACITY);

  for (const auto &sw : getAtualSwitchesConfig().items)
  {
    JsonObject sdoc = doc.createNestedObject();
    sdoc["id"] = sw.id;
    sdoc["name"] = sw.name;
    sdoc["family"] = sw.family;
    sdoc["stateControl"] = sw.stateControl;
    sdoc["alexaSupport"] = sw.alexaSupport;
    sdoc["chipId"] = getAtualConfig().chipId;
    
  }
  
serializeJson(doc,payload);
Serial.println(payload);
  http.addHeader("Content-Type", "application/json");
    if (http.begin(client,queryString))
    {
        

        int httpCode = http.POST(payload);

        if (httpCode < 0)
        {
#ifdef DEBUG
Serial.println(httpCode);
            Log.error("%s GET Request, error %s" CR, tags::cloudIO, http.errorToString(httpCode).c_str());
#endif
        }

        http.end();
    }
#ifdef DEBUG
    else
    {
        Log.error("%s Unable to connect" CR, tags::cloudIO);
    }
#endif
}
