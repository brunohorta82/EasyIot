#include "CloudIO.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "constants.h"
#include "Switches.h"
#include "Sensors.h"
#include <ESP8266HTTPClient.h>
AsyncMqttClient mqttClient;
String user;
String pw;
bool cloudIOReadyToConnect = false;
Ticker mqttReconnectTimer;
Ticker cloudIO;

void connectToClounIOMqtt()
{
#ifdef DEBUG
  Log.error("%s Connecting to MQTT..." CR, tags::cloudIO);
#endif
  mqttClient.connect();
}

bool mqttCloudIOConnected()
{
  return mqttClient.connected();
}
void notifyStateToCloudIO(const char *topic, const char *state)
{
  if (!mqttClient.connected())
    return;
  mqttClient.publish(topic, 0, false, state);
}
void subscribeOnMqttCloudIO(const char *topic)
{
  if (!mqttCloudIOConnected())
  {
#ifdef DEBUG
    Log.warning("%s Required Mqtt connection" CR, tags::cloudIO);
#endif
    return;
  }
  mqttClient.subscribe(topic, 0);
}
void onMqttConnect(bool sessionPresent)
{
#ifdef DEBUG
  Log.warning("%s Connected to MQTT." CR, tags::cloudIO);
#endif
  for (auto &sw : getAtualSwitchesConfig().items)
  {
    String topic;
    topic.reserve(200);
    topic.concat(user);
    topic.concat("/");
    topic.concat(getAtualConfig().chipId);
    topic.concat("/");
    topic.concat(sw.family);
    topic.concat("/");
    topic.concat(sw.id);
    topic.concat("/set");
    topic.toLowerCase();
    strlcpy(sw.mqttCloudCommandTopic, topic.c_str(), sizeof(sw.mqttCloudCommandTopic));
    subscribeOnMqttCloudIO(sw.mqttCloudCommandTopic);

    topic.replace("/set", "/status");

    strlcpy(sw.mqttCloudStateTopic, topic.c_str(), sizeof(sw.mqttCloudStateTopic));
    mqttClient.publish(sw.mqttCloudStateTopic, 0, false, sw.mqttPayload);
  }
  for (auto &ss : getAtualSensorsConfig().items)
  {
    String topic;
    topic.reserve(200);
    topic.concat(user);
    topic.concat("/");
    topic.concat(getAtualConfig().chipId);
    topic.concat("/");
    topic.concat(ss.deviceClass);
    topic.concat("/");
    topic.concat(ss.id);
    topic.concat("/status");
    topic.toLowerCase();
    strlcpy(ss.mqttCloudStateTopic, topic.c_str(), sizeof(ss.mqttCloudStateTopic));
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{

#ifdef DEBUG
  Log.warning("%s Disconnected from MQTT." CR, tags::cloudIO);
#endif
  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(10, connectToClounIOMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
}

void onMqttUnsubscribe(uint16_t packetId)
{
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  char msg[100];
#ifdef DEBUG
  Log.warning("%s Message from MQTT. %s %s" CR, tags::cloudIO, topic, payload);
#endif
  strlcpy(msg, payload, len + 1);
  mqttCloudSwitchControl(getAtualSwitchesConfig(), topic, msg);
}

void onMqttPublish(uint16_t packetId)
{
}

void setupCloudIO()
{

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setCleanSession(true);
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  mqttClient.setClientId(getAtualConfig().chipId);
  mqttClient.setCredentials(user.c_str(), pw.c_str());
  connectToClounIOMqtt();
}
bool tryCloudConnectio()
{
  if (!cloudIOReadyToConnect && user.length() > 0 && pw.length() > 0)
  {
#ifdef DEBUG
    Log.error("%s Ready to try..." CR, tags::cloudIO);
#endif
    cloudIOReadyToConnect = true;
    setupCloudIO();
  }
  return true;
}
WiFiClient client;
HTTPClient http;
void connectoToCloudIO()
{
if (WiFi.status() != WL_CONNECTED)
    return;

  String payload = "";
  size_t s = getAtualSwitchesConfig().items.size();
  size_t ss = getAtualSensorsConfig().items.size();
  const size_t CAPACITY = JSON_ARRAY_SIZE(s + ss) + (s * JSON_OBJECT_SIZE(7) + sizeof(SwitchT)) + (ss * (JSON_OBJECT_SIZE(7) + sizeof(SensorT)));
  DynamicJsonDocument doc(CAPACITY);
  JsonObject device = doc.to<JsonObject>();
  device["chipId"] = String(ESP.getChipId());
  device["currentVersion"] = String(VERSION, 3);
  device["nodeId"] = getAtualConfig().nodeId;
  device["firmwareMode"] = constantsConfig::firmwareMode;
  device["macAddr"] = WiFi.softAPmacAddress();
  device["configKey"] = getAtualConfig().configkey;
  JsonArray feactures = device.createNestedArray("features");
  for (const auto &sw : getAtualSwitchesConfig().items)
  {
    JsonObject sdoc = feactures.createNestedObject();
    sdoc["id"] = sw.id;
    sdoc["name"] = sw.name;
    sdoc["family"] = sw.family;
    sdoc["stateControl"] = sw.stateControl;
    sdoc["cloudIOSupport"] = sw.alexaSupport;
  }


  for (const auto &ss : getAtualSensorsConfig().items)
  {
    JsonObject sdoc = feactures.createNestedObject();
      sdoc["id"] = ss.id;
      sdoc["name"] = ss.name;
      sdoc["family"] = ss.deviceClass;
      sdoc["cloudIOSupport"] = true;
    switch (ss.type)
    {
    
    case DHT_11:
    case DHT_21:
    case DHT_22:
    {
      JsonObject sdoc2 = feactures.createNestedObject();
      sdoc2["id"] = ss.id;
      sdoc2["name"] = ss.name;
      sdoc2["family"] = "HUMIDITY";
      sdoc2["cloudIOSupport"] = true;
    }
    break;
    default:
    break;
    }
  }
  serializeJson(doc, payload);
  Serial.println(payload);
  http.begin(client, "http://easyiot.bhonofre.pt/devices");
  http.addHeader("Content-Type", "application/json");
#ifdef DEBUG
  Log.error("%s [HTTP] POST" CR, tags::cloudIO);
#endif
  // start connection and send HTTP header and body
  int httpCode = http.POST(payload.c_str());

  // httpCode will be negative on error
  if (httpCode > 0)
  {

#ifdef DEBUG
    Log.error("%s [HTTP] POST... code: %d" CR, tags::cloudIO, httpCode);
#endif
    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = http.getString();
      StaticJsonDocument<200> resp;
      DeserializationError error = deserializeJson(doc, payload);
      const char *_user = doc["username"];
      const char *_pw = doc["password"];
#ifdef DEBUG
      Log.error("%s USER: %s PASSWORD: %s" CR, tags::cloudIO, _user, _pw);
#endif
      if (!error && user && pw)
      {
#ifdef DEBUG
        Log.error("%s SETUP MQTT CLOUD" CR, tags::cloudIO);
#endif
        user = String(_user);
        pw = String(_pw);

        if (mqttClient.connected())
        {
          mqttClient.disconnect();
        }

        cloudIO.once(5, tryCloudConnectio);
      }
      else
      {
#ifdef DEBUG
        Log.error("%s NO USER FOUND" CR, tags::cloudIO);
#endif
      }

#ifdef DEBUG
      Log.error("%s [HTTP] POST... Result: %s" CR, tags::cloudIO, payload.c_str());
#endif
    }
    else
    {
#ifdef DEBUG
      Log.error("%s [HTTP] POST... failed, error: %s" CR, tags::cloudIO, http.errorToString(httpCode).c_str());
#endif
    }
  }
}
