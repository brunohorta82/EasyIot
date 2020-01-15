#include "CloudIO.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "constants.h"
#include "Switches.h"
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
    subscribeOnMqttCloudIO(topic.c_str());
    topic.replace("/set", "/status");
    mqttClient.publish(topic.c_str(), 0, false, sw.mqttPayload);
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
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
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

void onMqttPublish(uint16_t packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
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
  Serial.println(user);
  Serial.println(pw);
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
}
WiFiClient client;
HTTPClient http;
void connectoToCloudIO()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  String payload = "";
  if (getAtualSwitchesConfig().items.empty())
  {
    payload = "[]";
  }

  size_t s = getAtualSwitchesConfig().items.size();
  const size_t CAPACITY = JSON_ARRAY_SIZE(s) + s * (JSON_OBJECT_SIZE(12) + sizeof(SwitchT));
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
    sdoc["alexaSupport"] = sw.alexaSupport;
  }
  serializeJson(doc, payload);
  http.begin(client, "http://easyiot.bhonofre.pt/devices");
  //http.begin(client, "http://192.168.187.94:8080/devices");
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
