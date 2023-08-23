#include "Mqtt.h"
#include "ConfigOnofre.h"
#include <PubSubClient.h>
#include "Actuatores.h"
#include "CoreWiFi.h"
#include "constants.h"
#include "HomeAssistantMqttDiscovery.h"
extern ConfigOnofre config;
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);
void processMqttAction(const char *topic, const char *payload)
{
    mqttSwitchControl(SwitchStateOrigin::MQTT, topic, payload);
}
void callbackMqtt(char *topic, byte *payload, unsigned int length)
{
#ifdef DEBUG_ONOFRE
    Log.notice("%s Message received" CR, tags::mqtt);
#endif
    char *payload_as_string = (char *)malloc(length + 1);
    memcpy(payload_as_string, (char *)payload, length);
    payload_as_string[length] = 0;
#ifdef DEBUG_ONOFRE
    Log.notice("%s Topic: %s" CR, tags::mqtt, topic);
    Log.notice("%s Payload: " CR, tags::mqtt, payload_as_string);
#endif
    if ((strcmp(topic, String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/status").c_str()) == 0 || strcmp(topic, String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefixLegacy) + "/status").c_str()) == 0) && strcmp(payload_as_string, constantsMqtt::availablePayload) == 0)
    {
        initHomeAssistantDiscovery();
    }
    else
    {
        processMqttAction(topic, payload_as_string);
    }

    free(payload_as_string);
}

String getBaseTopic()
{
    String topic;

    if (strlen(config.mqttUsername) == 0)
    {
        topic.concat("bhonofre");
    }
    else
    {
        topic.concat(config.mqttUsername);
    }
    topic.concat("/");
    topic.concat(config.chipId);
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

boolean reconnect()
{

    if (WiFi.status() != WL_CONNECTED || strlen(config.mqttIpDns) == 0)
        return false;
#ifdef DEBUG_ONOFRE
    Log.notice("%s Trying to connect on broker %s" CR, tags::mqtt, config.mqttIpDns);
#endif
    publishMessage(getAvailableTopic(), constantsMqtt::unavailablePayload);
    if (mqttClient.connect(config.chipId, String(config.mqttUsername).c_str(), String(config.mqttPassword).c_str(), getAvailableTopic().c_str(), 0, true, constantsMqtt::unavailablePayload, true))
    {
#ifdef DEBUG_ONOFRE
        Log.notice("%s Connected to %s" CR, tags::mqtt, config.mqttIpDns);
#endif
        publishOnMqtt(getAvailableTopic().c_str(), constantsMqtt::availablePayload, true);
        publishOnMqtt(getConfigStatusTopic().c_str(), String("{\"firmware\":" + String(VERSION, 3) + "}").c_str(), true);
        subscribeOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefix) + "/status").c_str());
        subscribeOnMqtt(String(String(constantsMqtt::homeAssistantAutoDiscoveryPrefixLegacy) + "/status").c_str());
        publishMessage(getAvailableTopic(), constantsMqtt::availablePayload);
        for (auto &sw : config.actuatores)
        {
            // TODO subscribeOnMqtt(sw.writeTopic);
            // TODO publishOnMqtt(sw.readTopic, sw.getCurrentState().c_str(), false);
        }
    }

    return mqttConnected();
}

void setupMQTT()
{

#ifdef DEBUG_ONOFRE
    Log.notice("%s Setup" CR, tags::mqtt);
#endif
    if (mqttConnected())
    {
        mqttClient.disconnect();
    }
    if (strlen(config.mqttIpDns) == 0)
    {
        return;
    }
    mqttClient.setServer(config.mqttIpDns, config.mqttPort);
    mqttClient.setCallback(callbackMqtt);
}

void loopMqtt()
{
    if (WiFi.status() != WL_CONNECTED || strlen(config.mqttIpDns) == 0)
        return;
    static unsigned long lastReconnectAttempt = millis();
    if (!mqttConnected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 8000)
        {
#ifdef DEBUG_ONOFRE
            Log.notice("%s Disconnected" CR, tags::mqtt);
#endif
            lastReconnectAttempt = now;
            if (reconnect())
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        mqttClient.loop();
    }
}

void publishOnMqtt(const char *topic, const char *payload, bool retain)
{
    if (strlen(config.mqttIpDns) == 0)
    {
#ifdef DEBUG_ONOFRE
        Log.warning("%s Setup required to publish messages" CR, tags::mqtt);
#endif
        return;
    }
    if (!mqttConnected())
    {
#ifdef DEBUG_ONOFRE
        Log.warning("%s Connection Required" CR, tags::mqtt);
#endif
        return;
    }
    static unsigned int retries = 0;
    while (!mqttClient.publish(topic, payload, retain) && retries < 3)
    {
        retries++;
    }
#ifdef DEBUG_ONOFRE
    if (retries < 3)
    {

        Log.error("%s Message %s published to the topic %s successfully" CR, tags::mqtt, payload, topic);
    }
    else
    {

        Log.error("%s Error on try publish to the topic %s with message %s" CR, tags::mqtt, topic, payload);
    }
#endif
    retries = 0;
}
bool mqttConnected()
{
    return mqttClient.connected();
}
void subscribeOnMqtt(const char *topic)
{
    if (strlen(config.mqttIpDns) == 0)
    {
#ifdef DEBUG_ONOFRE
        Log.warning("%s Setup required to subscrive messages" CR, tags::mqtt);
#endif
        return;
    }
    if (!mqttConnected())
    {
#ifdef DEBUG_ONOFRE
        Log.warning("%s Required Mqtt connection" CR, tags::mqtt);
#endif
        return;
    }
    mqttClient.subscribe(topic);
}
void unsubscribeOnMqtt(const char *topic)
{
    if (mqttConnected())
    {
#ifdef DEBUG_ONOFRE
        Log.notice("%s Unsubscribe on topic %s " CR, tags::mqtt, topic);
#endif
        mqttClient.unsubscribe(topic);
    }
}
