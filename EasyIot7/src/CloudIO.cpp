#include "CloudIO.h"
#include "ConfigOnofre.h"
#include "CoreWiFi.h"
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "constants.h"
#include "Actuatores.h"
#include "Sensors.h"
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#endif
#ifdef ESP32
#include <HTTPClient.h>
#endif
extern ConfigOnofre config;
AsyncMqttClient mqttClient;
HTTPClient http;
Ticker mqttReconnectTimer;
Ticker cloudIOReconnectTimer;

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
void notifyStateToCloudIO(const char *topic, const char *state)
{
  if (!mqttClient.connected())
    return;
  mqttClient.publish(topic, 0, true, state);
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
  mqttClient.publish(config.cloudIOhealthTopic, 0, true, "1\0");
  subscribeOnMqttCloudIO(config.cloudIOwriteTopic);
  for (auto &sw : config.actuatores)
  {
    //  subscribeOnMqttCloudIO(sw.cloudIOwriteTopic);
    // notifyStateToCloudIO(sw.cloudIOreadTopic, String(sw.state).c_str());
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
  char msg[len + 1];
#ifdef DEBUG_ONOFRE
  Log.warning("%s Message from MQTT. %s %s" CR, tags::cloudIO, topic, payload);
#endif
  strlcpy(msg, payload, sizeof(msg));
  if (strcmp(topic, config.cloudIOwriteTopic) == 0)
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
    config.controlFeature(SwitchStateOrigin::CLOUDIO, topic, msg);
  }
}

void setupCloudIO()
{
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setCleanSession(true);
  mqttClient.setWill(config.cloudIOhealthTopic, 0, true, "0\0");
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  mqttClient.setClientId(config.chipId);
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

void connectToCloudIO()
{
  cloudIOReconnectTimer.detach();
  if (!wifiConnected() || reconectCount > 3)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s [HTTP] reconectCount %d" CR, tags::cloudIO, reconectCount);
#endif
    return;
  }
  WiFiClient client;
  reconectCount++;
  String payload = "";
  DynamicJsonDocument doc(DYNAMIC_JSON_DOCUMENT_SIZE);
  JsonVariant root = doc.to<JsonVariant>();
  config.json(root);
  serializeJson(doc, payload);
  http.begin(client, constanstsCloudIO::configUrl);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(payload.c_str());
#ifdef DEBUG_ONOFRE
  Log.error("%s [HTTP] POST... code: %d" CR, tags::cloudIO, httpCode);
#endif
  if (httpCode == HTTP_CODE_NO_CONTENT || httpCode == HTTP_CODE_BAD_REQUEST)
  {
    reconectCount = 0;
    return;
  }
  else if (httpCode == HTTP_CODE_OK)
  {
    const String &payload = http.getString();
    StaticJsonDocument<200> resp;
    DeserializationError error = deserializeJson(doc, payload);
    strlcpy(config.cloudIOUsername, doc["username"] | "", sizeof(config.cloudIOUsername));
    strlcpy(config.cloudIOPassword, doc["password"] | "", sizeof(config.cloudIOPassword));
    snprintf(config.cloudIOhealthTopic, sizeof(config.cloudIOhealthTopic), "%s/%s/available", config.cloudIOUsername, config.chipId);
    snprintf(config.cloudIOwriteTopic, sizeof(config.cloudIOwriteTopic), "%s/%s/config/set", config.cloudIOUsername, config.chipId);
    for (auto &sw : config.actuatores)
    {
      snprintf(sw.cloudIOwriteTopic, sizeof(sw.cloudIOwriteTopic), "%s/%s/%s/%s/set", config.cloudIOUsername, config.chipId, sw.uniqueId, sw.familyToText());
      snprintf(sw.cloudIOreadTopic, sizeof(sw.cloudIOreadTopic), "%s/%s/%s/%s/status", config.cloudIOUsername, config.chipId, sw.uniqueId, sw.familyToText());
    }
    for (auto &ss : config.sensors)
    {
      snprintf(ss.cloudIOreadTopic, sizeof(ss.cloudIOreadTopic), "%s/%s/%s/%s/status", config.cloudIOUsername, config.chipId, ss.uniqueId, ss.family);
    }
#ifdef DEBUG_ONOFRE
    Log.error("%s USER: %s PASSWORD: %s" CR, tags::cloudIO, config.cloudIOUsername, config.cloudIOPassword);
#endif
    if (!error && strlen(config.cloudIOUsername) > 0 && strlen(config.cloudIOPassword) > 0)
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
