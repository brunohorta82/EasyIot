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
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
extern Config config;
extern Switches switches;

void disconnectToCloudIOMqtt()
{
#ifdef DEBUG_ONOFRE
  Log.error("%s Disconnect to MQTT..." CR, tags::cloudIO);
#endif
  mqttClient.disconnect();
}
void connectToCloudIOMqtt()
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
void notifyStateToCloudIO(SwitchT *s)
{
  if (!mqttCloudIOConnected())
    return;
#ifdef DEBUG_ONOFRE
  Log.warning("%s Message sent to %s with value %s" CR, tags::cloudIO, s->mqttCloudStateTopic, s->getCurrentState());
#endif
  mqttClient.publish(s->mqttCloudStateTopic, 0, true, s->getCurrentState().c_str());
}
void notifyStateToCloudIO(const String topic, const String state)
{
  if (!mqttCloudIOConnected())
    return;
#ifdef DEBUG_ONOFRE
  Log.warning("%s Message sent to %s with value %s" CR, tags::cloudIO, topic, state);
#endif
  mqttClient.publish(topic.c_str(), 0, true, state.c_str());
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
  String topicAction;
  topicAction.reserve(200);
  topicAction.concat(config.cloudIOUserName);
  topicAction.concat("/");
  topicAction.concat(config.chipId);
  topicAction.concat("/remote-action");
  topicAction.toLowerCase();
  strlcpy(config.mqttCloudRemoteActionsTopic, topicAction.c_str(), sizeof(config.mqttCloudRemoteActionsTopic));
  subscribeOnMqttCloudIO(config.mqttCloudRemoteActionsTopic);
  for (auto &sw : switches.items)
  {

    String topic;
    topic.concat(config.cloudIOUserName);
    topic.concat("/");
    topic.concat(getChipId());
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
    if (strcmp(sw.family, constanstsSwitch::familyCover) == 0)
    {
      char dump[4] = {0};
      int l = sprintf(dump, "%d", sw.lastPercentage);
      notifyStateToCloudIO(sw.mqttCloudStateTopic, dump);
    }
    else
    {
      mqttClient.publish(sw.mqttCloudStateTopic, 0, true, sw.getCurrentState().c_str());
    }
  }
  mqttClient.publish(config.availableCloudIO, 0, true, "1\0");
  for (auto &ss : getAtualSensorsConfig().items)
  {
    String topic;
    topic.reserve(200);
    topic.concat(config.cloudIOUserName);
    topic.concat("/");
    topic.concat(config.chipId);
    topic.concat("/");
    topic.concat(ss.deviceClass);
    topic.concat("/");
    topic.concat(ss.id);
    topic.concat("/status");
    topic.toLowerCase();
    strlcpy(ss.mqttCloudStateTopic, topic.c_str(), sizeof(ss.mqttCloudStateTopic));
    mqttClient.publish(ss.mqttCloudStateTopic, 0, true, ss.lastReading);
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{

#ifdef DEBUG_ONOFRE
  Log.warning("%s Disconnected from MQTT." CR, tags::cloudIO);
#endif
  if (wifiConnected())
  {
    mqttReconnectTimer.once(2, connectToCloudIOMqtt);
  }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  char msg[100];

  strlcpy(msg, payload, len + 1);
#ifdef DEBUG_ONOFRE
  Log.warning("%s Message from MQTT. %s %s" CR, tags::cloudIO, topic, msg);
#endif
  mqttSwitchControl(switches, topic, msg);
}

void setupCloudIO()
{
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setCleanSession(true);
  mqttClient.setKeepAlive(60);
  mqttClient.setWill(config.availableCloudIO, 0, true, "0\0");
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  mqttClient.setClientId(config.chipId);
  mqttClient.setCredentials(config.cloudIOUserName, config.cloudIOUserPassword);
  connectToCloudIOMqtt();
}
bool tryCloudConnection()
{
  if (strlen(config.cloudIOUserName) > 0 && strlen(config.cloudIOUserPassword) > 0)
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
void connectToCloudIO()
{
  if (!wifiConnected())
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s No WiFi Connection %d" CR, tags::cloudIO);
#endif
    return;
  }
  String payload = "";
  size_t s = switches.items.size();
  size_t ss = getAtualSensorsConfig().items.size();
  const size_t CAPACITY = JSON_ARRAY_SIZE(s + ss) + (s * JSON_OBJECT_SIZE(8) + sizeof(SwitchT)) + (ss * (JSON_OBJECT_SIZE(7) + sizeof(SensorT)));
  DynamicJsonDocument doc(CAPACITY);
  JsonObject device = doc.to<JsonObject>();
  device["chipId"] = config.chipId;
  device["currentVersion"] = String(VERSION, 3);
  device["nodeId"] = config.nodeId;
  device["wifi"] = config.wifiSSID;

  device["firmwareMode"] = "NO_FEATURES";
  device["macAddr"] = WiFi.macAddress();
  JsonArray feactures = device.createNestedArray("features");
  for (auto &sw : switches.items)
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
  http.begin(client, constanstsCloudIO::cloudioDevicesUrl);
  http.addHeader("Content-Type", "application/json");
#ifdef DEBUG_ONOFRE
  Log.error("%s [HTTP] POST " CR, tags::cloudIO);
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
      return;
    }
    if (httpCode == HTTP_CODE_OK)
    {
      const String &payload = http.getString();
      StaticJsonDocument<200> resp;
      DeserializationError error = deserializeJson(doc, payload);
      const char *_user = doc["username"];
      const char *_pw = doc["password"];
      strlcpy(config.cloudIOUserName, doc["username"] | "", sizeof(config.cloudIOUserName));
      strlcpy(config.cloudIOUserPassword, doc["password"] | "", sizeof(config.cloudIOUserPassword));
      String topicAvailable;
      topicAvailable.reserve(sizeof(config.availableCloudIO));
      topicAvailable.concat(config.cloudIOUserName);
      topicAvailable.concat("/");
      topicAvailable.concat(config.chipId);
      topicAvailable.concat("/available");
      strlcpy(config.availableCloudIO, topicAvailable.c_str(), sizeof(config.availableCloudIO));
#ifdef DEBUG_ONOFRE
      Log.error("%s USER: %s PASSWORD: %s" CR, tags::cloudIO, _user, _pw);
#endif
      if (!error && _user && _pw)
      {
#ifdef DEBUG_ONOFRE
        Log.error("%s SETUP MQTT CLOUD" CR, tags::cloudIO);
#endif
        if (mqttCloudIOConnected())
        {
          disconnectToCloudIOMqtt();
        }
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
    }
  }
  else
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s [HTTP] POST... error: %s" CR, tags::cloudIO, http.errorToString(httpCode).c_str());
#endif
  }
}
