#include "CloudIO.h"
#include "Config.h"
#include "CoreWiFi.h"
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "constants.h"
#include "Switches.h"
#include "Sensors.h"
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#endif
#ifdef ESP32
#include <HTTPClient.h>
#endif
extern Config config;
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker cloudIOReconnectTimer;
String topicAvailable;
String topicAction;
int reconectCount = 0;

void disconnectToClounIOMqtt()
{
#ifdef DEBUG_ONOFRE
  Log.error("%s Disconnect to MQTT..." CR, tags::cloudIO);
#endif
  mqttClient.disconnect();
}
void connectToClounIOMqtt()
{
#ifdef DEBUG_ONOFRE
  Log.error("%s Connecting to MQTT..." CR, tags::cloudIO);
#endif
  mqttClient.connect();
}

bool mqttCloudIOConnected()
{
  return mqttClient.connected();
}
void notifyStateToCloudIO(const char *topic, const char *state, size_t length)
{
  if (!mqttClient.connected())
    return;
  mqttClient.publish(topic, 0, true, state, length);
}
void subscribeOnMqttCloudIO(const char *topic)
{
  if (!mqttCloudIOConnected())
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s Required Mqtt connection" CR, tags::cloudIO);
#endif
    return;
  }
  mqttClient.subscribe(topic, 0);
}
void onMqttConnect(bool sessionPresent)
{
#ifdef DEBUG_ONOFRE
  Log.warning("%s Connected to MQTT." CR, tags::cloudIO);
#endif

  topicAction.reserve(200);
  topicAction.concat(config.cloudIOUsername);
  topicAction.concat("/");
  topicAction.concat(getChipId());
  topicAction.concat("/remote-action");
  subscribeOnMqttCloudIO(topicAction.c_str());
  for (auto &sw : config.switches)
  {
    String topic;
    topic.reserve(200);
    topic.concat(config.cloudIOUsername);
    topic.concat("/");
    topic.concat(getChipId());
    topic.concat("/");
    topic.concat(sw.family);
    topic.concat("/");
    topic.concat(sw.id);
    topic.concat("/set");
    subscribeOnMqttCloudIO(topic.c_str());

    topic.replace("/set", "/status");

    if (sw.isCover())
    {
      char dump[4] = {0};
      int l = sprintf(dump, "%d", sw.lastPercentage);
      notifyStateToCloudIO(topic.c_str(), dump, l);
    }
    else
    {
      mqttClient.publish(topic.c_str(), 0, true, sw.getCurrentState().c_str());
    }
  }
  mqttClient.publish(topicAvailable.c_str(), 0, true, "1\0");
  for (auto &ss : getAtualSensorsConfig().items)
  {
    String topic;
    topic.reserve(200);
    topic.concat(config.cloudIOUsername);
    topic.concat("/");
    topic.concat(getChipId());
    topic.concat("/");
    topic.concat(ss.deviceClass);
    topic.concat("/");
    topic.concat(ss.id);
    topic.concat("/status");
    strlcpy(ss.mqttCloudStateTopic, topic.c_str(), sizeof(ss.mqttCloudStateTopic));
    mqttClient.publish(ss.mqttCloudStateTopic, 0, true, ss.lastReading);
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{

#ifdef DEBUG_ONOFRE
  Log.warning("%s Disconnected from MQTT." CR, tags::cloudIO);
#endif
  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(2, connectToClounIOMqtt);
  }
}
bool cloudIOConnected()
{
  return mqttClient.connected();
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
#ifdef DEBUG_ONOFRE
  Log.warning("%s Message from MQTT. %s %s" CR, tags::cloudIO, topic, payload);
#endif
  strlcpy(msg, payload, len + 1);
  if (strcmp(topic, topicAction.c_str()) == 0)
  {
    if (strcmp(msg, "REBOOT") == 0)
    {
      config.requestRestart();
    }
    if (strcmp(msg, "UPDATE") == 0)
    {
      config.requestAutoUpdate();
    }
  }
  else
  {
    mqttSwitchControl(topic, msg);
  }
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
  mqttClient.setWill(topicAvailable.c_str(), 0, true, "0\0");
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  mqttClient.setClientId(getChipId().c_str());
  mqttClient.setCredentials(config.cloudIOUsername, config.cloudIOPassword);
  connectToClounIOMqtt();
}
bool tryCloudConnection()
{
  if (strlen(config.cloudIOUsername) > 0 && strlen(config.cloudIOPassword) > 0)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s Ready to try..." CR, tags::cloudIO);
#endif
    setupCloudIO();
  }
  return true;
}

void cloudIoKeepAlive()
{
  config.requestCloudIOSync();
}
WiFiClient client;
HTTPClient http;
void connectoToCloudIO()
{

  cloudIOReconnectTimer.detach();
  if (WiFi.status() != WL_CONNECTED || reconectCount > 3)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s [HTTP] reconectCount %d" CR, tags::cloudIO, reconectCount);
#endif
    return;
  }

  reconectCount++;
  String payload = "";
  size_t s = config.switches.size();
  size_t ss = config.sensors.size();
  const size_t CAPACITY = JSON_ARRAY_SIZE(s + ss) + (s * JSON_OBJECT_SIZE(8) + sizeof(SwitchT)) + (ss * (JSON_OBJECT_SIZE(7) + sizeof(SensorT)));
  DynamicJsonDocument doc(CAPACITY);
  JsonObject device = doc.to<JsonObject>();
  device["chipId"] = getChipId();
  device["currentVersion"] = String(VERSION, 3);
  device["nodeId"] = config.nodeId;
  device["wifi"] = config.wifiSSID;
  device["macAddr"] = WiFi.macAddress();
  JsonArray feactures = device.createNestedArray("features");
  for (auto &sw : config.switches)
  {
    JsonObject sdoc = feactures.createNestedObject();
    sdoc["id"] = sw.id;
    sdoc["name"] = sw.name;
    sdoc["family"] = sw.family;
    sdoc["shutterPercentage"] = sw.lastPercentage;
    sdoc["stateControl"] = sw.getCurrentState();
    sdoc["cloudIOSupport"] = sw.cloudIOSupport;
    sdoc["knk"] = String(sw.knxAddress[0]) + "." + String(sw.knxAddress[1]) + "." + String(sw.knxAddress[2]);
  }
  for (const auto &ss : getAtualSensorsConfig().items)
  {
    JsonObject sdoc = feactures.createNestedObject();
    sdoc["id"] = ss.id;
    sdoc["name"] = ss.name;
    sdoc["family"] = ss.deviceClass;
    sdoc["stateControl"] = ss.lastReading;
    sdoc["cloudIOSupport"] = ss.cloudIOSupport;
  }
  serializeJson(doc, payload);
  http.begin(client, "http://cloudio.bhonofre.pt/devices");
  http.addHeader("Content-Type", "application/json");
#ifdef DEBUG_ONOFRE
  Log.error("%s [HTTP] POST %d" CR, tags::cloudIO, reconectCount);
#endif
  // start connection and send HTTP header and body
  int httpCode = http.POST(payload.c_str());

  // httpCode will be negative on error
  if (httpCode > 0)
  {

#ifdef DEBUG_ONOFRE
    Log.error("%s [HTTP] POST... code: %d" CR, tags::cloudIO, httpCode);
#endif
    // file found at server
    if (httpCode == HTTP_CODE_NO_CONTENT)
    {
      reconectCount = 0;
      return;
    }
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = http.getString();
      StaticJsonDocument<200> resp;
      DeserializationError error = deserializeJson(doc, payload);
      const char *_user = doc["username"];
      const char *_pw = doc["password"];
      strlcpy(config.cloudIOUsername, doc["username"] | "", sizeof(config.cloudIOUsername));
      strlcpy(config.cloudIOPassword, doc["password"] | "", sizeof(config.cloudIOPassword));

      topicAvailable.concat(config.cloudIOUsername);
      topicAvailable.concat("/");
      topicAvailable.concat(getChipId());
      topicAvailable.concat("/available");
#ifdef DEBUG_ONOFRE
      Log.error("%s USER: %s PASSWORD: %s" CR, tags::cloudIO, _user, _pw);
#endif
      if (!error && _user && _pw)
      {
#ifdef DEBUG_ONOFRE
        Log.error("%s SETUP MQTT CLOUD" CR, tags::cloudIO);
#endif
        if (mqttClient.connected())
        {
          mqttClient.disconnect();
        }
        reconectCount = 0;
        tryCloudConnection();
      }
      else
      {
#ifdef DEBUG_ONOFRE
        Log.error("%s NO USER FOUND" CR, tags::cloudIO);
#endif
      }

#ifdef DEBUG_ONOFRE
      Log.error("%s [HTTP] POST... Result: %s" CR, tags::cloudIO, payload.c_str());
#endif
    }
    else
    {
#ifdef DEBUG_ONOFRE
      Log.error("%s [HTTP] POST... failed, error: %s" CR, tags::cloudIO, http.errorToString(httpCode).c_str());
#endif
      cloudIOReconnectTimer.once(10, cloudIoKeepAlive);
    }
  }
  else
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s [HTTP] POST... error: %s" CR, tags::cloudIO, http.errorToString(httpCode).c_str());
#endif
    cloudIOReconnectTimer.once(10, cloudIoKeepAlive);
  }
}
