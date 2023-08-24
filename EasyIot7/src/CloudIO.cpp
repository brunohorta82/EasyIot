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

  subscribeOnMqttCloudIO(config.writeTopic);
  for (auto &sw : config.actuatores)
  {
    subscribeOnMqttCloudIO(sw.writeTopic);
    notifyStateToCloudIO(sw.readTopic, String(sw.state).c_str());
  }
  mqttClient.publish(config.healthTopic, 0, true, "1\0");
  for (auto &ss : config.sensors)
  {
    notifyStateToCloudIO(ss.readTopic, ss.lastReading);
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
  if (strcmp(topic, config.writeTopic) == 0)
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
    // TODO  mqttSwitchControl(SwitchStateOrigin::CLOUDIO, topic, msg);
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
WiFiClient client;
HTTPClient http;
void connectToCloudIO()
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
  size_t s = config.actuatores.size();
  size_t ss = config.sensors.size();
  DynamicJsonDocument doc(DYNAMIC_JSON_DOCUMENT_SIZE);
  JsonVariant root = doc.to<JsonVariant>();
  config.json(root);
  serializeJson(doc, payload);
  http.begin(client, "http://cloudio.bhonofre.pt/devices/config");
  http.addHeader("Content-Type", "application/json");
#ifdef DEBUG_ONOFRE
  Log.error("%s [HTTP] POST %d" CR, tags::cloudIO, reconectCount);
#endif
  int httpCode = http.POST(payload.c_str());
  if (httpCode > 0)
  {

#ifdef DEBUG_ONOFRE
    Log.error("%s [HTTP] POST... code: %d" CR, tags::cloudIO, httpCode);
#endif
    if (httpCode == HTTP_CODE_NO_CONTENT)
    {
      reconectCount = 0;
      return;
    }
    if (httpCode == HTTP_CODE_BAD_REQUEST)
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
