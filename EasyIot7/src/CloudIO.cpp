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
Ticker mqttReconnectTimer;
Ticker cloudIOReconnectTimer;
int reconectCount = 0;

void disconnectToClounIOMqtt()
{
#ifdef DEBUG
  Log.error("%s Disconnect to MQTT..." CR, tags::cloudIO);
#endif
  mqttClient.disconnect();
}
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
  String topicAction;
  topicAction.reserve(200);
  topicAction.concat(getAtualConfig().cloudIOUserName);
  topicAction.concat("/");
  topicAction.concat(getAtualConfig().chipId);
  topicAction.concat("/remote-action");
  strlcpy(getAtualConfig().mqttCloudRemoteActionsTopic, topicAction.c_str(), sizeof(getAtualConfig().mqttCloudRemoteActionsTopic));
  subscribeOnMqttCloudIO(getAtualConfig().mqttCloudRemoteActionsTopic);
  for (auto &sw : getAtualSwitchesConfig().items)
  {

    String topic;
    topic.reserve(200);
    topic.concat(getAtualConfig().cloudIOUserName);
    topic.concat("/");
    topic.concat(getAtualConfig().chipId);
    topic.concat("/");
    topic.concat(sw.family);
    topic.concat("/");
    topic.concat(sw.id);
    topic.concat("/set");

    strlcpy(sw.mqttCloudCommandTopic, topic.c_str(), sizeof(sw.mqttCloudCommandTopic));
    subscribeOnMqttCloudIO(sw.mqttCloudCommandTopic);

    topic.replace("/set", "/status");

    strlcpy(sw.mqttCloudStateTopic, topic.c_str(), sizeof(sw.mqttCloudStateTopic));
    if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
    {
      char dump[4] = {0};
      int l = sprintf(dump, "%d", sw.lastPercentage);
      notifyStateToCloudIO(sw.mqttCloudStateTopic, dump, l);
    }
    else
    {
      mqttClient.publish(sw.mqttCloudStateTopic, 0, true, sw.getCurrentState());
    }
  }
  mqttClient.publish(getAtualConfig().availableCloudIO, 0, true, "1\0");
  for (auto &ss : getAtualSensorsConfig().items)
  {
    String topic;
    topic.reserve(200);
    topic.concat(getAtualConfig().cloudIOUserName);
    topic.concat("/");
    topic.concat(getAtualConfig().chipId);
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

#ifdef DEBUG
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
#ifdef DEBUG
  Log.warning("%s Message from MQTT. %s %s" CR, tags::cloudIO, topic, payload);
#endif
  strlcpy(msg, payload, len + 1);
  if (strcmp(topic, getAtualConfig().mqttCloudRemoteActionsTopic) == 0)
  {
    if (strcmp(msg, "REBOOT") == 0)
    {
      requestRestart();
    }
    if (strcmp(msg, "UPDATE") == 0)
    {
      requestAutoUpdate();
    }
  }
  else
  {
    mqttSwitchControl(getAtualSwitchesConfig(), topic, msg);
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
  mqttClient.setWill(getAtualConfig().availableCloudIO, 0, true, "0\0");
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  mqttClient.setClientId(getAtualConfig().chipId);
  mqttClient.setCredentials(getAtualConfig().cloudIOUserName, getAtualConfig().cloudIOUserPassword);
  connectToClounIOMqtt();
}
bool tryCloudConnection()
{
  if (strlen(getAtualConfig().cloudIOUserName) > 0 && strlen(getAtualConfig().cloudIOUserPassword) > 0)
  {
#ifdef DEBUG
    Log.error("%s Ready to try..." CR, tags::cloudIO);
#endif
    setupCloudIO();
  }
  return true;
}

void cloudIoKeepAlive()
{
  requestCloudIOSync();
}
WiFiClient client;
HTTPClient http;
void connectoToCloudIO()
{
  reconectCount++;
  cloudIOReconnectTimer.detach();
  if (WiFi.status() != WL_CONNECTED || reconectCount > 20)
    return;

  String payload = "";
  size_t s = getAtualSwitchesConfig().items.size();
  size_t ss = getAtualSensorsConfig().items.size();
  const size_t CAPACITY = JSON_ARRAY_SIZE(s + ss) + (s * JSON_OBJECT_SIZE(8) + sizeof(SwitchT)) + (ss * (JSON_OBJECT_SIZE(7) + sizeof(SensorT)));
  DynamicJsonDocument doc(CAPACITY);
  JsonObject device = doc.to<JsonObject>();
  device["chipId"] = getAtualConfig().chipId;
  device["currentVersion"] = String(VERSION, 3);
  device["nodeId"] = getAtualConfig().nodeId;
  device["wifi"] = getAtualConfig().wifiSSID;
  const char *firmwareMode = {FEATURES_TEMPLATE};
  device["firmwareMode"] = firmwareMode;
  device["macAddr"] = WiFi.macAddress();
  JsonArray feactures = device.createNestedArray("features");
  for (auto &sw : getAtualSwitchesConfig().items)
  {
    JsonObject sdoc = feactures.createNestedObject();
    sdoc["id"] = sw.id;
    sdoc["name"] = sw.name;
    sdoc["family"] = sw.family;
    sdoc["shutterPercentage"] = sw.lastPercentage;
    sdoc["stateControl"] = sw.getCurrentState();
    sdoc["cloudIOSupport"] = sw.cloudIOSupport;
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
#ifdef DEBUG
  Log.error("%s [HTTP] POST %d" CR, tags::cloudIO, reconectCount);
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
      strlcpy(getAtualConfig().cloudIOUserName, doc["username"] | "", sizeof(getAtualConfig().cloudIOUserName));
      strlcpy(getAtualConfig().cloudIOUserPassword, doc["password"] | "", sizeof(getAtualConfig().cloudIOUserPassword));
      String topicAvailable;
      topicAvailable.reserve(sizeof(getAtualConfig().availableCloudIO));
      topicAvailable.concat(getAtualConfig().cloudIOUserName);
      topicAvailable.concat("/");
      topicAvailable.concat(getAtualConfig().chipId);
      topicAvailable.concat("/available");
      strlcpy(getAtualConfig().availableCloudIO, topicAvailable.c_str(), sizeof(getAtualConfig().availableCloudIO));
#ifdef DEBUG
      Log.error("%s USER: %s PASSWORD: %s" CR, tags::cloudIO, _user, _pw);
#endif
      if (!error && _user && _pw)
      {
#ifdef DEBUG
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
      cloudIOReconnectTimer.once(10, cloudIoKeepAlive);
    }
  }
  else
  {
#ifdef DEBUG
    Log.error("%s [HTTP] POST... error: %s" CR, tags::cloudIO, http.errorToString(httpCode).c_str());
#endif
    cloudIOReconnectTimer.once(10, cloudIoKeepAlive);
  }
}
