#include "Mqtt.h"
#include "Config.h"
#include <AsyncMqttClient.h>
#include "Switches.h"
#include "WiFi.h"
#include <Ticker.h>
#include "constants.h"
#include "ESP8266WiFi.h"
#include "Discovery.h"

AsyncMqttClient mqttLocalClient;
Ticker mqttLocalReconnectTimer;
Ticker mqttLocal;

void processMqttAction(const char *topic, const char *payload)
{
    mqttSwitchControl(getAtualSwitchesConfig(), topic, payload);
}
void connectLocalMqtt(){
    mqttLocal.once(5, setupMQTT);
}

String getBaseTopic()
{
    String topic;

    if (strlen(getAtualConfig().mqttUsername) == 0)
    {
        topic.concat("easyiot");
    }
    else
    {
        topic.concat(getAtualConfig().mqttUsername);
    }
    topic.concat("/");
    topic.concat(ESP.getChipId());
    return topic;
}
String getAvailableTopic()
{
    String baseTopic = getBaseTopic();
    baseTopic.concat("/available");
    return baseTopic;
}
String getConfigStatusTopic()
{
    String baseTopic = getBaseTopic();
    baseTopic.concat("/config/status");
    return baseTopic;
}


void connectToLocalMqtt()
{
#ifdef DEBUG
  Log.error("%s Connecting to MQTT..." CR, tags::mqtt);
#endif
  mqttLocalClient.connect();
}


void onMqttLocalPublish(uint16_t packetId)
{
}
void onMqttLocalDisconnect(AsyncMqttClientDisconnectReason reason)
{

#ifdef DEBUG
  Log.warning("%s Disconnected from MQTT." CR, tags::mqtt);
#endif
  if (WiFi.isConnected())
  {
    mqttLocalReconnectTimer.once(10, connectToLocalMqtt);
  }
}

void onMqttLocalSubscribe(uint16_t packetId, uint8_t qos)
{
}

void onMqttLocalUnsubscribe(uint16_t packetId)
{
}

void onMqttLocalMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  char msg[100];
#ifdef DEBUG
  Log.warning("%s Message from MQTT. %s %s" CR, tags::mqtt, topic, payload);
#endif
  strlcpy(msg, payload, len + 1);
  #ifdef DEBUG
    Log.notice("%s Message received" CR, tags::mqtt);
#endif
#ifdef DEBUG
    Log.notice("%s Topic: %s" CR, tags::mqtt, topic);
    Log.notice("%s Payload: " CR, tags::mqtt, msg);
#endif
    if (strcmp(topic, constantsMqtt::homeassistantOnlineTopic) == 0 && strcmp(msg, constantsMqtt::availablePayload) == 0)
    {
        initHaDiscovery(getAtualSwitchesConfig());
        initHaDiscovery(getAtualSensorsConfig());
    }
    else
    {
        processMqttAction(topic, msg);
    }

}


void onMqttLocalConnect(bool sessionPresent)
{
    #ifdef DEBUG
        Log.notice("%s Connected to %s" CR, tags::mqtt, getAtualConfig().mqttIpDns);
#endif
        publishOnMqtt(getAvailableTopic().c_str(), constantsMqtt::availablePayload, true);
        publishOnMqtt(getConfigStatusTopic().c_str(), "{\"firmware\":7.0}", true); //TODO generate simple config status
        subscribeOnMqtt(constantsMqtt::homeassistantOnlineTopic);
        //Init Switches Subscritions and publish de current state
        refreshMDNS(getAtualConfig().nodeId);
        for (auto &sw : getAtualSwitchesConfig().items)
        {
            subscribeOnMqtt(sw.mqttCommandTopic);
            publishOnMqtt(sw.mqttStateTopic, sw.mqttPayload, sw.mqttRetain);
        }


}
void setupMQTT()
{
    if (strlen(getAtualConfig().mqttIpDns) == 0)
    {
        return;
    }
#ifdef DEBUG
    Log.notice("%s Setup" CR, tags::mqtt);
#endif
  mqttLocalClient.onConnect(onMqttLocalConnect);
  mqttLocalClient.onDisconnect(onMqttLocalDisconnect);
  mqttLocalClient.onSubscribe(onMqttLocalSubscribe);
  mqttLocalClient.onUnsubscribe(onMqttLocalUnsubscribe);
  mqttLocalClient.onMessage(onMqttLocalMessage);
  mqttLocalClient.onPublish(onMqttLocalPublish);
  mqttLocalClient.setCleanSession(true);
  mqttLocalClient.setServer(getAtualConfig().mqttIpDns,getAtualConfig().mqttPort);
  mqttLocalClient.setClientId(getAtualConfig().chipId);
  mqttLocalClient.setCredentials(getAtualConfig().mqttUsername, getAtualConfig().mqttPassword);
  mqttLocalClient.connect();
}

void publishOnMqtt(const char *topic, const char *payload, bool retain)
{
    if (strlen(getAtualConfig().mqttIpDns) == 0)
    {
#ifdef DEBUG
        Log.warning("%s Setup required to publish messages" CR, tags::mqtt);
#endif
        return;
    }
    if (!mqttConnected())
    {
#ifdef DEBUG
        Log.warning("%s Connection Required" CR, tags::mqtt);
#endif
        return;
    }
    mqttLocalClient.publish(topic,0,retain, payload, retain);
    
}
bool mqttConnected()
{
    return mqttLocalClient.connected();
}
void subscribeOnMqtt(const char *topic)
{
    if (strlen(getAtualConfig().mqttIpDns) == 0)
    {
#ifdef DEBUG
        Log.warning("%s Setup required to subscrive messages" CR, tags::mqtt);
#endif
        return;
    }
    if (!mqttConnected())
    {
#ifdef DEBUG
        Log.warning("%s Required Mqtt connection" CR, tags::mqtt);
#endif
        return;
    }
    mqttLocalClient.subscribe(topic,0);
}
void unsubscribeOnMqtt(const char *topic)
{
    if (mqttConnected())
    {
#ifdef DEBUG
        Log.notice("%s Unsubscribe on topic %s " CR, tags::mqtt, topic);
#endif
        mqttLocalClient.unsubscribe(topic);
    }
}
