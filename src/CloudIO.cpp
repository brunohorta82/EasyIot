#include "CloudIO.h"
#include "ConfigOnofre.h"
#include "CoreWiFi.h"
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "Constants.h"
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
Ticker checkCloudIOWatchdog;

void disconnectToClounIOMqtt()
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
    if (sw.isVirtual())
    {
      continue;
    }
    subscribeOnMqttCloudIO(sw.cloudIOwriteTopic);
    notifyStateToCloudIO(sw.cloudIOreadTopic, String(sw.state).c_str());
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
#ifdef DEBUG_ONOFRE
  Log.warning("%s Disconnected from MQTT." CR, tags::cloudIO);
#endif
  if (WiFi.isConnected())
  {
    connectToCloudIOMqtt();
  }
}
bool cloudIOConnected()
{
  return mqttClient.connected();
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  char msg[len + 1];
  strlcpy(msg, payload, sizeof(msg));
#ifdef DEBUG_ONOFRE
  Log.info("%s Message from MQTT. %s %s" CR, tags::cloudIO, topic, msg);
#endif
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
    config.controlFeature(StateOrigin::CLOUDIO, topic, msg);
  }
}

void setupMqttCloudIO()
{
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setCleanSession(true);
  mqttClient.setKeepAlive(36);
  mqttClient.setWill(config.cloudIOhealthTopic, 0, true, "0\0");
  mqttClient.setServer(constanstsCloudIO::mqttDns, constanstsCloudIO::mqttPort);
  mqttClient.setClientId(config.chipId);
  mqttClient.setCredentials(config.cloudIOUsername, config.cloudIOPassword);
  connectToCloudIOMqtt();
}
bool tryMqttCloudConnection()
{
  if (mqttClient.connected())
  {
    mqttClient.disconnect();
  }
  if (strlen(config.cloudIOUsername) > 0 && strlen(config.cloudIOPassword) > 0)
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s Ready to try..." CR, tags::cloudIO);
#endif
    setupMqttCloudIO();
  }
  return true;
}
void watchdogTimer()
{
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Watchdog running." CR, tags::cloudIO);
#endif
  if (!wifiConnected())
  {
#ifdef DEBUG_ONOFRE
    Log.warning("%s No Internet Connection." CR, tags::cloudIO);
#endif
    return;
  }
  if (mqttClient.connected())
  {
#ifdef DEBUG_ONOFRE
    Log.info("%s CloudIO OK." CR, tags::cloudIO);
#endif
    return;
  }
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Reconnect." CR, tags::cloudIO);
#endif
  config.requestCloudIOSync();
}
void ConfigOnofre::startCloudIOWatchdog()
{
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Watchdog Started" CR, tags::cloudIO);
#endif
  checkCloudIOWatchdog.attach_ms(60000, watchdogTimer);
}
void ConfigOnofre::stopCloudIOWatchdog()
{
#ifdef DEBUG_ONOFRE
  Log.info("%s CloudIO Watchdog Stopped" CR, tags::cloudIO);
#endif
  checkCloudIOWatchdog.detach();
}
void connectToCloudIO()
{

  if (!wifiConnected())
  {
#ifdef DEBUG_ONOFRE
    Log.error("%s WIFI DISCONECTED" CR, tags::cloudIO);
#endif
    return;
  }
  HTTPClient http;
  WiFiClient client;
  String payload = "";
  JsonDocument doc;
  JsonVariant root = doc.to<JsonVariant>();
  config.json(root, false);
  serializeJson(doc, payload);
  http.begin(client, constanstsCloudIO::configUrl);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(payload.c_str());
  doc.clear();
  config.cloudIOReady = false;
#ifdef DEBUG_ONOFRE
  Log.info("%s [HTTP]  Request result: %d" CR, tags::cloudIO, httpCode);
#endif
  if (httpCode == HTTP_CODE_NO_CONTENT)
  {
#ifdef DEBUG_ONOFRE
    Log.info("%s [HTTP] Device not adopted" CR, tags::cloudIO);
#endif
    config.stopCloudIOWatchdog();
    return;
  }
  if (httpCode != HTTP_CODE_OK)
  {
    return;
  }
  else if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    JsonDocument resp;
    DeserializationError error = deserializeJson(doc, payload);
    strlcpy(config.cloudIOUsername, doc["username"] | "", sizeof(config.cloudIOUsername));
    strlcpy(config.cloudIOPassword, doc["password"] | "", sizeof(config.cloudIOPassword));
    config.cloudIOReady = true;
    snprintf(config.cloudIOhealthTopic, sizeof(config.cloudIOhealthTopic), "%s/%s/available", config.cloudIOUsername, config.chipId);
    snprintf(config.cloudIOwriteTopic, sizeof(config.cloudIOwriteTopic), "%s/%s/config/set", config.cloudIOUsername, config.chipId);
    for (auto &sw : config.actuatores)
    {
      String family = sw.familyToText();
      family.toLowerCase();
      snprintf(sw.cloudIOwriteTopic, sizeof(sw.cloudIOwriteTopic), "%s/%s/%s/%s/set", config.cloudIOUsername, config.chipId, family.c_str(), sw.uniqueId);
      snprintf(sw.cloudIOreadTopic, sizeof(sw.cloudIOreadTopic), "%s/%s/%s/%s/status", config.cloudIOUsername, config.chipId, family.c_str(), sw.uniqueId);
    }
    for (auto &ss : config.sensors)
    {
      String family = ss.familyToText();
      family.toLowerCase();
      snprintf(ss.cloudIOreadTopic, sizeof(ss.cloudIOreadTopic), "%s/%s/%s/%s/metrics", config.cloudIOUsername, config.chipId, family.c_str(), ss.uniqueId);
    }
    resp.clear();
    if (!error && strlen(config.cloudIOUsername) > 0 && strlen(config.cloudIOPassword) > 0)
    {
#ifdef DEBUG_ONOFRE
      Log.info("%s SETUP MQTT CLOUD" CR, tags::cloudIO);
#endif
      tryMqttCloudConnection();
    }
  }
  client.stop();
  http.end();
}
